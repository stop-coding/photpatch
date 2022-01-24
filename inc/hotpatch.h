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
#include <sys/ptrace.h>
#include <sys/wait.h>

#include "config.h"


namespace ns_patch{

struct hp_frame
{
	hp_frame(uint64_t pc, const std::string &name, uint64_t offset):pc(pc),
	func_name(name),func_offset(offset){};
	hp_frame():hp_frame(0, "none", 0){};
	uint64_t pc;
	std::string func_name;
	uint64_t func_offset;
};

class ptrace_attach
{
public:
	ptrace_attach(const uint32_t &id){
		attach(id);
	}
	ptrace_attach(const std::vector<uint32_t> &ids){
		for (const uint32_t &id : ids) {
			attach(id);
		}
	}
	ptrace_attach()=delete;
	~ptrace_attach(){
		for (const uint32_t &id : m_attach_ids) {
			(void) ptrace(PTRACE_DETACH, id, 0, 0);
		}
	}
private:
	std::vector<uint32_t> m_attach_ids;
	void attach(const uint32_t &pid){
		if (ptrace(PTRACE_ATTACH, pid, 0, 0) == 0){
			wait(NULL);
			m_attach_ids.emplace_back(pid);
		}
	}
};

//补丁管理器
class hot_patch{
public:
	hot_patch(uint32_t pid, const std::string &yaml):m_pid(pid), m_yaml(yaml){
		
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
	uint32_t m_pid;
	std::string m_yaml;
	std::vector<uint32_t> m_tpids;
private:
	int attach();
	int detach();
	int load_lib(const YAML::hp_patch& patch);
	int get_stack(const uint32_t &pid, std::vector<hp_frame> &stack);
	int get_threadids();
	bool is_number(const std::string& str);
	bool is_loaded(const std::string& libpath);
};

}

