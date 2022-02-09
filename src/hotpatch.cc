#include <limits.h>
#include <dirent.h>
#include <iostream>
#include <fstream>
#include <libunwind-ptrace.h>
#include <stdlib.h>
#include <unistd.h>

#include "arch.h"
#include "injector.h"
#include "hotpatch.h"
#include "elog.h"
#include "util.h"
#include "config.h"


using namespace std;
using namespace ns_patch;
using namespace YAML;

const static int MAX_RETRY_TIMES = 5;
const static int RETRY_INTERVAL_TIME_MS = 200;

int hot_patch::patch()
{
    if (m_pid == 0) {
        ELOG_ERROR("load fail.");
        return HP_ERR;
    }
    int i;
    for (i = 0; i < MAX_RETRY_TIMES; i++) {
        if (try_patch() == HP_OK) {
            break;
        }
        uint64_t wait_ms = RETRY_INTERVAL_TIME_MS * 1000 * (i + 1);
        ELOG_ERROR("try patch fail, wait (%u ms) then retry!", wait_ms);
        usleep(wait_ms);
    }
    if (i == MAX_RETRY_TIMES) {
        ELOG_ERROR("try patch process[%u] fail.", m_pid);
        return HP_ERR;
    }
    ELOG_NOTICE("patch pid[%u] success!", m_pid);
    return 0;
}

int hot_patch::try_patch() {
    int ret = HP_OK;
    ptrace_attach pt(m_proc.pids);
    ret = check_consistency();
    if (ret) {
        ELOG_ERROR("check consistency fail.");
        return ret;
    }
    ret = apply_patch();
    if (ret) {
        ELOG_ERROR("apply patch fail.");
        return ret;
    }
    return 0;
    
}

int hot_patch::check_consistency()
{
    int ret = HP_OK;
    hp_elf target_elf(m_proc.target);
    if (target_elf.is_open() == false) {
        ELOG_ERROR("parse target[%s] elf fail.", m_proc.target.c_str());
        return HP_ERR;
    }
    map<uint64_t, string> stacks;
    for (const auto &pid : m_proc.pids) {
        vector<ns_patch::hp_frame> stack;
        ret = get_stack(pid, stack);
        if (ret) {
            ELOG_ERROR("get pid[%u] stack fail.", pid);
            continue;
        }
        for (const auto &frame: stack) {
            if (stacks.find(frame.pc) == stacks.end()) {
                stacks[frame.pc - frame.func_offset] = frame.func_name;
            }
            //ELOG_NOTICE("stack[%s], addr[0x%016lx].", frame.func_name.c_str(), frame.pc-frame.func_offset);
        }
    }
    for (const auto &cfg : m_cfg->get().patch) {
        hp_elf target_elf(cfg.target);
        if (target_elf.is_open() == false && target_elf.load(m_proc.target) == false) {
            ELOG_ERROR("parse target[%s] elf fail.", cfg.target.c_str());
            continue;
        }
        for (const auto &func : cfg.funcs) {
            hp_function old_func = get_function(func.old_name, func.old_addr, target_elf);
            auto it = stacks.find(old_func.offset+ old_func.start);
            if (it != stacks.end()) {
                ELOG_WARN("find target func[%s, 0x%016lx] on stack.", it->second.c_str(), it->first);
                ret = HP_ERR;
                break;
            } 
        }
        if (ret) {
            break;
        }
    }
    return ret;
}

uint64_t hot_patch::get_code_start_addr(const string &file)
{
    if (m_proc.maps.find(file) == m_proc.maps.end()) {
        ELOG_ERROR("can't find address of file[%s] in memory.", file.c_str());
        return HP_ERR;
    }
    return m_proc.maps[file].addr[elf_type::CODE].start_addr;
}

