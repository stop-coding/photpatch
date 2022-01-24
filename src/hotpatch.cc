#include <limits.h>
#include <dirent.h>
#include <iostream>
#include <fstream>
#include <libunwind-ptrace.h>
#include <stdlib.h>
#include <unistd.h>

#include "injector.h"

#include "hotpatch.h"
#include "elog.h"
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
    get_threadids();
    for (uint32_t const &id : m_tpids) {
        ELOG_NOTICE("%u", id);
        vector<hp_frame> stack;
        if (get_stack(id, stack) != 0) {
            continue;
        }
        for (hp_frame const &f : stack) {
            ELOG_NOTICE("0x%lx : %s + 0x%lx.", f.pc, f.func_name.c_str(), f.func_offset);
        }
        //break;
    }
    //ELOG_NOTICE("head:%s", m_cfg->get().head.version);
    //elog().error("test:" + m_cfg->get().head.version);
    for (auto it = m_cfg->get().patch.begin(); it != m_cfg->get().patch.end(); it++) {
        if (load_lib(*it) != 0) {
            abort();
        }
    }
    return 0;
}

bool hot_patch::is_loaded(const string& libpath)
{
    string maps_path = "/proc/" + to_string(m_pid) + "/maps";
    ifstream maps(maps_path, ios::in);
    if (!maps) {
        ELOG_ERROR("open maps[%s] fail, maybe process exit?.", maps_path.c_str());
        return false;
    }
    string line;
    while( maps >> line ) {    
        if (line.find(libpath) >= 0) {
            return true;
        }
    }
    return false;
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

bool hot_patch::is_number(const string& str)
{
    for (char const &c : str) {
        if (std::isdigit(c) == 0) return false;
    }
    return true;
}

int hot_patch::get_threadids()
{
    DIR * dir;
    struct dirent * ptr;

    string task_path = "/proc/" + to_string(m_pid) + "/task";
    dir = opendir(task_path.c_str()); //打开一个目录
    if (!dir) {
        ELOG_ERROR("opendir [%s] fail.", task_path.c_str());
        return -1;
    }

    if (!m_tpids.empty()) {
        m_tpids.clear();
    }

    while((ptr = readdir(dir)) != NULL) //循环读取目录数据
    {
        if (ptr->d_type != DT_DIR) {
            continue;
        }

        if (!is_number(ptr->d_name)) {
            continue;
        }
        if (m_tpids.size() > 1000) 
            break;
        m_tpids.emplace_back(atoi(ptr->d_name));
    }
    if (dir) {
        closedir(dir);//关闭目录指针
    }
    return 0;
}

int hot_patch::get_stack(const uint32_t &pid, vector<hp_frame> &stack)
{
    int ret = 0;
    bool have_attach = false;
    unw_cursor_t cursor = {};
    unw_addr_space_t as = 0;

    if (pid == 0) {
        return HP_ERR;
    }
    if (ptrace(PTRACE_ATTACH, pid, 0, 0) == 0){
        have_attach = true;
        ELOG_NOTICE("attach pid[%u] success!", pid);
        wait(NULL);
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
    if (have_attach) {
        (void) ptrace(PTRACE_DETACH, pid, 0, 0);
    }
    return 0;
}