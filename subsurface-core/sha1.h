/*
 * SHA1 routine optimized to do word accesses rather than byte accesses,
 * and to avoid unnecessary copies into the context array.
 *
 * This was initially based on the Mozilla SHA1 implementation, although
 * none of the original Mozilla code remains.
 */
#ifndef SHA1_H
#define SHA1_H

typedef struct
{
	unsigned long long size;
	unsigned int H[5];
	unsigned int W[16];
} blk_SHA_CTX;

void blk_SHA1_Init(blk_SHA_CTX *ctx);
void blk_SHA1_Update(blk_SHA_CTX *ctx, const void *dataIn, unsigned long len);
void blk_SHA1_Final(unsigned char hashout[20], blk_SHA_CTX *ctx);

/* Make us use the standard names */
#define SHA_CTX blk_SHA_CTX
#define SHA1_Init blk_SHA1_Init
#define SHA1_Update blk_SHA1_Update
#define SHA1_Final blk_SHA1_Final

/* Trivial helper function */
static inline void SHA1(const void *dataIn, unsigned long len, unsigned char hashout[20])
{
	SHA_CTX ctx;

	SHA1_Init(&ctx);
	SHA1_Update(&ctx, dataIn, len);
	SHA1_Final(hashout, &ctx);
}

#endif // SHA1_H
