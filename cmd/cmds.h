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

#include <functional>
#include <iostream>
#include <vector>
#include <regex>

#include "cmdline.h"

namespace ns_cmd{

//cmd基类
class cmd_base{
public:
    virtual ~cmd_base(){}
	virtual const std::string &name() const=0;
    virtual void usage() const=0;
	int run(int argc, char *argv[]){
		if (m_opts.parse(argc, argv) == false) {
			std::cerr<<m_opts.error()<<std::endl<<m_opts.usage()<<std::endl;
			return -1;
		}
		return do_run(m_opts);
	};
	int run(const std::string &cmd){
		if (m_opts.parse(cmd) == false) {
			std::cerr<<m_opts.error()<<std::endl<<m_opts.usage()<<std::endl;
			return -1;
		}
		return do_run(m_opts);
	};
protected:
	virtual int do_run(const cmdline::parser &opts) const = 0;
	cmdline::parser m_opts;
};

static const char *PROCESS_ID = "pid";
static const char *YAML_FILE_PATH = "file";
static const char *RUN_ONECE = "once";
static const char *CMD_BOX_DEFAULT = "hotpatch";

class cmd_hp : public cmd_base{
public:
    cmd_hp(){
		m_is_runonce = true;
		m_opts.add<bool>(RUN_ONECE, 'o', "run once", false, true);
    	m_opts.add<int>(PROCESS_ID, 'p', "process pid", true);
		m_opts.add<std::string>(YAML_FILE_PATH, 'f', "the config file of yaml", true, "./conf/exmple-patch.yaml");
    	m_opts.add("help", 0, "help that show all cmd");
	};
	~cmd_hp(){};
	const std::string &name() const {return m_name;};
    void usage()const {};
protected:
    int do_run(const cmdline::parser &opts) const;
private:
    std::string m_yaml;
	bool m_is_runonce;
	const std::string m_name = "hotpatch";
};
}

