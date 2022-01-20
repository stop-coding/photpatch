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

#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "cmdbox.h"
#include <iostream>

using namespace ns_cmd;
using namespace std;

static bool isstop = false;
void SignalFunc(int sig_num)
{
	switch (sig_num)
	{
	case SIGINT:
		isstop = true;
		exit(0);
		break;
	case SIGQUIT:
		isstop = true;
		break;
	case SIGHUP:
		break;
	default:
		break;
	}
	cout<<sig_num<<endl;
	return;
}
/*---------------------------------------------------------------------------*/
/* main									     */
/*---------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	cmdbox mytest;
	signal(SIGINT, SignalFunc);//注册信号
	signal(SIGHUP, SignalFunc); 
	signal(SIGQUIT, SignalFunc);
	mytest.dispatch<cmd_hp>(argc, argv);//接收命令
	
	return 0;
}

