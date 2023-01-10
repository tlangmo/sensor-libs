#pragma once
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
typedef unsigned int MD5_u32plus;

typedef struct {
	MD5_u32plus lo, hi;
	MD5_u32plus a, b, c, d;
	unsigned char buffer[64];
	MD5_u32plus block[16];
} Md5Context;

void md5_init(Md5Context *ctx);
void md5_update(Md5Context *ctx, const void *data, unsigned long size);
void md5_final(unsigned char *result, Md5Context *ctx);
#ifdef __cplusplus
}
#endif /* __cplusplus */