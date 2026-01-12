#include "d1/ItemRegen.h"

#include "tables/itemdat.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace dv::d1 {

// Devilution/Diablo LCG, compatible with how Diablo chooses random numbers.
class DiabloLCG {
public:
	explicit DiabloLCG(uint32_t seed)
	    : state_(seed)
	{
	}

	uint32_t next()
	{
		// (a * x + c) mod 2^32, where a=0x015A4E35, c=1
		state_ = state_ * 0x015A4E35u + 1u;
		return state_;
	}

	int32_t advanceAbs()
	{
		int32_t v = static_cast<int32_t>(next());
		// abs(INT_MIN) is UB; mimic DevilutionX behavior (special-case)
		if (v == static_cast<int32_t>(0x80000000u))
			return v;
		return v < 0 ? -v : v;
	}

	int32_t rnd(int32_t v)
	{
		if (v <= 0)
			return 0;
		if (v <= 0x7FFF) {
			return (advanceAbs() >> 16) % v;
		}
		return advanceAbs() % v;
	}

private:
	uint32_t state_;
};

// icreateinfo flags (copied from DevilutionX items.h)
enum : uint16_t {
	CF_LEVEL = (1u << 6) - 1u,
	CF_ONLYGOOD = 1u << 6,
	CF_UPER15 = 1u << 7,
	CF_UPER1 = 1u << 8,
	CF_UNIQUE = 1u << 9,
	CF_PREGEN = 1u << 15,
};

static bool ItemTypeMatches(const std::vector<std::string> &list, std::string_view itemType)
{
	if (list.empty() || itemType.empty())
		return false;
	for (const std::string &t : list) {
		if (t == itemType)
			return true;
	}
	return false;
}

struct SelectedAffixes {
	std::string prefix;
	std::string suffix;
};

static SelectedAffixes SelectAffixesForDisplay(uint32_t seed, int ilvl, std::string_view itemType)
{
	using namespace dv::tables;
	SelectedAffixes out;
	ItemDb &db = GetItemDb();
	if (!db.IsLoaded())
		return out;

	DiabloLCG rng(seed);
	// Roughly emulate the first RNG consumption in GetItemAttrs.
	rng.next();

	auto selectOne = [&](bool isPrefix) -> std::string {
		// Build candidate list with weights.
		struct Cand {
			const AffixRow *row;
			int weight;
		};
		std::vector<Cand> cands;
		int total = 0;
		// We don't have a direct accessor by range; iterate by index.
		for (int i = 0;; ++i) {
			const AffixRow *r = isPrefix ? db.TryGetPrefixByIndex(i) : db.TryGetSuffixByIndex(i);
			if (r == nullptr)
				break;
			if (r->chance <= 0)
				continue;
			if (r->minLevel > ilvl)
				continue;
			if (!ItemTypeMatches(r->itemTypes, itemType))
				continue;
			cands.push_back({ r, r->chance });
			total += r->chance;
		}
		if (cands.empty() || total <= 0)
			return {};
		int pick = rng.rnd(total);
		for (const auto &c : cands) {
			pick -= c.weight;
			if (pick < 0)
				return c.row->name;
		}
		return cands.back().row->name;
	};

	// For display we attempt both prefix and suffix; real game rules are more nuanced.
	out.prefix = selectOne(true);
	out.suffix = selectOne(false);
	return out;
}

static const dv::tables::UniqueItemRow *SelectUniqueForDisplay(int baseMappingId, int ilvl)
{
	using namespace dv::tables;
	ItemDb &db = GetItemDb();
	if (!db.IsLoaded())
		return nullptr;

	// Vanilla tends to pick the last applicable unique for a base.
	const UniqueItemRow *best = nullptr;
	for (int i = 0;; ++i) {
		const UniqueItemRow *u = db.TryGetUniqueByIndex(i);
		if (u == nullptr)
			break;
		if (u->uniqueBaseItemMappingId != baseMappingId)
			continue;
		if (u->minLevel > ilvl)
			continue;
		best = u;
	}
	return best;
}

UnpackedItemView RegenerateItemView(const ItemPack &pk, bool isHellfire)
{
	UnpackedItemView view;
	(void)isHellfire; // presently unused for display-only regeneration

	if (pk.idx == 0xFFFF)
		return view;

	const uint16_t icreate = pk.iCreateInfo;
	const int ilvl = static_cast<int>(icreate & CF_LEVEL);
	view.ilvl = ilvl;
	view.quality = (pk.bId & 1) ? "identified" : "unidentified";

	using namespace dv::tables;
	ItemDb &db = GetItemDb();
	const ItemDataRow *base = db.IsLoaded() ? db.TryGetItemByMappingId(static_cast<int>(pk.idx)) : nullptr;
	if (base != nullptr) {
		view.baseName = base->name;
		view.reqStr = base->minStrength;
		view.reqMag = base->minMagic;
		view.reqDex = base->minDexterity;
	}

	if (base == nullptr) {
		view.name = "(unknown item)";
		return view;
	}

	// Quality / naming
	const bool wantsUnique = (icreate & CF_UNIQUE) != 0;
	const bool wantsMagic = (icreate != 0) && !wantsUnique;

	if (wantsUnique) {
		if (const UniqueItemRow *u = SelectUniqueForDisplay(base->mappingId, ilvl)) {
			view.name = u->name;
			view.quality += ", unique";
			view.affixes = "unique";
			return view;
		}
		// If no unique found, fall back to magic naming.
	}

	if (wantsMagic) {
		SelectedAffixes a = SelectAffixesForDisplay(pk.iSeed, ilvl, base->itemType);
		view.name.clear();
		if (!a.prefix.empty()) {
			view.name += a.prefix;
			view.name += ' ';
		}
		view.name += base->name;
		if (!a.suffix.empty()) {
			view.name += " of ";
			view.name += a.suffix;
		}
		view.quality += ", magic";
		std::string aff;
		if (!a.prefix.empty())
			aff += "prefix=" + a.prefix;
		if (!a.suffix.empty()) {
			if (!aff.empty())
				aff += ", ";
			aff += "suffix=" + a.suffix;
		}
		view.affixes = std::move(aff);
		return view;
	}

	// Normal/base item
	view.name = base->name;
	return view;
}

} // namespace dv::d1
