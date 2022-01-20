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
#include "cmds.h"
#include "cmdline.h"
#include <vector>

namespace ns_cmd{

static const char TTY_WELCOME[] = "### welcome to use terminal tools!!! ###\n";
static const char TTY_HEAD[] = "[tty@#]";
class terminal{
public:
	terminal(const std::string &head, const std::string &welcome):m_head(head),m_welcome(welcome),m_is_stop(false){
		std::cout<<std::endl<<std::endl<<m_welcome<<std::endl<<std::endl;
	};
	terminal():terminal(TTY_HEAD, TTY_WELCOME){};
	~terminal(){};
	int getopts(std::string &arg);
private:
	std::string m_head;
	std::string m_welcome;
	bool m_is_stop;
	std::vector<std::string>  m_history;
private:
	void show(const std::string &in);
	void parse(const std::string &in);
};



static const char MASTER_CMD[] = "cmd";

class cmdbox{
public:
	cmdbox(){

	};
	~cmdbox();
	template <class T>
	void dispatch(int argc, char *argv[]) {
		std::unique_ptr<cmd_base> cmd(new T());
		cmd->run(argc, argv);
	};
	void dispatch();
	template <class T>
	void add()
	{
		cmd_base *base = new T();
		m_func[base->name()] = base;
	}

private:
	cmdline::parser m_opts;
	std::map<std::string, cmd_base*> m_func;		
};

}

