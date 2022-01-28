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
#include <stdlib.h>
#include <type_traits>
#include <utility>
#include "hp_elf.h"
#include "elog.h"
#include "util.h"



using namespace std;
using namespace ns_patch;

static int read_elf_ehdr(FILE *fp, Elf_Ehdr *ehdr);
static int read_elf_shdr(FILE *fp, Elf_Shdr *shdr, size_t shdr_size);
static int read_elf_sym(FILE *fp, Elf_Sym *sym, size_t sym_size);
static size_t find_strtab_offset(FILE *fp, size_t offset, size_t size, const char *name);
struct elf_section_map
{
    Elf_Word type;
    char name[128];
};

static const elf_section_map G_SEC_MAP[] = 
{
    {SHT_STRTAB, ".dynstr"},
    {SHT_DYNSYM, ".dynsym"},
};

hp_elf::hp_elf(const std::string &elf_file)
{
    m_is_ok = false;
    m_fp = fopen(elf_file.c_str(), "r");
    if (!m_fp) {
        ELOG_ERROR("open elf file[%s] fail, check it.", elf_file.c_str());
        return;
    }
    if(parse() != 0) {
        return;
    }
    m_is_ok = true;
}

hp_elf::~hp_elf()
{
    if (m_fp) {
        fclose(m_fp);
    }
}

size_t hp_elf::get_offset(const std::string &func_name)
{
    Elf_Sym sym;
    if (get_elf_symtable(func_name, sym) != 0) {
        ELOG_ERROR("get elf symbol table[%s] fail.", func_name.c_str());
        return 0;
    }
    return (size_t)sym.st_value;
}

size_t hp_elf::get_size(const std::string &func_name)
{
    Elf_Sym sym;
    if (get_elf_symtable(func_name, sym) != 0) {
        ELOG_ERROR("get elf symbol table[%s] fail.", func_name.c_str());
        return 0;
    }
    return (size_t)sym.st_size;
}

bool hp_elf::is_func_exist(const std::string &func_name)
{
    Elf_Sym sym;
    if (get_elf_symtable(func_name, sym) != 0) {
        return false;
    }
    return true;
}

int hp_elf::get_elf_section(const sec_type &type, Elf_Shdr &shdr) 
{
    auto it = m_msg.sections.find(type);
    if (it != m_msg.sections.end()) {
        shdr = it->second;
        return 0;
    }
    if (!m_fp) {
        ELOG_ERROR("fd is null.");
        return HP_ERR;
    }
    auto &ehdr = m_msg.head;
    if (ehdr.e_shoff <= 0) {
        ELOG_ERROR("head offset[%ld] is invalid.", ehdr.e_shoff);
        return -1;
    }
    size_t shstrtab_offset = m_msg.shstrtab.sh_offset;
    fseek(m_fp, ehdr.e_shoff, SEEK_SET);//指针回位
    for (int i = 0; i < ehdr.e_shnum; i++) {
        fpos_t pos;
        char buf[64];
        int ret = read_elf_shdr(m_fp, &shdr, ehdr.e_shentsize);//指针会增加ehdr.e_shentsize
        if (ret != 0) {
            ELOG_ERROR("read_elf_shdr fail.");
            return ret;
        }
        if (shdr.sh_type != G_SEC_MAP[static_cast<Elf_Word>(type)].type) {
            continue;
        }
        fgetpos(m_fp, &pos);
        fseek(m_fp, shstrtab_offset + shdr.sh_name, SEEK_SET);
        fgets(buf, sizeof(buf), m_fp);
        fsetpos(m_fp, &pos);
        if (strcmp(buf, G_SEC_MAP[static_cast<Elf_Word>(type)].name) != 0) {
            continue;
        }
        m_msg.sections[type] = shdr;
        return 0;
    }
    return HP_ERR;
}

int hp_elf::parse() 
{
    const sec_type default_secs[] = {sec_type::dynstr, sec_type::dynsym};
    if (!m_fp) {
        ELOG_ERROR("fd is null.");
        return -1;
    }
    int ret = read_elf_ehdr(m_fp, &m_msg.head);
    if (ret) {
        ELOG_ERROR("read_elf_ehdr fail.");
        return -1;
    }
    auto &ehdr = m_msg.head;
    fseek(m_fp, ehdr.e_shoff + ehdr.e_shstrndx * ehdr.e_shentsize, SEEK_SET);
    ret = read_elf_shdr(m_fp, &m_msg.shstrtab, ehdr.e_shentsize);
    if (ret != 0) {
        ELOG_ERROR("read_elf_shdr of strtable fail.");
        return -1;
    }

    for (size_t i = 0; i < sizeof(default_secs)/sizeof(uint32_t); i++) {
        Elf_Shdr shdr;
        if (get_elf_section(default_secs[i], shdr) != 0) {
            ELOG_ERROR("get section[%d] fail.", default_secs[i]);
            return -1;
        }
        m_msg.sections[default_secs[i]] = shdr;
    }
    return 0;
}