int hot_patch::apply_patch() 
{
    int ret = HP_OK;
    for (const auto &cfg : m_cfg->get().patch) {
        hp_elf patch_elf(cfg.patch);
        hp_elf target_elf(cfg.target);
        if (patch_elf.is_open() == false) {
            ELOG_ERROR("parse patch[%s] elf fail.", cfg.patch.c_str());
            continue;
        }
        if (target_elf.is_open() == false && target_elf.load(m_proc.target) == false) {
            ELOG_ERROR("parse target[%s] elf fail.", cfg.target.c_str());
            continue;
        }

        vector<hp_backup> recover;
        for (const auto &func : cfg.funcs) {
            hp_function old_func = get_function(func.old_name, func.old_addr, target_elf);
            hp_function new_func = get_function(func.new_name, func.new_addr, patch_elf);
            if (old_func.size == 0 || new_func.size == 0) {
                ELOG_ERROR("get function fail, new func[%s], old func[%s].", func.new_name.c_str(), func.old_name.c_str());
                break;
            }
            hp_backup bak;
            bak.name = old_func.name;
            bak.addr = old_func.start + old_func.offset;
            bak.size = old_func.size;
            ret = bytecode_read(bak.addr, bak.size, bak.byte_codes);
            if (ret) {
                ELOG_ERROR("backup func[%s] byte codes fail.", bak.name.c_str());
                break;
            }
            recover.emplace_back(bak);
            ret = replace(new_func, old_func);
            if (ret) {
                ELOG_ERROR("invalid target[%s] that to patch.", m_proc.target.c_str());
                break;
            }
        }
        if (ret) {
            for (const auto &f: recover) {
                ELOG_NOTICE("Restore Func[%s] -- addr:0x%lx, size:0x%lx", f.name.c_str(), f.addr, f.size);
                int ret = bytecode_write(f.addr, f.byte_codes);
                HP_ASSERT(ret == 0);
            }
            break;
        }
        ret = backup(recover);
        if (ret) {
            ELOG_ERROR("backup patch[%s] fail, it can't rollback.", cfg.name.c_str());
        }
    }
    return ret;
}

hp_function hot_patch::get_function(const string &func_name, const uint64_t &func_addr, hp_elf &elf)
{
    hp_function rfunc;

    rfunc.start = get_code_start_addr(elf.get_file());
    if (!func_name.empty()) {
        if (!elf.is_func_exist(func_name)) {
            ELOG_ERROR("invalid function[%s] that to patch.", func_name.c_str());
            return rfunc;
        }
        rfunc.name = func_name;
        rfunc.offset = elf.get_offset(rfunc.name);
        rfunc.size = elf.get_size(rfunc.name);
    }
    if (func_addr > 0) {
        rfunc.name = elf.get_func_name(func_addr);
        if (rfunc.name.empty()) {
            ELOG_ERROR("invalid function offset[0x%016lx].", func_addr);
            rfunc.size = 0;
            return rfunc;
        }
        rfunc.offset = func_addr;
        rfunc.size = elf.get_size(rfunc.name);
    }
    
    return rfunc;
}
int hot_patch::replace(const hp_function &new_func, const hp_function &old_func)
{
    vector<uint8_t> data;
    uint8_t byte_code[FUNC_BYTE_CODE_MIN_LEN + 1] = {};
    int len = 0;

    ELOG_NOTICE("New func[%s] -- start:0x%016lx, offset:0x%lx, size:%lu", new_func.name.c_str(), new_func.start, new_func.offset, new_func.size);
    ELOG_NOTICE("Old func[%s] -- start:0x%016lx, offset:0x%lx, size:%lu", old_func.name.c_str(), old_func.start, old_func.offset, old_func.size);
    ELOG_NOTICE("Get arch: %s", ARCH_NAME);
    len = get_jump_byte_codes(new_func.start + new_func.offset, byte_code, sizeof(byte_code));
    if (len <= 0) {
        ELOG_ERROR("get jump byte codes fail.");
        return HP_ERR;
    }
    if (old_func.size < (uint64_t)len) {
        ELOG_ERROR("the size of replace function must be large to byte codes.");
        return HP_ERR;
    }
    for (int i = 0; i < len; i++) {
        data.push_back(byte_code[i]);
    }
    int ret = bytecode_write(old_func.start + old_func.offset, data);
    HP_ASSERT(ret == 0);
    return 0;
}

