// codec.cpp
//
// Save-game codec used by Diablo / DevilutionX for *.sv / *.hsv contents.
//
// This module is intentionally self-contained.
//
// Notes on correctness:
// DevilutionX's codec uses a running SHA1 context as a keystream generator:
//   - initialize context by hashing a 64-byte key buffer
//   - for each 64-byte block: XOR with SHA1(current_context) digest words,
//     then hash the decrypted block into the context.
//
// SHA1Result in DevilutionX is effectively "digest of current transcript"
// without mutating the running context (it duplicates the context and finalizes
// the copy). We mirror that behavior using Windows CryptoAPI.

#include "codec.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib, "advapi32.lib")
#else
#error "DiabloVault codec currently requires Windows (CryptoAPI)"
#endif
#include <stdexcept>

namespace dv::devx {
namespace {

constexpr std::size_t BlockSize = 16;
constexpr std::size_t BlockSizeBytes = BlockSize * sizeof(uint32_t);
constexpr std::size_t SHA1HashSize = 5; // 5 uint32_t words = 20 bytes
constexpr std::size_t SignatureSize = 8;

static inline uint32_t LoadLE32(const std::byte *p)
{
	const uint8_t b0 = static_cast<uint8_t>(p[0]);
	const uint8_t b1 = static_cast<uint8_t>(p[1]);
	const uint8_t b2 = static_cast<uint8_t>(p[2]);
	const uint8_t b3 = static_cast<uint8_t>(p[3]);
	return (static_cast<uint32_t>(b0))
	    | (static_cast<uint32_t>(b1) << 8)
	    | (static_cast<uint32_t>(b2) << 16)
	    | (static_cast<uint32_t>(b3) << 24);
}

static inline uint32_t LoadLE32(const char *p)
{
	const auto b0 = static_cast<uint8_t>(p[0]);
	const auto b1 = static_cast<uint8_t>(p[1]);
	const auto b2 = static_cast<uint8_t>(p[2]);
	const auto b3 = static_cast<uint8_t>(p[3]);
	return (static_cast<uint32_t>(b0))
	    | (static_cast<uint32_t>(b1) << 8)
	    | (static_cast<uint32_t>(b2) << 16)
	    | (static_cast<uint32_t>(b3) << 24);
}

static inline uint32_t Swap32(uint32_t v)
{
	return ((v & 0x000000FFu) << 24)
	    | ((v & 0x0000FF00u) << 8)
	    | ((v & 0x00FF0000u) >> 8)
	    | ((v & 0xFF000000u) >> 24);
}

struct CodecSignature {
	uint32_t checksum;
	uint8_t error;
	uint8_t lastChunkSize;
};

class Sha1Ctx {
public:
	Sha1Ctx()
	{
		if (!CryptAcquireContextW(&prov_, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
			ok_ = false;
		else if (!CryptCreateHash(prov_, CALG_SHA1, 0, 0, &hash_))
			ok_ = false;
	}

	Sha1Ctx(const Sha1Ctx &) = delete;
	Sha1Ctx &operator=(const Sha1Ctx &) = delete;

	Sha1Ctx(Sha1Ctx &&o) noexcept
	    : prov_(o.prov_)
	    , hash_(o.hash_)
	    , ok_(o.ok_)
	{
		o.prov_ = 0;
		o.hash_ = 0;
		o.ok_ = false;
	}

	Sha1Ctx &operator=(Sha1Ctx &&o) noexcept
	{
		if (this == &o)
			return *this;
		Cleanup();
		prov_ = o.prov_;
		hash_ = o.hash_;
		ok_ = o.ok_;
		o.prov_ = 0;
		o.hash_ = 0;
		o.ok_ = false;
		return *this;
	}

	~Sha1Ctx() { Cleanup(); }

	bool ok() const { return ok_ && prov_ != 0 && hash_ != 0; }

	bool Update(const void *data, std::size_t bytes)
	{
		if (!ok())
			return false;
		return CryptHashData(hash_, static_cast<const BYTE *>(data), static_cast<DWORD>(bytes), 0) != 0;
	}

	// Return digest words for the current transcript WITHOUT mutating this context.
	bool DigestWords(uint32_t out[SHA1HashSize]) const
	{
		if (!ok())
			return false;
		HCRYPTHASH dup = 0;
		if (!CryptDuplicateHash(hash_, nullptr, 0, &dup))
			return false;
		BYTE digest[20];
		DWORD len = sizeof(digest);
		const BOOL got = CryptGetHashParam(dup, HP_HASHVAL, digest, &len, 0);
		CryptDestroyHash(dup);
		if (!got || len != sizeof(digest))
			return false;
		for (int i = 0; i < 5; ++i) {
			const int o = i * 4;
			out[i] = (uint32_t(digest[o]) << 24)
			    | (uint32_t(digest[o + 1]) << 16)
			    | (uint32_t(digest[o + 2]) << 8)
			    | (uint32_t(digest[o + 3]));
		}
		return true;
	}

private:
	void Cleanup()
	{
		if (hash_)
			CryptDestroyHash(hash_);
		if (prov_)
			CryptReleaseContext(prov_, 0);
		hash_ = 0;
		prov_ = 0;
		ok_ = false;
	}

	HCRYPTPROV prov_ = 0;
	HCRYPTHASH hash_ = 0;
	bool ok_ = true;
};

static Sha1Ctx CodecInitKey(const char *pszPassword)
{
	uint32_t pw[BlockSize];
	std::size_t j = 0;
	for (uint32_t &value : pw) {
		if (pszPassword[j] == '\0')
			j = 0;
		value = LoadLE32(&pszPassword[j]);
		j += sizeof(uint32_t);
	}

	uint32_t digest[SHA1HashSize] {};
	{
		Sha1Ctx ctx;
		ctx.Update(pw, sizeof(pw));
		ctx.DigestWords(digest);
	}

	uint32_t key[BlockSize] {
		2908958655u, 4146550480u,  658981742u, 1113311088u,
		3927878744u,  679301322u, 1760465731u, 3305370375u,
		2269115995u, 3928541685u,  580724401u, 2607446661u,
		2233092279u, 2416822349u, 4106933702u, 3046442503u
	};

	for (unsigned i = 0; i < BlockSize; ++i)
		key[i] ^= digest[(i + 3) % SHA1HashSize];

	Sha1Ctx ctx;
	ctx.Update(key, sizeof(key));
	return ctx;
}

static CodecSignature GetCodecSignature(std::byte *src)
{
	CodecSignature result;
	result.checksum = LoadLE32(src);
	src += 4;
	result.error = static_cast<uint8_t>(*src++);
	result.lastChunkSize = static_cast<uint8_t>(*src);
	return result;
}

static void ByteSwapBlock(uint32_t *data)
{
	for (std::size_t i = 0; i < BlockSize; ++i)
		data[i] = Swap32(data[i]);
}

static void XorBlock(const uint32_t *shaResultWords, uint32_t *inOut)
{
	for (unsigned i = 0; i < BlockSize; ++i)
		inOut[i] ^= shaResultWords[i % SHA1HashSize];
}

} // namespace

std::size_t codec_decode(std::byte *pbSrcDst, std::size_t size, const char *pszPassword)
{
	uint32_t buf[BlockSize];
	uint32_t dst[SHA1HashSize];

	Sha1Ctx context = CodecInitKey(pszPassword);
	if (!context.ok())
		return 0;
	if (size <= SignatureSize)
		return 0;
	size -= SignatureSize;
	if (size % BlockSizeBytes != 0)
		return 0;

	for (std::size_t i = 0; i < size; pbSrcDst += BlockSizeBytes, i += BlockSizeBytes) {
		std::memcpy(buf, pbSrcDst, BlockSizeBytes);
		ByteSwapBlock(buf);
		if (!context.DigestWords(dst))
			return 0;
		XorBlock(dst, buf);
		// hash decrypted block into the running context
		if (!context.Update(buf, sizeof(buf)))
			return 0;
		ByteSwapBlock(buf);
		std::memcpy(pbSrcDst, buf, BlockSizeBytes);
	}

	std::memset(buf, 0, sizeof(buf));
	const CodecSignature sig = GetCodecSignature(pbSrcDst);
	if (sig.error > 0)
		return 0;

	if (!context.DigestWords(dst))
		return 0;
	if (sig.checksum != dst[0])
		return 0;

	// lastChunkSize stores how many bytes of the last 256-byte block are actually used.
	size += sig.lastChunkSize - BlockSizeBytes;
	return size;
}

std::size_t codec_get_encoded_len(std::size_t dwSrcBytes)
{
	if (dwSrcBytes % BlockSizeBytes != 0)
		dwSrcBytes += BlockSizeBytes - (dwSrcBytes % BlockSizeBytes);
	return dwSrcBytes + SignatureSize;
}

void codec_encode(std::byte * /*pbSrcDst*/, std::size_t /*size*/, std::size_t /*size_64*/, const char * /*pszPassword*/)
{
	// DiabloVault is read-only for now.
	throw std::runtime_error("codec_encode is not implemented in DiabloVault (read-only)");
}

} // namespace dv::devx
