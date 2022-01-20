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
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>
#include <termios.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <queue>
#include <regex>

#include "cmdbox.h"

using namespace std;
using namespace ns_cmd;

cmdbox::~cmdbox()
{
    for(auto iter=m_func.begin(); iter!=m_func.end(); iter++){
        delete iter->second;
    }
}

void cmdbox::dispatch() {
    int ret = 0;
    ns_cmd::terminal cur_tty;
    do {
        string options;
        ret = cur_tty.getopts(options);
        if (ret < 0 ) {
            break;
        }
        if (m_opts.parse(options) == false) {
            cerr<<m_opts.error()<<endl<<m_opts.usage()<<endl;
            continue;
        }
        if (!m_opts.exist(MASTER_CMD)) {
            continue;
        }
        std::string cmd = m_opts.get<std::string>(MASTER_CMD);
        if (cmd == "exit") {
            break;
        } else if (cmd == "help") {
            for(auto it = m_func.begin(); it != m_func.end(); it++) {
                it->second->usage();
            }
        } else {
            ;
        }
        auto iter = m_func.find(m_opts.get<string>(PROCESS_ID));
        if (iter != m_func.end()){
            ret = iter->second->run(options);
        }
        if (ret < 0) {
            break;
        }
    }while(true);
}

// terminal

static int getch() {
    struct termios tm, tm_old;
    int fd =0, c;
 
    if (tcgetattr(fd, &tm) < 0) {
        return -1;
    }
 
    tm_old = tm;    // save original mode
    cfmakeraw(&tm);
 
    if (tcsetattr(fd, TCSANOW, &tm) < 0) {    // set new mode
        return -1;
    }
 
    c = fgetc(stdin);
 
    if (tcsetattr(fd, TCSANOW, &tm_old) < 0) {   // set old mode
        return -1;
    }
 
    return c;
}

static void locateCursor(const int row, const int col){
    printf("%c[%d;%dH",27,row,col);
}

static void left_shift_cursor(const int n)
{
    printf("\033[%dD",n);
}

static void right_shift_cursor(const int n)
{
    printf("\033[%dC",n);
}
static void clean_cursor_ctx()
{
    printf("\033[K");
}

static void light_cursor()
{
    printf("\033[8m");
}

static bool is_delete(char ch)
{
    return (ch == 8 || ch == 126)?true:false;
}

static bool is_find_history(char ch)
{
    return (ch == 'A' || ch == 'B' || ch == 'D' || ch == 'C')?true:false;
}

static bool is_exit(char cmd)
{
    return (cmd == 3 || cmd == 17)?true:false;
}

static bool is_search(char cmd)
{
    return (cmd == '\t')?true:false;
}

static bool is_exec(char cmd)
{
    return (cmd == '\n' || cmd == 13)?true:false;
}

static bool is_special_key(const string &precmd)
{
    return (precmd.size() == 2 && precmd[0] == 27 && precmd[1] == 91)?true:false;
}

static bool is_valid_char(char ch)
{
    return (isalnum(ch) || ch == '/' || ch == ' ' || ch == '_' || ch == '.'|| ch == '-')?true:false;
}

/* static vector<string> split(const string& in, const string& delim)
{
    vector<string> ret;
    try
    {
        regex re{delim};
        return vector<string>{
                sregex_token_iterator(in.begin(), in.end(), re, -1),
                sregex_token_iterator()
           };      
    }
    catch(const std::exception& e)
    {
        cout<<"error:"<<e.what()<<std::endl;
    }
    return ret;
} */

void terminal::show(const std::string &in)
{
    ;
}

int terminal::getopts(string &arg)
{
    string str_in;
    string pre_in;
    u_int32_t cmd_index = 0;
    u_int32_t in_index = 0;
    char ch;
    cout<<m_head;
    while (!m_is_stop) {
        ch = getch();
        if (is_search(ch)){
            cout<<endl;
            show(str_in);
            cout<<m_head<<str_in;
            continue;
        }
        else if (is_exec(ch)){
            cout<<endl;
            if(str_in.size()){
                m_history.push_back(str_in);
                cmd_index = m_history.size();
                arg = str_in;
                return 0;
            }
            cout<<m_head;
            continue;
        }
        else if(is_exit(ch)){ //退出
            cout<<endl;
            cout<<endl;
            cout<<"### thank you for using cafs tools, Good Bye!!! ###"<<endl;
            cout<<endl;
            break;
        }else if(is_delete(ch)){ //删除
            if(str_in.size()){
                if (in_index >= str_in.size()) {
                    str_in.pop_back();
                    left_shift_cursor(1);
                    clean_cursor_ctx();
                    in_index = str_in.size();
                }else if(in_index > 0) {
                    left_shift_cursor(in_index);
                    clean_cursor_ctx();
                    str_in.erase(in_index - 1, 1);
                    cout<<str_in;
                    in_index--;
                    if(str_in.size() >= in_index){
                        left_shift_cursor(str_in.size() - in_index);
                    }else{
                        left_shift_cursor(str_in.size());
                    }
                }
            }
            continue;
        }else if(is_find_history(ch)){ //历史记录
            if (is_special_key(pre_in)){
                if(ch == 'A' && cmd_index){
                    if(str_in.length()){
                        left_shift_cursor(str_in.length());
                        clean_cursor_ctx();
                    }
                    cmd_index--;
                    str_in = m_history[cmd_index];
                    in_index = str_in.size();
                    cout<<str_in;
                }else if(ch == 'B' && cmd_index < m_history.size()){
                    if(str_in.length()){
                        left_shift_cursor(str_in.length());
                        clean_cursor_ctx();
                    }
                    str_in = m_history[cmd_index];
                    in_index = str_in.size();
                    cmd_index++;
                    cout<<str_in;
                }else if(ch == 'D' && in_index > 0){
                    left_shift_cursor(1);
                    in_index--;
                }else if(ch == 'C' && in_index < str_in.size()){
                    right_shift_cursor(1);
                    in_index++;
                }else if(ch == 'A' || ch == 'B'){
                    str_in = "";
                    cout<<endl;
                    cout<<m_head;
                }
                pre_in.clear();
                continue;
            }else{
                cout<<"  find ABCD error"<<endl;
            }
        }

        //输入
        if(is_valid_char(ch)) {
            if(in_index >= str_in.size()){
                str_in +=ch;
                in_index = str_in.size();
                cout<<ch;
            }else{
                if(in_index > 0){
                    left_shift_cursor(in_index);
                }
                clean_cursor_ctx();
                str_in.insert(in_index, 1, ch);
                cout<<str_in;
                in_index++;
                if(str_in.size() > in_index){
                    left_shift_cursor(str_in.size() - in_index);
                }
            }
        }else{
            pre_in +=ch;
            if(pre_in.size() > 2){ // 保留两个输入
                pre_in = pre_in.erase(0, pre_in.size() - 2);
            }
        }
    }
    cout<<m_head<<"exit"<<endl;
    return -1;
}