int hot_patch::backup(const vector<hp_backup> &recover)
{
    // TODO backup old func to memory
    for (const auto &f: recover) {
        ELOG_NOTICE("backup Func[%s] -- addr:0x%016lx, size:%lu", f.name.c_str(), f.addr, f.size);
    }
    return 0;
}


int hot_patch::init(const uint32_t &pid, const std::string &yaml)
{
    if (pid == 0) {
        ELOG_ERROR("target pid [%u] fail.", pid);
        return HP_ERR;
    }
    if (is_file_exsit("/proc/"+to_string(pid)) == false) {
        ELOG_ERROR("target pid [%u] not running, check it.", pid);
        return HP_ERR;
    }
    std::unique_ptr<ns_patch::config> pcfg(new config(yaml));
    if (pcfg->load() != 0) {
        ELOG_ERROR("load yaml file[%s] fail.", yaml.c_str());
        return HP_ERR;
    }
    m_cfg = std::move(pcfg);
    
    for (const auto &patch : m_cfg->get().patch) {
        if (load_lib(patch) != HP_OK) {
            ELOG_ERROR("load lib[%s] fail.", patch.patch.c_str());
            return HP_ERR;
        }
    }

    if (load_proc(pid, m_proc) != 0) {
        ELOG_ERROR("find pid[%u] fail.", pid);
        return HP_ERR;
    }   
    ELOG_NOTICE("Load pid[%u] with cfg[%s] success!", pid, yaml.c_str());
    return 0;
}

int hot_patch::load_proc(const uint32_t &main_pid, hp_process &proc)
{
    int ret = 0;

    ret = get_status(main_pid, proc.status);
    if (ret) {
        ELOG_ERROR("get status with pid[%u] fail.", main_pid);
        return HP_ERR;
    }

    ret = get_maps(main_pid, proc.maps, proc.target);
    if (ret) {
        ELOG_ERROR("get maps with pid[%u] fail.", main_pid);
        return HP_ERR;
    }

    ret = get_pids(main_pid, proc.pids);
    if (ret) {
        ELOG_ERROR("get pids with pid[%u] fail.", main_pid);
        return HP_ERR;
    }

    return 0;
}


bool hot_patch::is_loaded(const string& libpath)
{
    for (const auto &map : m_proc.maps) {

        if (map.first == libpath) {
            return true;
        }
    }
    return false;
}

map_type hot_patch::get_map_type(const string& map_str)
{
    map_type t;
    if (is_file_exsit(map_str)) {
        if (is_find(map_str, ".so")) {
            t = map_type::LIB;
        } else {
            t = map_type::EXE;
        }
    } else {
        if (map_str == "[heep]") {
            t = map_type::HEAP;
        } else if (map_str == "[stack]") {
            t = map_type::STACK;
        } else {
            t = map_type::OTHER;
        }
    }
    return t;
}

elf_type hot_patch::get_elf_type(const string& str)
{
    elf_type t;
    if (str == "r-xp") {
        t = elf_type::CODE;
    } else if (str == "r--p") {
        t = elf_type::DATA;
    } else if (str == "rw-p") {
        t = elf_type::VALUE;
    } else {
        t = elf_type::UNKNOWN;
    }
    return t;
}


int hot_patch::get_status(const uint32_t &pid, map<string, string> &status)
{
    string sta_path = "/proc/" + to_string(pid) + "/status";
    ifstream proc(sta_path, ios::in);
    if (!proc) {
        ELOG_ERROR("open [%s] status fail, maybe process exit?.", sta_path.c_str());
        return -1;
    }
    string line;
    while(getline(proc, line)) {
        if (line.empty()) {
            continue;
        }
        vector<string> map = string_split(line, ":");
        if (map.size() != 2) {
            continue;
        }
        
        status[string_strip(map[0])] = string_strip(map[1]);
        if (status.size() > 5) { // define it interst
            break;
        }
    }
    return 0;
}