int hp_elf::get_elf_symtable(const string &func_name, Elf_Sym &sym) 
{
    auto it = m_msg.symtables.find(func_name);
    if (it != m_msg.symtables.end()) {
        sym = it->second;
        return 0;
    }
    
    Elf_Shdr shdr;
    int ret = get_elf_section(sec_type::dynstr, shdr);
    if (ret) {
        ELOG_ERROR("get_elf_section fail with type [%d].", SHT_STRTAB);
        return -1;
    }
    size_t str_offset = shdr.sh_offset;
    size_t str_size = shdr.sh_size;
    if (!m_fp) {
        ELOG_ERROR("fd is null.");
        return -1;
    }

    size_t st_name = find_strtab_offset(m_fp, str_offset, str_size, func_name.c_str());
    if (st_name == 0) {
        ELOG_ERROR("failed to find %s in the .dynstr section.", func_name.c_str());
        return HP_ERR;
    }

    ret = get_elf_section(sec_type::dynsym, shdr);
    if (ret) {
        ELOG_ERROR("get_elf_section fail with type [%d].", SHT_DYNSYM);
        return -1;
    }
    size_t sym_offset = shdr.sh_offset;
    size_t sym_entsize = shdr.sh_entsize;
    size_t sym_num = 1;
    if (shdr.sh_entsize > 0 ) {
        sym_num = shdr.sh_size / shdr.sh_entsize;
    }
    fseek(m_fp, sym_offset, SEEK_SET);
    for (size_t i = 0; i < sym_num; i++) {
        ret = read_elf_sym(m_fp, &sym, sym_entsize);
        if (ret != 0) {
            ELOG_ERROR("read_elf_sym on offset [%ld] fail.", sym_entsize);
            return -1;
        }
        if (sym.st_name != st_name) {
            continue;
        }
        m_msg.symtables[func_name] = sym;
        return 0;
    }
    ELOG_ERROR("can't find symbol table with func name[%s].", func_name.c_str());
    return -1;
}

static int read_elf_ehdr(FILE *fp, Elf_Ehdr *ehdr)
{
    if (fread(ehdr, sizeof(*ehdr), 1, fp) != 1) {
        ELOG_ERROR("failed to read ELF header. (error: %s)", strerror(errno));
        return HP_ERR;
    }
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
        ELOG_ERROR("Invalid ELF header: 0x%02x,0x%02x,0x%02x,0x%02x",
                           ehdr->e_ident[0], ehdr->e_ident[1], ehdr->e_ident[2], ehdr->e_ident[3]);
        return HP_ERR;
    }
    switch (ehdr->e_ident[EI_CLASS]) {
    case ELFCLASS32:
#ifdef __LP64__
        {
            Elf32_Ehdr *ehdr32 = (Elf32_Ehdr *)ehdr;
            /* copy from last */
            ehdr->e_shstrndx = ehdr32->e_shstrndx;
            ehdr->e_shnum = ehdr32->e_shnum;
            ehdr->e_shentsize = ehdr32->e_shentsize;
            ehdr->e_phnum = ehdr32->e_phnum;
            ehdr->e_phentsize = ehdr32->e_phentsize;
            ehdr->e_ehsize = ehdr32->e_ehsize;
            ehdr->e_flags = ehdr32->e_flags;
            ehdr->e_shoff = ehdr32->e_shoff;
            ehdr->e_phoff = ehdr32->e_phoff;
            ehdr->e_entry = ehdr32->e_entry;
            ehdr->e_version = ehdr32->e_version;
            ehdr->e_machine = ehdr32->e_machine;
            ehdr->e_type = ehdr32->e_type;
        }
#endif
        break;
    case ELFCLASS64:
#ifndef __LP64__
        ELOG_ERROR("64-bit target process isn't supported by 32-bit process.");
        return HP_ERR;
#endif
        break;
    default:
        ELOG_ERROR("Invalid ELF class: 0x%x", ehdr->e_ident[EI_CLASS]);
        return HP_ERR;
    }
    return 0;
}

static int read_elf_shdr(FILE *fp, Elf_Shdr *shdr, size_t shdr_size)
{
    if (fread(shdr, shdr_size, 1, fp) != 1) {
        ELOG_ERROR("failed to read a section header. (error: %s)", strerror(errno));
        return HP_ERR;
    }
#ifdef __LP64__
    if (shdr_size == sizeof(Elf32_Shdr)) {
        Elf32_Shdr shdr32 = *(Elf32_Shdr *)shdr;
        shdr->sh_name = shdr32.sh_name;
        shdr->sh_type = shdr32.sh_type;
        shdr->sh_flags = shdr32.sh_flags;
        shdr->sh_addr = shdr32.sh_addr;
        shdr->sh_offset = shdr32.sh_offset;
        shdr->sh_size = shdr32.sh_size;
        shdr->sh_link = shdr32.sh_link;
        shdr->sh_info = shdr32.sh_info;
        shdr->sh_addralign = shdr32.sh_addralign;
        shdr->sh_entsize = shdr32.sh_entsize;
    }
#endif
    return 0;
}

static int read_elf_sym(FILE *fp, Elf_Sym *sym, size_t sym_size)
{
    if (fread(sym, sym_size, 1, fp) != 1) {
        ELOG_ERROR("failed to read a symbol table entry. (error: %s)", strerror(errno));
        return HP_ERR;
    }
#ifdef __LP64__
    if (sym_size == sizeof(Elf32_Sym)) {
        Elf32_Sym sym32 = *(Elf32_Sym *)sym;
        sym->st_name = sym32.st_name;
        sym->st_value = sym32.st_value;
        sym->st_size = sym32.st_size;
        sym->st_info = sym32.st_info;
        sym->st_other = sym32.st_other;
        sym->st_shndx = sym32.st_shndx;
    }
#endif
    return 0;
}

static size_t find_strtab_offset(FILE *fp, size_t offset, size_t size, const char *name)
{
    size_t off;
    size_t idx = 0;

    fseek(fp, offset, SEEK_SET);
    for (off = 0; off < size; off++) {
        int c = fgetc(fp);
        if (c == EOF) {
            return 0;
        }
        if (c == name[idx]) {
            if (c == 0) {
                return off - idx;
            }
            idx++;
        } else {
            idx = 0;
        }
    }
    return 0;
}



