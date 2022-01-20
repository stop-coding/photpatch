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

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <cstdlib>

#include "config.h"

namespace ns_patch{

//补丁管理器
class hot_patch{
public:
	hot_patch(u_int32_t pid, const std::string &yaml):m_pid(pid), m_yaml(yaml){
		
	};
	hot_patch() = delete; //禁止无参数初始化
	~hot_patch(){};
	int init();
	int load_patch_to_target(){};
	int get_patch_message(){};
	int get_target_message(){};
	int active_patch(){};
	int deactive_patch(){};
private:
	std::unique_ptr<ns_patch::config> m_cfg;
	u_int32_t m_pid;
	std::string m_yaml;
private:
	int attach();
	int detach();
};

}

