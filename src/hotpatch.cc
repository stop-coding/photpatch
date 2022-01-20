
#include "hotpatch.h"
#include "elog.h"
#include "config.h"

using namespace std;
using namespace ns_patch;

int hot_patch::init()
{
    std::unique_ptr<ns_patch::config> pcfg(new config(m_yaml));
    if (pcfg->load() != 0) {
        ELOG_ERROR("load yaml file[%s] fail.", m_yaml.c_str());
        return HP_ERR;
    }
    m_cfg = std::move(pcfg);

    //ELOG_NOTICE("head:%s", m_cfg->get().head.version);
    elog().error("test:" + m_cfg->get().head.version);
    return 0;
}