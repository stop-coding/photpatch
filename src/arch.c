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


#include "arch.h"

const static  uint8_t g_byte_code[] = JUMP_BYTE_CODE;

static int is_little_endian()
{
  union
  {
    unsigned int a;
    unsigned char b; 
  }c;
  c.a = 1;
  return (c.b == 1); 
}

int get_jump_byte_codes(uint64_t addr, uint8_t *codes, uint32_t len)
{
    int byte_code_len = sizeof(g_byte_code)/sizeof(uint8_t);
    int is_little = is_little_endian();

    uint8_t *addr_btyes = (uint8_t *)&addr;
    if (len < byte_code_len) {
        return -1;
    }
    for (int i = 0; i < byte_code_len; i++) {
        if (i >= ADDR_OFFSET &&  i < (ADDR_OFFSET + ADDR_SIZE)) {
            codes[i] = addr_btyes[i - ADDR_OFFSET];
        } else {
            codes[i] = g_byte_code[i];
        }
    }
    return byte_code_len;
}


