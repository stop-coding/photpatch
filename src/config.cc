/*
 * Copyright(C)  All rights reserved.
 */

/*!
* \file 
* \brief 
* 
* 包含..
*
* \copyright All rights reserved.
* \author hongchunhua
* \version v1.0.0
* \note none 
*/

#include <sys/types.h>    
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <thread>

#include "elog.h"
#include "config.h"

using namespace std;
using namespace ns_patch;
using namespace YAML;

// 配置文件格式
int config::load()
{
    if (m_yaml.size() < 2) {
        ELOG_ERROR("yaml file invalid");
        return -1;
    }
    try {
        Node tmp = LoadFile(m_yaml);
        if(!tmp.IsMap()) {
            ELOG_ERROR("Parser yaml file[%s] invalid: %s", m_yaml.c_str());
            return HP_ERR;
        }
        m_config = tmp.as<YAML::hp_cfg>();
    } catch(Exception& e) {
        std::cout << e.what() << "\n";
        ELOG_ERROR("Parser yaml file[%s] error: %s", m_yaml.c_str(), e.what());
        return -1;
    }
    return 0;
}


