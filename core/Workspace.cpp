// Workspace.cpp

#include "Workspace.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>
#include <unordered_set>

#include "devx/codec.h"
#include "d1/D1Packed.h"
#include "d1/MpqStorm.h"

namespace fs = std::filesystem;

namespace dv {

	int CoreVersion()
	{
		return kCoreVersion;
	}

	static std::string ToLower(std::string s)
	{
		for (char& c : s)
			c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		return s;
	}

	static bool EndsWith(std::string_view s, std::string_view suffix)
	{
		return s.size() >= suffix.size() && s.substr(s.size() - suffix.size()) == suffix;
	}

	static bool StartsWith(std::string_view s, std::string_view prefix)
	{
		return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
	}

	static ContainerKind ClassifySaveName(std::string_view filenameLowerNoExt)
	{
		// Devilution/DevilutionX naming conventions for packed saves.
		if (filenameLowerNoExt == "stash" || filenameLowerNoExt == "stash_spawn")
			return ContainerKind::SharedStash;

		// Character saves: single_0, multi_2, share_0, spawn_1, etc.
		if (StartsWith(filenameLowerNoExt, "single_")
			|| StartsWith(filenameLowerNoExt, "multi_")
			|| StartsWith(filenameLowerNoExt, "share_")
			|| StartsWith(filenameLowerNoExt, "spawn_")) {
			return ContainerKind::CharacterSave;
		}

		return ContainerKind::Unknown;
	}

	static bool DecodeInPlaceTryPasswords(std::vector<std::byte>& buf, std::size_t& decodedLen, std::string_view filenameLowerNoExt)
	{
		// Matches DevilutionX passwords (pfile.cpp).
		constexpr const char* PASSWORD_SPAWN_SINGLE = "adslhfb1";
		constexpr const char* PASSWORD_SPAWN_MULTI = "lshbkfg1";
		constexpr const char* PASSWORD_SINGLE = "xrgyrkj1";
		constexpr const char* PASSWORD_MULTI = "szqnlsk1";

		// Try the likely password first based on filename, then fall back to all.
		const char* preferred = PASSWORD_SINGLE;
		if (StartsWith(filenameLowerNoExt, "multi_"))
			preferred = PASSWORD_MULTI;
		else if (StartsWith(filenameLowerNoExt, "spawn_"))
			preferred = PASSWORD_SPAWN_SINGLE;

		const char* candidates[] = { preferred, PASSWORD_SINGLE, PASSWORD_MULTI, PASSWORD_SPAWN_SINGLE, PASSWORD_SPAWN_MULTI };
		for (const char* pw : candidates) {
			std::vector<std::byte> tmp = buf;
			const std::size_t outLen = dv::devx::codec_decode(tmp.data(), tmp.size(), pw);
			std::fprintf(stderr, "codec_decode pw='%s' -> outLen=%zu\n", pw, outLen);
			if (outLen == 0)
				continue;
			buf.swap(tmp);
			decodedLen = outLen;
			return true;
		}
		return false;
	}