int hot_patch::get_pids(const uint32_t &mpid, std::vector<uint32_t> &pids)
{
    string task_path = "/proc/" + to_string(mpid) + "/task";
    vector<string> dirs;
    int ret = get_dirs(task_path, dirs);
    if (ret) {
        ELOG_ERROR("get dirs on [%s] fail.", task_path.c_str());
        return -1;
    }
    
    if (!pids.empty()) {
        pids.clear();
    }
    for (const auto &d : dirs) {
        if (!is_number(d)) {
            continue;
        }
        pids.emplace_back(std::stoi(d));
    }

    return 0;
}


int hot_patch::get_maps(const uint32_t &pid, map<string, hp_map> &maps, string &target)
{
    string maps_path = "/proc/" + to_string(pid) + "/maps";
    ifstream proc(maps_path, ios::in);
    if (!proc) {
        ELOG_ERROR("open maps[%s] fail, maybe process exit?.", maps_path.c_str());
        return -1;
    }
    string line;
    // xx-xx r-- file_offset fd:00 映射文件所属节点号  有名文件映射
    while(getline(proc, line)) {    
        if (line.empty()) {
            continue;
        }
        vector<string> map = string_split(line, " ");
        if (map.size() < 6) {
            continue;
        }
        vector<string> addr = string_split(map[0], "-");
        if (addr.size() != 2) {
            ELOG_ERROR("addr[%s] parse fail.", map[0].c_str());
            return HP_ERR;
        }
        if (!is_hexnum(addr[0]) || !is_hexnum(addr[1])) {
            ELOG_ERROR("addr[%s] parse fail.", map[0].c_str());
            return HP_ERR;
        }
        if (!is_hexnum(map[2])) {
            ELOG_ERROR("offset[%s] parse fail.", map[2].c_str());
            return HP_ERR;
        }
        map_addr addr_num(std::stoull(addr[0], nullptr, 16), std::stoull(addr[1], nullptr, 16), std::stoull(map[2], nullptr, 16));
        auto it = maps.find(map[5]);
        if (it == maps.end()) {
            hp_map m;
            m.path = map[5];
            m.attr = get_map_type(m.path);
            m.addr[get_elf_type(map[1])] = move(addr_num);
            maps[m.path] = move(m);
        } else {
            it->second.addr[get_elf_type(map[1])] = move(addr_num);
        }
        if (maps.size() == 1 && is_file_exsit(map[5]) && !is_find(map[5], ".so")) {
            target = map[5];
            maps[map[5]].addr[elf_type::CODE].start_addr = 0; // 进程自身起始地址为0
        }
    }
    return 0;
}


int hot_patch::load_lib(const hp_patch& patch)
{
    string libpath = get_realpath(patch.patch);
    if (is_loaded(libpath)) {
        ELOG_NOTICE("patch[%s] have load to proccess[%u].", libpath.c_str(), m_pid);
        return 0;
    }   

    injector_t *injector;
    if (injector_attach(&injector, m_pid) != 0) {
        ELOG_ERROR("injector_attach err: %s", injector_error());
        return HP_ERR;
    }
    if (injector_inject(injector, libpath.c_str(), NULL) == 0) {
        ELOG_NOTICE("\"%s\" successfully injected.", libpath.c_str());
    } else {
        ELOG_ERROR("could not inject \"%s\"", libpath.c_str());
        ELOG_ERROR("%s\n", injector_error());
    }
    injector_detach(injector);
    return 0;
}


int hot_patch::get_dirs(const string &path, vector<string> &dirs)
{
    DIR * dir;
    struct dirent * ptr;

    dir = opendir(path.c_str()); //打开一个目录
    if (!dir) {
        ELOG_ERROR("opendir [%s] fail.", path.c_str());
        return -1;
    }

    while((ptr = readdir(dir)) != NULL) //循环读取目录数据
    {
        if (ptr->d_type != DT_DIR) {
            continue;
        }

        if (dirs.size() > 1000) 
            break;
        dirs.emplace_back(ptr->d_name);
    }
    if (dir) {
        closedir(dir);//关闭目录指针
    }
    return 0;
}

string hot_patch::get_realpath(const string &path)
{
    char abspath[PATH_MAX];
    string rpath;

    if (realpath(path.c_str(), abspath) == NULL) {
        ELOG_ERROR("failed to get the full path[%s]", path.c_str());
        return rpath;
    }
    rpath = string(abspath);
    return rpath;
}

