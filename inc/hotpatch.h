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
	hp_frame()=delete;
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

enum class map_type
{
	EXE,
	LIB,
	HEAP,
	STACK,
	OTHER,
};

struct hp_map
{
	std::string path;	// 
	std::string attr; // rwxp
	uint64_t start_addr;
	uint64_t end_addr;
	uint64_t file_offset;

};

using type_kv = std::map<std::string, std::string>;
struct hp_process
{
	uint32_t mpid;
	type_kv status;
	std::vector<uint32_t> pids;
	std::map<std::string, hp_map> maps;
};


//补丁管理器
using type_stacks = std::map<uint32_t,std::vector<hp_frame>>;
class hot_patch{
public:
	hot_patch(uint32_t pid, const std::string &yaml):m_pid(pid), m_yaml(yaml){
		
	};
	hot_patch() = delete; //禁止无参数初始化
	~hot_patch(){};
	int init();
private:
	std::unique_ptr<ns_patch::config> m_cfg;
	uint32_t m_pid;
	std::string m_yaml;
	std::vector<uint32_t> m_tpids;
	hp_process m_proc;
private:
	int get_process_msg(const uint32_t &main_pid, hp_process &proc);
	int load_lib(const YAML::hp_patch& patch);
	int get_stack(const uint32_t &pid, std::vector<hp_frame> &stack);
	int attach_get_stack(const uint32_t &pid, std::vector<hp_frame> &stack);
	int attach_get_stack(const std::vector<uint32_t> &pids, type_stacks &stacks);
	int get_pids(const uint32_t &mpid, std::vector<uint32_t> &pids);
	int get_maps(const uint32_t &pid, std::map<std::string, hp_map> &maps);
	bool is_loaded(const std::string& libpath);
	int get_dirs(const std::string &path, std::vector<std::string> &dirs);
	int get_status(const uint32_t &pid, std::map<std::string, std::string> &status);
	map_type get_map_type(const std::string& map_str);
};

}