	bool Workspace::Open(const std::string& rootDir)
	{
		rootDir_.clear();
		containers_.clear();

		std::error_code ec;
		fs::path root = fs::absolute(fs::path(rootDir), ec);
		if (ec)
			return false;
		if (!fs::exists(root, ec) || !fs::is_directory(root, ec))
			return false;

		rootDir_ = root.string();

		// Scan only the selected directory (non-recursive).
		// We accept:
		//   - Packed saves: *.sv / *.hsv
		//   - Unpacked save directories created by some ports/tools (contain "hero", etc.)
		std::vector<Container> found;
		for (const fs::directory_entry& de : fs::directory_iterator(root, ec)) {
			if (ec)
				break;

			const fs::path p = de.path();
			if (de.is_regular_file(ec)) {
				const std::string extLower = ToLower(p.extension().string());
				if (extLower != ".sv" && extLower != ".hsv")
					continue;

				std::string stemLower = ToLower(p.stem().string());
				const ContainerKind kind = ClassifySaveName(stemLower);
				if (kind == ContainerKind::Unknown)
					continue;

				Container c;
				c.id = stemLower + extLower; // stable enough; includes extension to avoid collisions
				c.displayName = p.stem().string();
				c.path = fs::absolute(p, ec).string();
				c.kind = kind;
				found.push_back(std::move(c));
				continue;
			}

			if (!de.is_directory(ec))
				continue;

			// Unpacked saves: directory names follow the same convention (single_0, stash, etc.)
			const std::string dirName = p.filename().string();
			const std::string dirNameLower = ToLower(dirName);
			ContainerKind kind = ClassifySaveName(dirNameLower);
			if (kind == ContainerKind::Unknown)
				continue;

			// Heuristic: ensure it contains at least one known file.
			const bool hasHero = fs::exists(p / "hero", ec);
			const bool hasStash = fs::exists(p / "spstashitems", ec) || fs::exists(p / "mpstashitems", ec);
			if (kind == ContainerKind::CharacterSave && !hasHero)
				continue;
			if (kind == ContainerKind::SharedStash && !hasStash)
				continue;

			Container c;
			c.id = dirNameLower; // directory names are already unique within root
			c.displayName = dirName;
			c.path = fs::absolute(p, ec).string();
			c.kind = kind;
			found.push_back(std::move(c));
		}

		// Sort: Character saves first, then stash; alphabetical within.
		auto kindRank = [](ContainerKind k) {
			switch (k) {
			case ContainerKind::CharacterSave: return 0;
			case ContainerKind::SharedStash: return 1;
			default: return 2;
			}
			};
		std::sort(found.begin(), found.end(), [&](const Container& a, const Container& b) {
			const int ra = kindRank(a.kind);
			const int rb = kindRank(b.kind);
			if (ra != rb)
				return ra < rb;
			return ToLower(a.displayName) < ToLower(b.displayName);
			});

		containers_ = std::move(found);
		return true;
	}

	std::optional<Container> Workspace::FindContainer(std::string_view containerId) const
	{
		for (const Container& c : containers_) {
			if (c.id == containerId)
				return c;
		}
		return std::nullopt;
	}

