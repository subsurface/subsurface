/*
 * SHA1 routine optimized to do word accesses rather than byte accesses,
 * and to avoid unnecessary copies into the context array.
 *
 * This was initially based on the Mozilla SHA1 implementation, although
 * none of the original Mozilla code remains.
 */
#ifndef SHA1_H
#define SHA1_H

#include <array>
#include <string>

struct SHA1
{
	SHA1();
	void update(const void *dataIn, unsigned long len);
	void update(const std::string &s);
	// Note: the hash() functions change state. Call only once.
	std::array<unsigned char, 20> hash();
	uint32_t hash_uint32(); // Return first 4 bytes of hash interpreted
				// as little-endian unsigned integer.
private:
	unsigned long long size;
	unsigned int H[5];
	unsigned int W[16];
};

/* Helper function that calculates an SHA1 has and returns the first 4 bytes as uint32_t */
uint32_t SHA1_uint32(const void *dataIn, unsigned long len);

#endif // SHA1_H
