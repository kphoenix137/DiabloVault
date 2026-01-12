#include "sha.h"

#include <cstdint>
#include <cstring>

namespace dv::devx {

	// NOTE: Diablo's "SHA1" differs from real SHA-1 in that it uses arithmetic
	// right shifts (sign bit extension) in the circular shift helper.

	namespace {

		uint32_t SHA1CircularShift(uint32_t word, std::size_t bits)
		{
			// When the sign bit is set, arithmetic right shift produces 1s in the high bits.
			// This matches the original buggy implementation behavior.
			if ((word & (1u << 31)) != 0)
				return (0xFFFFFFFFu << bits) | (word >> (32 - bits));
			return (word << bits) | (word >> (32 - bits));
		}

		void SHA1ProcessMessageBlock(SHA1Context* context)
		{
			uint32_t w[80];
			std::memcpy(w, context->buffer, BlockSize * sizeof(uint32_t));

			// Expand words (note: Diablo's variant does NOT do the usual rotate here)
			for (int i = 16; i < 80; i++) {
				w[i] = w[i - 16] ^ w[i - 14] ^ w[i - 8] ^ w[i - 3];
			}

			uint32_t a = context->state[0];
			uint32_t b = context->state[1];
			uint32_t c = context->state[2];
			uint32_t d = context->state[3];
			uint32_t e = context->state[4];

			for (int i = 0; i < 20; i++) {
				const uint32_t temp = SHA1CircularShift(a, 5)
					+ ((b & c) | ((~b) & d))
					+ e + w[i] + 0x5A827999u;
				e = d;
				d = c;
				c = SHA1CircularShift(b, 30);
				b = a;
				a = temp;
			}

			for (int i = 20; i < 40; i++) {
				const uint32_t temp = SHA1CircularShift(a, 5)
					+ (b ^ c ^ d)
					+ e + w[i] + 0x6ED9EBA1u;
				e = d;
				d = c;
				c = SHA1CircularShift(b, 30);
				b = a;
				a = temp;
			}

			for (int i = 40; i < 60; i++) {
				const uint32_t temp = SHA1CircularShift(a, 5)
					+ ((b & c) | (b & d) | (c & d))
					+ e + w[i] + 0x8F1BBCDCu;
				e = d;
				d = c;
				c = SHA1CircularShift(b, 30);
				b = a;
				a = temp;
			}

			for (int i = 60; i < 80; i++) {
				const uint32_t temp = SHA1CircularShift(a, 5)
					+ (b ^ c ^ d)
					+ e + w[i] + 0xCA62C1D6u;
				e = d;
				d = c;
				c = SHA1CircularShift(b, 30);
				b = a;
				a = temp;
			}

			context->state[0] += a;
			context->state[1] += b;
			context->state[2] += c;
			context->state[3] += d;
			context->state[4] += e;
		}

	} // namespace

	void SHA1Result(SHA1Context& context, uint32_t messageDigest[SHA1HashSize])
	{
		std::memcpy(messageDigest, context.state, sizeof(context.state));
	}

	void SHA1Calculate(SHA1Context& context, const uint32_t data[BlockSize])
	{
		std::memcpy(&context.buffer[0], data, BlockSize * sizeof(uint32_t));
		SHA1ProcessMessageBlock(&context);
	}

} // namespace dv::devx
