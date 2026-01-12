/**
 * @file sha.h
 *
 * X-SHA-1 (Diablo's flawed SHA-1-like compression) used by the save-game codec.
 *
 * Ported from DevilutionX. Namespace adjusted for DiabloVault.
 */
#pragma once

#include <cstddef>
#include <cstdint>

namespace dv::devx {

	constexpr std::size_t BlockSize = 16;     // uint32_t words per block
	constexpr std::size_t SHA1HashSize = 5;   // uint32_t words in state/digest

	struct SHA1Context {
		uint32_t state[SHA1HashSize] = {
			0x67452301u,
			0xEFCDAB89u,
			0x98BADCFEu,
			0x10325476u,
			0xC3D2E1F0u
		};
		uint32_t buffer[BlockSize]{};
	};

	void SHA1Result(SHA1Context& context, uint32_t messageDigest[SHA1HashSize]);
	void SHA1Calculate(SHA1Context& context, const uint32_t data[BlockSize]);

} // namespace dv::devx
