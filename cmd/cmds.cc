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
#include <queue>
#include <thread>

#include "cmds.h"
#include "hotpatch.h"

using namespace std;
using namespace ns_cmd;
using namespace ns_patch;


int cmd_hp::do_run(const cmdline::parser &opts) const
{
    if (!opts.exist(YAML_FILE_PATH)) {
        cerr<<"yaml file empty."<<endl;
        return -1;
    }

    if (!opts.exist(PROCESS_ID)) {
        cerr<<"pid empty."<<endl;
        return -1;
    }

    hot_patch mypatch(opts.get<int>(PROCESS_ID), opts.get<string>(YAML_FILE_PATH));
    if (mypatch.init() != 0) {
        cerr<<" init patch fail."<<endl;
        return -1;
    }
    return 0;
}