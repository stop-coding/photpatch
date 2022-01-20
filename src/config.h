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

#include <string>
#include "patch_yaml_def.h"
#pragma once


namespace ns_patch{

class config{
public:
    config(const std::string &yaml):m_yaml(yaml){};
    config()=delete;
    ~config(){};

    config& operator=(config&& other)
    {
      if (this != &other){
         m_config = std::move(other.m_config);
         m_yaml = std::move(other.m_yaml);
      }
      return *this;
    }

    int load();
    void down(){};
    const YAML::hp_cfg &get(){return m_config;};
private:
    YAML::hp_cfg m_config;
    std::string m_yaml;
};
}