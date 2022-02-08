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
#include <stdint.h>
#include <stdlib.h>

#ifndef _ARCH_H_
#define _ARCH_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__x86_64__) && defined(__LP64__)
// 函数字节码长度
#define FUNC_BYTE_CODE_MIN_LEN  12

/*
x86_64汇编
mov rax,address64
jmp rax
*/
#define JUMP_BYTE_CODE  {0x48, 0xb8, 0x88, 0x77, 0x66, 0x55,0x44,0x33, 0x22, 0x11, 0xff, 0xe0}

#define ADDR_SIZE        8

#define ADDR_OFFSET      2

//是否小端
#define ARCH_NAME "X86_64"

#else
    #error "unsupport"
#endif


int get_jump_byte_codes(uint64_t addr, uint8_t *codes, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif
