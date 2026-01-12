#include "codec.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>

#include "sha.h"

namespace dv::devx {
	namespace {

		struct CodecSignature {
			uint32_t checksum;
			uint8_t error;
			uint8_t lastChunkSize;
		};

		constexpr std::size_t BlockSizeBytes = BlockSize * sizeof(uint32_t);
		constexpr std::size_t SignatureSize = 8;

		static uint32_t LoadLE32(const std::byte* p)
		{
			return (uint32_t(static_cast<uint8_t>(p[0])))
				| (uint32_t(static_cast<uint8_t>(p[1])) << 8)
				| (uint32_t(static_cast<uint8_t>(p[2])) << 16)
				| (uint32_t(static_cast<uint8_t>(p[3])) << 24);
		}

		static uint32_t Swap32LE(uint32_t v)
		{
			// Windows/x86/x64 are little-endian; keep this as a no-op for perf,
			// but retain the function so the code matches the reference structure.
			return v;
		}

		static uint32_t LoadPasswordWordRepeated(const char* pszPassword, std::size_t wordIndex)
		{
			// Diablo code effectively repeats the password until it fills 64 bytes.
			// The original implementation reads uint32_t chunks; for safety and
			// determinism, do it byte-wise with wraparound.
			const std::size_t len = std::strlen(pszPassword);
			if (len == 0)
				return 0;

			const std::size_t base = wordIndex * 4;
			const uint8_t b0 = static_cast<uint8_t>(pszPassword[(base + 0) % len]);
			const uint8_t b1 = static_cast<uint8_t>(pszPassword[(base + 1) % len]);
			const uint8_t b2 = static_cast<uint8_t>(pszPassword[(base + 2) % len]);
			const uint8_t b3 = static_cast<uint8_t>(pszPassword[(base + 3) % len]);

			return uint32_t(b0) | (uint32_t(b1) << 8) | (uint32_t(b2) << 16) | (uint32_t(b3) << 24);
		}

		static SHA1Context CodecInitKey(const char* pszPassword)
		{
			uint32_t pw[BlockSize];
			for (std::size_t i = 0; i < BlockSize; ++i)
				pw[i] = LoadPasswordWordRepeated(pszPassword, i);

			uint32_t digest[SHA1HashSize];
			{
				SHA1Context context;
				SHA1Calculate(context, pw);
				SHA1Result(context, digest);
			}

			uint32_t key[BlockSize]{
				2908958655u, 4146550480u,  658981742u, 1113311088u,
				3927878744u,  679301322u, 1760465731u, 3305370375u,
				2269115995u, 3928541685u,  580724401u, 2607446661u,
				2233092279u, 2416822349u, 4106933702u, 3046442503u
			};

			for (unsigned i = 0; i < BlockSize; ++i)
				key[i] ^= digest[(i + 3) % SHA1HashSize];

			SHA1Context context;
			SHA1Calculate(context, key);
			return context;
		}

		static CodecSignature GetCodecSignature(std::byte* src)
		{
			CodecSignature result;
			result.checksum = LoadLE32(src);
			src += 4;
			result.error = static_cast<uint8_t>(*src++);
			result.lastChunkSize = static_cast<uint8_t>(*src);
			return result;
		}

		static void ByteSwapBlock(uint32_t* data)
		{
			for (std::size_t i = 0; i < BlockSize; ++i)
				data[i] = Swap32LE(data[i]);
		}

		static void XorBlock(const uint32_t* shaResult, uint32_t* out)
		{
			for (unsigned i = 0; i < BlockSize; ++i)
				out[i] ^= shaResult[i % SHA1HashSize];
		}

	} // namespace

	std::size_t codec_decode(std::byte* pbSrcDst, std::size_t size, const char* pszPassword)
	{
		uint32_t buf[BlockSize];
		uint32_t dst[SHA1HashSize];

		SHA1Context context = CodecInitKey(pszPassword);

		if (size <= SignatureSize)
			return 0;
		size -= SignatureSize;

		// size is bytes; it must be a multiple of 64 bytes (16 words).
		if (size % BlockSizeBytes != 0)
			return 0;

		for (std::size_t i = 0; i < size; pbSrcDst += BlockSizeBytes, i += BlockSizeBytes) {
			std::memcpy(buf, pbSrcDst, BlockSizeBytes);
			ByteSwapBlock(buf);

			SHA1Result(context, dst);
			XorBlock(dst, buf);

			// Critical: Diablo uses X-SHA-1 "calculate" (compression step) on the decrypted block
			// to advance the rolling context (no padding/finalize).
			SHA1Calculate(context, buf);

			ByteSwapBlock(buf);
			std::memcpy(pbSrcDst, buf, BlockSizeBytes);
		}

		std::memset(buf, 0, sizeof(buf));
		const CodecSignature sig = GetCodecSignature(pbSrcDst);
		if (sig.error > 0)
			return 0;

		SHA1Result(context, dst);
		if (sig.checksum != dst[0])
			return 0;

		// lastChunkSize is the actual byte count of the final block (<= 64)
		size += sig.lastChunkSize - BlockSizeBytes;
		return size;
	}

	std::size_t codec_get_encoded_len(std::size_t dwSrcBytes)
	{
		if (dwSrcBytes % BlockSizeBytes != 0)
			dwSrcBytes += BlockSizeBytes - (dwSrcBytes % BlockSizeBytes);
		return dwSrcBytes + SignatureSize;
	}

	void codec_encode(std::byte* /*pbSrcDst*/, std::size_t /*size*/, std::size_t /*size_64*/, const char* /*pszPassword*/)
	{
		// Keep DiabloVault read-only unless/until you intentionally implement writing.
		throw std::runtime_error("codec_encode is not implemented in DiabloVault (read-only)");
	}

} // namespace dv::devx
