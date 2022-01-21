#include <limits.h>

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

    //ELOG_NOTICE("head:%s", m_cfg->get().head.version);
    //elog().error("test:" + m_cfg->get().head.version);
    for (auto it = m_cfg->get().patch.begin(); it != m_cfg->get().patch.end(); it++) {
        if (load_lib(*it) != 0) {
            abort();
        }
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