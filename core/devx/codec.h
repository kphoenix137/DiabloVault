/**
 * @file codec.h
 *
 * Interface of save game encryption algorithm (Diablo / Hellfire save codec).
 *
 * Ported from DevilutionX. Namespace adjusted for DiabloVault.
 */
#pragma once

#include <cstddef>
#include <cstdint>

namespace dv::devx {

	std::size_t codec_decode(std::byte* pbSrcDst, std::size_t size, const char* pszPassword);
	std::size_t codec_get_encoded_len(std::size_t dwSrcBytes);
	void codec_encode(std::byte* pbSrcDst, std::size_t size, std::size_t size_64, const char* pszPassword);

} // namespace dv::devx