	std::vector<ItemRecord> Workspace::LoadItemsFor(std::string_view containerId) const
	{
		std::vector<ItemRecord> items;
		const auto c = FindContainer(containerId);
		if (!c)
			return items;

		std::error_code ec;
		fs::path p = fs::path(c->path);

		// Unpacked directory format (initial milestone).
		if (fs::is_directory(p, ec)) {
			if (c->kind == ContainerKind::CharacterSave) {
				std::ifstream f(p / "hero", std::ios::binary);
				if (!f)
					return items;
				d1::PlayerPack pack{};
				f.read(reinterpret_cast<char*>(&pack), sizeof(pack));
				if (!f)
					return items;

				// Header row: show character identity in the first columns.
				{
					ItemRecord r;
					r.sourcePath = c->path + "/hero";
					r.name = std::string(pack.pName);
					r.baseType = "Class=" + std::to_string(pack.pClass);
					r.quality = "Level=" + std::to_string(pack.pLevel);
					r.affixes = "XP=" + std::to_string(pack.pExperience);
					r.location = "(character)";
					items.push_back(std::move(r));
				}

				auto addPacked = [&](const d1::ItemPack& ip, const std::string& loc) {
					if (ip.idx == 0)
						return;
					ItemRecord r;
					r.sourcePath = c->path + "/hero";
					r.name = "(packed item)";
					r.baseType = "idx=" + std::to_string(ip.idx);
					r.quality = ip.bId != 0 ? "identified" : "unidentified";
					r.affixes = "seed=" + std::to_string(ip.iSeed) + ", val=" + std::to_string(ip.wValue);
					r.location = loc;
					items.push_back(std::move(r));
					};

				// Body equipment
				for (int i = 0; i < d1::NumInvLoc; ++i)
					addPacked(pack.InvBody[i], "Body[" + std::to_string(i) + "]");

				// Inventory list (40 cells)
				for (int i = 0; i < d1::InventoryGridCells; ++i)
					addPacked(pack.InvList[i], "Inventory[" + std::to_string(i) + "]");

				// Belt
				for (int i = 0; i < d1::MaxBeltItems; ++i)
					addPacked(pack.SpdList[i], "Belt[" + std::to_string(i) + "]");

				return items;
			}

			if (c->kind == ContainerKind::SharedStash) {
				auto addStashSummary = [&](const char* fileName, const std::string& label) {
					std::ifstream sf(p / fileName, std::ios::binary);
					if (!sf)
						return;
					uint8_t version = 0;
					uint32_t gold = 0;
					uint32_t pages = 0;
					sf.read(reinterpret_cast<char*>(&version), sizeof(version));
					sf.read(reinterpret_cast<char*>(&gold), sizeof(gold));
					sf.read(reinterpret_cast<char*>(&pages), sizeof(pages));
					if (!sf)
						return;
					ItemRecord r;
					r.sourcePath = (p / fileName).string();
					r.name = label;
					r.baseType = "ver=" + std::to_string(version);
					r.quality = "gold=" + std::to_string(gold);
					r.affixes = "pages=" + std::to_string(pages);
					r.location = "(stash summary)";
					items.push_back(std::move(r));
					};

				addStashSummary("spstashitems", "SP Stash");
				addStashSummary("mpstashitems", "MP Stash");
				return items;
			}
		}

		// Packed MPQ saves (*.sv/*.hsv) are detected but not parsed in this milestone.
		// Packed MPQ saves (*.sv/*.hsv).
		// If StormLib is enabled, we can read and decode at least the 'hero' record.
		{
			std::string err;
			std::vector<std::byte> raw;
			const std::string stemLower = ToLower(fs::path(c->path).stem().string());
			if (dv::d1::ReadMpqFileStorm(c->path, "hero", raw, err)) {
				std::size_t decodedLen = 0;

				std::fprintf(stderr, "MPQ open OK: %s hero bytes=%zu\n", c->path.c_str(), raw.size());

				if (raw.size() >= 16) {
					const auto* b = reinterpret_cast<const unsigned char*>(raw.data());
					std::fprintf(stderr,
						"hero[0..15]=%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
						b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7],
						b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15]);
				}
				if (raw.size() >= 8) {
					const auto* b = reinterpret_cast<const unsigned char*>(raw.data());
					const size_t n = raw.size();
					std::fprintf(stderr,
						"hero[last8]=%02X %02X %02X %02X %02X %02X %02X %02X\n",
						b[n - 8], b[n - 7], b[n - 6], b[n - 5], b[n - 4], b[n - 3], b[n - 2], b[n - 1]);
				}

				if (DecodeInPlaceTryPasswords(raw, decodedLen, stemLower) && decodedLen >= sizeof(d1::PlayerPack)) {
					d1::PlayerPack pack{};
					std::memcpy(&pack, raw.data(), sizeof(pack));

					ItemRecord hdr;
					hdr.sourcePath = c->path + "::hero";
					hdr.name = std::string(pack.pName);
					hdr.baseType = "Class=" + std::to_string(pack.pClass);
					hdr.quality = "Level=" + std::to_string(pack.pLevel);
					hdr.affixes = "XP=" + std::to_string(pack.pExperience);
					hdr.location = "(packed hero)";
					items.push_back(std::move(hdr));

					auto addPacked = [&](const d1::ItemPack& ip, const std::string& loc) {
						if (ip.idx == 0)
							return;
						ItemRecord r;
						r.sourcePath = c->path + "::hero";
						r.name = "(packed item)";
						r.baseType = "idx=" + std::to_string(ip.idx);
						r.quality = ip.bId != 0 ? "identified" : "unidentified";
						r.affixes = "seed=" + std::to_string(ip.iSeed) + ", val=" + std::to_string(ip.wValue);
						r.location = loc;
						items.push_back(std::move(r));
						};

					for (int i = 0; i < d1::NumInvLoc; ++i)
						addPacked(pack.InvBody[i], "Body[" + std::to_string(i) + "]");
					for (int i = 0; i < d1::InventoryGridCells; ++i)
						addPacked(pack.InvList[i], "Inventory[" + std::to_string(i) + "]");
					for (int i = 0; i < d1::MaxBeltItems; ++i)
						addPacked(pack.SpdList[i], "Belt[" + std::to_string(i) + "]");

					return items;
				}
				// Decode failed.
				ItemRecord r;
				r.sourcePath = c->path;
				r.name = "(failed to decode 'hero')";
				r.baseType = "Check password/format";
				r.affixes = "Try enabling StormLib + correct save type";
				r.location = err;
				items.push_back(std::move(r));
				return items;
			}

			// Either StormLib disabled or cannot open MPQ.
			ItemRecord r;
			r.sourcePath = c->path;
			r.name = "(packed save)";
			r.baseType = "MPQ";
			r.quality = "Not loaded";
			r.affixes = err.empty() ? "Enable DIABLOVAULT_ENABLE_STORMLIB" : err;
			r.location = "packed";
			items.push_back(std::move(r));
			return items;
		}
	}

} // namespace dv
