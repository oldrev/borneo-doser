#pragma once

#ifdef __cplusplus  
extern "C" { 
#endif 
    /* Declarations of this file */

enum {
    MD5_DIGEST_SIZE = 16,
    MD5_DIGEST_HEX_SIZE = 32,
};

int MD5_compute_buffer(const void* buf, size_t size, uint8_t* digest);

int MD5_compute_buffer_to_hex(const void* buf, size_t size, char* digest_str);










#ifdef __cplusplus 
} 
#endif 