int hot_patch::attach_get_stack(const std::vector<uint32_t> &pids, type_stacks &stacks)
{
    ptrace_attach at(pids);
    for (const uint32_t &pid : pids) {
        vector<hp_frame> f;
        if (get_stack(pid, f) != 0 ) {
            ELOG_ERROR("get stack from pid [%u] fail, then continue.", pid);
            continue;
        }
        stacks[pid] = move(f);
    }
    return 0;
}

int hot_patch::attach_get_stack(const uint32_t &pid, vector<hp_frame> &stack)
{
    ptrace_attach at(pid);
    return get_stack(pid, stack);
}

int hot_patch::get_stack(const uint32_t &pid, vector<hp_frame> &stack)
{
    int ret = 0;
    unw_cursor_t cursor = {};
    unw_addr_space_t as = 0;

    if (pid == 0) {
        return HP_ERR;
    }

    void *context = _UPT_create(pid);
    if (!context) {
        ELOG_ERROR("_UPT_create pid[%u] fail.", pid);
        ret = HP_ERR;
        goto end;
    }
    as = unw_create_addr_space(&_UPT_accessors, 0);
    if (!as) {
        ELOG_ERROR("unw_create_addr_space with pid[%u] fail.", pid);
        ret = HP_ERR;
        goto end;
    }
    if (unw_init_remote(&cursor, as, context) != 0) {
        ELOG_ERROR("cannot initialize cursor for remote unwinding with pid[%u].", pid);
        ret = HP_ERR;
        goto end;
    }

    do {
        unw_word_t offset = 0, pc = 0;
        char sym[4096] = {0};
        if (unw_get_reg(&cursor, UNW_REG_IP, &pc) != 0) {
            ELOG_ERROR("can't find pc addr.");
        }
        if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) != 0){
            ELOG_ERROR("can't find symbol name.");
            sprintf(sym, "unkown_func");
        }
        stack.emplace_back(hp_frame(pc, sym, offset));
    } while (unw_step(&cursor) > 0);
end:
    if (as) {
        unw_destroy_addr_space(as);
    }
    if (context) {
        _UPT_destroy(context);
    }
    return ret;
}

int hot_patch::bytecode_write(const size_t &addr, const vector<uint8_t> &data)
{
    char word[sizeof(long)] = {};
    size_t offset = 0;
    size_t i = 0;
    long ret;
    for (const auto &c: data) {
        word[i] = c;
        i++;
        if (i < sizeof(long)) {
            continue;
        }
        ret = ptrace(PTRACE_POKETEXT, m_pid, (addr + offset), *(long*)word);
        HP_ASSERT(ret == 0);
        offset += sizeof(long);
        i = 0;
    }
    if (i != 0) {
        long read_w = ptrace(PTRACE_PEEKTEXT, m_pid, (addr + offset), 0);
        HP_ASSERT(read_w != -1);
        char *dest = (char*)&read_w;
        for (; i < sizeof(long); i++) {
            word[i] = *(dest+i);
        }
        ret = ptrace(PTRACE_POKETEXT, m_pid, (addr + offset), *(long*)word);
        HP_ASSERT(ret == 0);
    }
    return 0;
}

int hot_patch::bytecode_read(const size_t &addr, const uint32_t &size, std::vector<uint8_t> &data)
{
    long word;
    uint32_t word_num = size/sizeof(long);
    for (uint32_t i = 0; i < word_num; i++) {
        word = ptrace(PTRACE_PEEKTEXT, m_pid, addr + i*sizeof(long), 0);
        HP_ASSERT(word != -1);
        char *src = (char *)&word;
        for (uint32_t j = 0; j < sizeof(long); j++) {
            data.push_back(*(src+j));
        }
    }
    if (size%sizeof(long) > 0) {
        word = ptrace(PTRACE_PEEKTEXT, m_pid, addr + word_num*sizeof(long), 0);
        HP_ASSERT(word != -1);
        char *src = (char *)&word;
        for (uint32_t j = 0; j < size%sizeof(long); j++) {
            data.push_back(*(src+j));
        }
    }
    return 0;
}