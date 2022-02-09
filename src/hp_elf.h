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
#include <stdlib.h>
#include <unistd.h>
#include <cstdlib>
#include <string>
#include <map>
#include <elf.h>
#pragma once

namespace ns_patch{

#ifdef __LP64__
#define Elf_Ehdr Elf64_Ehdr
#define Elf_Shdr Elf64_Shdr
#define Elf_Sym  Elf64_Sym
#define Elf_Word Elf64_Word
#else
#define Elf_Ehdr Elf32_Ehdr
#define Elf_Shdr Elf32_Shdr
#define Elf_Sym Elf32_Sym
#define Elf_Word Elf32_Word
#endif

enum class sec_type
{
    dynstr = 0,
    dynsym = 1,
};

struct hp_elf_msg
{
    Elf_Ehdr head;
    Elf_Shdr shstrtab;
    std::map<sec_type, Elf_Shdr> sections;
    std::map<std::string, Elf_Sym> symtables;
};

class hp_elf{
public:
    hp_elf(const std::string &elf_file);
    hp_elf()=delete;
    ~hp_elf();
    bool load(const std::string &elf_file);
    bool is_open(){return m_is_ok;};
    bool is_func_exist(const std::string &func_name);
    std::string get_func_name(const size_t &offset);
    size_t get_offset(const std::string &func_name);
    size_t get_size(const std::string &func_name);
    const std::string &get_file() const{return m_file;};
private:
    FILE *m_fp = nullptr;
    hp_elf_msg m_msg;
    bool m_is_ok;
    std::string m_file;
private:
    int get_elf_section(const sec_type &type, Elf_Shdr &shdr);
    int get_elf_symtable(const std::string &func_name, Elf_Sym &sym);
    int get_elf_symtable(const size_t &address, std::string &name, Elf_Sym &sym);
    int get_elf_func_name(const size_t &st_offset, std::string &name);
    int parse();
};
}