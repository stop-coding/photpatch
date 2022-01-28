#include <limits.h>
#include <dirent.h>
#include <iostream>
#include <fstream>
#include <libunwind-ptrace.h>
#include <stdlib.h>
#include <unistd.h>

#include "injector.h"
#include "hp_elf.h"
#include "hotpatch.h"
#include "elog.h"
#include "util.h"
#include "config.h"


using namespace std;
using namespace ns_patch;
using namespace YAML;

const static int MAX_RETRY_TIMES = 3;
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
        ELOG_ERROR("try patch fail, wait (%u ms) then retry!", RETRY_INTERVAL_TIME_MS);
        usleep(RETRY_INTERVAL_TIME_MS * 1000);
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
    if (m_proc.maps.find(m_proc.target) == m_proc.maps.end()) {
        ELOG_ERROR("invalid target[%s] that to patch.", m_proc.target.c_str());
        return HP_ERR;
    }
    auto &target_map = m_proc.maps[m_proc.target];
    hp_elf target_elf(m_proc.target);
    if (target_elf.is_open() == false) {
        ELOG_ERROR("parse target[%s] elf fail.", m_proc.target.c_str());
        return HP_ERR;
    }
    ptrace_attach pt(m_proc.pids);
    for (const auto &patch : m_cfg->get().patch) {
        auto it = m_proc.maps.find(patch.path);
        if (it == m_proc.maps.end()) {
            ELOG_ERROR("patch[%s] have not loaded.", patch.path.c_str());
            continue;
        }
        auto &patch_map = it->second;
        hp_elf p_elf(patch.path);
        if (target_elf.is_open() == false) {
            ELOG_ERROR("parse patch[%s] elf fail.", patch.path.c_str());
            continue;
        }
        // check name
        vector<hp_function> replaced;
        for (const auto &func : patch.funcs) {
            if (!target_elf.is_func_exist(func.new_name) || !p_elf.is_func_exist(func.new_name)) {
                ELOG_ERROR("invalid function[%s] that to patch.", func.new_name.c_str());
                ret = HP_ERR;
                break;
            }
            hp_function old_func;
            old_func.start = 0;
            old_func.offset = target_elf.get_offset(func.new_name);
            old_func.size = target_elf.get_size(func.new_name);

            hp_function new_func;
            new_func.start = patch_map.addr[elf_type::CODE].start_addr;
            new_func.offset = p_elf.get_offset(func.new_name);
            new_func.size = p_elf.get_size(func.new_name);

            ret = replace(new_func, old_func);
            if (ret) {
                ELOG_ERROR("invalid target[%s] that to patch.", m_proc.target.c_str());
                break;
            }
            replaced.emplace_back(old_func);
        }
        if (ret) {
            for (const hp_function &f: replaced) {
                // todo restore
                ELOG_ERROR("Restore Func -- start:0x%lx, offset:0x%lx, size:0x%lx", f.start, f.offset, f.size);
            }
            break;
        }
    }
    return ret;
}

int hot_patch::replace(const hp_function &new_func, const hp_function &old_func)
{
    vector<uint8_t> data;
    ELOG_NOTICE("New Func -- start:0x%lx, offset:0x%lx, size:0x%lx", new_func.start, new_func.offset, new_func.size);
    int ret = text_read(new_func.start + new_func.offset, new_func.size, data);
    HP_ASSERT(ret == 0);
    for (const auto &c: data) {
        printf("%x ", c);
    }
    printf("\n\n");
    ELOG_NOTICE("Old Func -- start:0x%lx, offset:0x%lx, size:0x%lx", old_func.start, old_func.offset, old_func.size);
    ret = text_read(old_func.start + old_func.offset, old_func.size, data);
    HP_ASSERT(ret == 0);
    for (const auto &c: data) {
        printf("%x ", c);
    }
    printf("\n\n");
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
            ELOG_ERROR("load lib[%s] fail.", patch.path.c_str());
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
        }
    }
    return 0;
}


int hot_patch::load_lib(const hp_patch& patch)
{
    char abspath[PATH_MAX];

    if (realpath(patch.path.c_str(), abspath) == NULL) {
        ELOG_ERROR("failed to get the full path[%s]", patch.path.c_str());
        return HP_ERR;
    }
    string libpath = string(abspath);
    if (is_loaded(libpath)) {
        ELOG_NOTICE("patch[%s] have load to proccess[%u].", libpath.c_str(), m_pid);
        return 0;
    }   

    injector_t *injector;
    if (injector_attach(&injector, m_pid) != 0) {
        ELOG_ERROR("%s\n", injector_error());
        return HP_ERR;
    }
    if (injector_inject(injector, libpath.c_str(), NULL) == 0) {
        ELOG_NOTICE("\"%s\" successfully injected\n", libpath.c_str());
    } else {
        ELOG_ERROR("could not inject \"%s\"\n", libpath.c_str());
        ELOG_ERROR("  %s\n", injector_error());
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

int hot_patch::text_write(const size_t &addr, const vector<uint8_t> &data)
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
        ret = ptrace(PTRACE_POKETEXT, m_pid, (addr + offset), word);
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
        ret = ptrace(PTRACE_POKETEXT, m_pid, (addr + offset), word);
        HP_ASSERT(ret == 0);
    }
    return 0;
}

int hot_patch::text_read(const size_t &addr, const uint32_t &size, std::vector<uint8_t> &data)
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