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
#include <map>
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
	EXE = 0,
	LIB = 1,
	HEAP = 2,
	STACK = 3,
	OTHER = 4,
};

enum class elf_type
{
	CODE = 0, //代码段
	DATA = 1, //数据段
	VALUE = 2, // 变量段
	UNKNOWN = 100,
};

struct map_addr
{
	map_addr(const uint64_t &s,const uint64_t &e,const uint64_t &o):
	start_addr(s),end_addr(e),file_offset(o){};
	map_addr(){};
	uint64_t start_addr = 0;
	uint64_t end_addr = 0;
	uint64_t file_offset = 0;
};

struct hp_function
{
	uint64_t start = 0;  	// 函数虚拟内存起始地址
	uint64_t offset = 0;	// 偏量
	uint64_t size = 0;		// 函数体大小
};

struct hp_backup
{
	std::string name;
	uint64_t addr = 0;	
	uint64_t size = 0;
	std::vector<uint8_t> byte_codes;
};

struct hp_map
{
	std::string path;	// 
	map_type attr; //
	std::map<elf_type, map_addr> addr;
};

using type_kv = std::map<std::string, std::string>;
struct hp_process
{
	uint32_t mpid = 0;
	std::string target;
	type_kv status;
	std::vector<uint32_t> pids;
	std::map<std::string, hp_map> maps;
};


//补丁管理器
using type_stacks = std::map<uint32_t,std::vector<hp_frame>>;
class hot_patch{
public:
	hot_patch(const uint32_t &pid, const std::string &yaml){
		m_pid = pid;
		if (init(pid, yaml)) {
			m_pid = 0;
		}
	};
	hot_patch() = delete; //禁止无参数初始化
	~hot_patch(){};
	int patch();
private:
	std::unique_ptr<ns_patch::config> m_cfg;
	uint32_t m_pid = 0;
	hp_process m_proc;
private:
	int init(const uint32_t &pid, const std::string &yaml);
	int try_patch();
	int load_proc(const uint32_t &main_pid, hp_process &proc);
	int load_lib(const YAML::hp_patch& patch);
	int get_stack(const uint32_t &pid, std::vector<hp_frame> &stack);
	int attach_get_stack(const uint32_t &pid, std::vector<hp_frame> &stack);
	int attach_get_stack(const std::vector<uint32_t> &pids, type_stacks &stacks);
	int get_pids(const uint32_t &mpid, std::vector<uint32_t> &pids);
	int get_maps(const uint32_t &pid, std::map<std::string, hp_map> &maps, std::string &target);
	bool is_loaded(const std::string& libpath);
	int get_dirs(const std::string &path, std::vector<std::string> &dirs);
	int get_status(const uint32_t &pid, std::map<std::string, std::string> &status);
	map_type get_map_type(const std::string& map_str);
	elf_type get_elf_type(const std::string& str);
	int replace(const hp_function &new_func, const hp_function &old_func);
	int backup(const std::vector<hp_backup> &recover);
	int bytecode_write(const size_t &addr, const std::vector<uint8_t> &data);
	int bytecode_read(const size_t &addr, const uint32_t &size, std::vector<uint8_t> &data);
};

}

