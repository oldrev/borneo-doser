#include <stdio.h>
#include <string.h>

#include <mbedtls/md5.h>

#include "borneo/utils/md5.h"

int MD5_compute_buffer(const void* buf, size_t size, uint8_t* digest)
{

    mbedtls_md5_context ctx;

    mbedtls_md5_init(&ctx);
    mbedtls_md5_starts_ret(&ctx);

    mbedtls_md5_update_ret(&ctx, buf, size);

    mbedtls_md5_finish_ret(&ctx, digest);

    return 0;
}

int MD5_compute_buffer_to_hex(const void* buf, size_t size, char* digest_str)
{
    uint8_t digest[MD5_DIGEST_SIZE];
    int ret = MD5_compute_buffer(buf, size, digest);
    if (ret != 0) {
        return ret;
    }
    for (size_t i = 0; i < MD5_DIGEST_SIZE; i++) {
        sprintf(&digest_str[i * 2], "%02X", (unsigned int)digest[i]);
    }

    return 0;
}