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

int hot_patch::init()
{
    if (m_pid == 0) {
        ELOG_ERROR("target pid [%u] fail.", m_pid);
        return HP_ERR;
    }
    std::unique_ptr<ns_patch::config> pcfg(new config(m_yaml));
    if (pcfg->load() != 0) {
        ELOG_ERROR("load yaml file[%s] fail.", m_yaml.c_str());
        return HP_ERR;
    }
    m_cfg = std::move(pcfg);
    if (get_process_msg(m_pid, m_proc) != 0) {
        ELOG_ERROR("find pid[%u] fail.", m_pid);
        return HP_ERR;
    }   
    type_stacks stacks;
    attach_get_stack(m_proc.pids, stacks);
   /*  for (const auto &s : stacks) {
        ELOG_NOTICE("get stack: %u", s.first);
        for (hp_frame const &f : s.second) {
            ELOG_NOTICE("0x%lx : %s + 0x%lx.", f.pc, f.func_name.c_str(), f.func_offset);
        }
        //break;
    } */
    
    for (const auto &libpath : m_cfg->get().patch) {
        if (load_lib(libpath) != 0) {
            abort();
        }
    }

    for (const auto &m : m_proc.maps) {
        const auto &map = m.second;
        //ELOG_NOTICE("map: 0x%lx - 0x%lx 0x%lx %s", map.start_addr, map.end_addr, map.file_offset, map.path.c_str());
        hp_elf elf(map.path);
        if (elf.is_open()) {
            size_t offset = elf.get_offset("helloworld");
            if (offset > 0 ) {
                ELOG_NOTICE("find func[helloworld] on file[%s] offset[%ld]", map.path.c_str(), offset);
            } else {
                ELOG_ERROR("find func[helloworld] on file[%s] fail.", map.path.c_str());
            }
        }
    }

    for (const auto &s : m_proc.status) {
        ELOG_NOTICE("%s = %s ", s.first.c_str(), s.second.c_str());
    }

    return 0;
}

int hot_patch::get_process_msg(const uint32_t &main_pid, hp_process &proc)
{
    int ret = 0;

    ret = get_status(main_pid, proc.status);
    if (ret) {
        ELOG_ERROR("get status with pid[%u] fail.", main_pid);
        return HP_ERR;
    }

    ret = get_maps(main_pid, proc.maps);
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
    return map_type::EXE;
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


int hot_patch::get_maps(const uint32_t &pid, map<string, hp_map> &maps)
{
    string maps_path = "/proc/" + to_string(pid) + "/maps";
    ifstream proc(maps_path, ios::in);
    if (!proc) {
        ELOG_ERROR("open maps[%s] fail, maybe process exit?.", maps_path.c_str());
        return -1;
    }
    string line;
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

        hp_map m;
        m.start_addr = std::stoull(addr[0], nullptr, 16);
        m.end_addr = std::stoull(addr[1], nullptr, 16);
        m.file_offset = std::stoull(map[2], nullptr, 16);
        m.path = map[5];
        m.attr = map[1];
        maps[map[5]] = move(m);
    }
    return 0;
}


int hot_patch::load_lib(const hp_patch& patch)
{
    char abspath[PATH_MAX];

    if (realpath(patch.path.c_str(), abspath) == NULL) {
        elog().error("failed to get the full path of: " + patch.path);
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
        return 1;
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
