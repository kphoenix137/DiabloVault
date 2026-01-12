#include "d1/ItemRegen.h"

#include "tables/itemdat.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace dv::d1 {

// Source: DevilutionX loadsave.cpp (RemapItemIdxFromDiablo)
static uint16_t RemapItemIdxFromDiablo(uint16_t i)
{
	if (i == 5 /*IDI_SORCERER*/)
		return 166 /*IDI_SORCERER_DIABLO*/;
	if (i >= 156)
		i = static_cast<uint16_t>(i + 5);
	if (i >= 88)
		i = static_cast<uint16_t>(i + 1);
	if (i >= 83)
		i = static_cast<uint16_t>(i + 4);
	return i;
}

// Diablo/Devilution LCG (matches DevilutionX engine/random.hpp DiabloGenerator).
class DiabloRng {
public:
	explicit DiabloRng(uint32_t seed)
	    : seed_(seed)
	{
	}

	void SetSeed(uint32_t seed) { seed_ = seed; }

	uint32_t GetSeed() const { return seed_; }

	uint32_t Advance()
	{
		seed_ = seed_ * 0x015A4E35u + 1u;
		return seed_;
	}

	int32_t AdvanceRng()
	{
		int32_t v = static_cast<int32_t>(Advance());
		if (v == static_cast<int32_t>(0x80000000u))
			return v;
		return v < 0 ? -v : v;
	}

	int32_t GenerateRnd(int32_t v)
	{
		if (v <= 0)
			return 0;
		if (v <= 0x7FFF)
			return (AdvanceRng() >> 16) % v;
		return AdvanceRng() % v;
	}

	bool FlipCoin(unsigned n = 2)
	{
		return (GenerateRnd(static_cast<int32_t>(n)) == 0);
	}

	void DiscardRandomValues(int n)
	{
		for (int i = 0; i < n; ++i)
			Advance();
	}

private:
	uint32_t seed_;
};

// icreateinfo flags (subset, copied from DevilutionX items.h)
enum : uint16_t {
	CF_LEVEL = (1u << 6) - 1u,
	CF_ONLYGOOD = 1u << 6,
	CF_UPER15 = 1u << 7,
	CF_UPER1 = 1u << 8,
	CF_UNIQUE = 1u << 9,
	CF_SMITH = 1u << 10,
	CF_SMITHPREMIUM = 1u << 11,
	CF_BOY = 1u << 12,
	CF_WITCH = 1u << 13,
	CF_UIDOFFSET = ((1u << 4) - 1u) << 1,
	CF_PREGEN = 1u << 15,
};

static int GetItemBLevel(DiabloRng &rng, int lvl, dv::tables::ItemMiscId miscId, bool onlygood, bool uper15)
{
	// Source: DevilutionX GetItemBLevel
	int iblvl = -1;
	if (rng.GenerateRnd(100) <= 10
	    || rng.GenerateRnd(100) <= lvl
	    || onlygood
	    || (miscId == dv::tables::ItemMiscId::Staff || miscId == dv::tables::ItemMiscId::Ring || miscId == dv::tables::ItemMiscId::Amulet)) {
		iblvl = lvl;
	}
	if (uper15)
		iblvl = lvl + 4;
	return iblvl;
}

static dv::tables::AffixItemType GetAffixItemType(const dv::tables::ItemDataRow &base)
{
	using namespace dv::tables;
	switch (base.itemTypeEnum) {
	case ItemType::Sword:
	case ItemType::Axe:
	case ItemType::Mace:
		return AffixItemType::Weapon;
	case ItemType::Bow:
		return AffixItemType::Bow;
	case ItemType::Shield:
		return AffixItemType::Shield;
	case ItemType::LightArmor:
	case ItemType::Helm:
	case ItemType::MediumArmor:
	case ItemType::HeavyArmor:
		return AffixItemType::Armor;
	case ItemType::Staff:
		return AffixItemType::Staff;
	case ItemType::Ring:
	case ItemType::Amulet:
		return AffixItemType::Misc;
	default:
		return AffixItemType::None;
	}
}

static std::optional<const dv::tables::AffixRow *> SelectAffix(
    DiabloRng &rng,
    const std::vector<dv::tables::AffixRow> &affixList,
    dv::tables::AffixItemType type,
    int minlvl, int maxlvl,
    bool onlygood,
    dv::tables::goodorevil goe)
{
	using namespace dv::tables;
	std::vector<const AffixRow *> eligible;
	eligible.reserve(256);

	for (const AffixRow &a : affixList) {
		if (!HasAnyOf(type, a.itemTypes))
			continue;
		if (a.minLevel < minlvl || a.minLevel > maxlvl)
			continue;
		// DevilutionX uses PLOk to reject "bad" powers for onlygood. TSV doesn't expose that,
		// but alignment filtering still matters for many items.
		if ((goe == GOE_GOOD && a.alignment == GOE_EVIL) || (goe == GOE_EVIL && a.alignment == GOE_GOOD))
			continue;

		for (int i = 0; i < a.chance; ++i)
			eligible.push_back(&a);
	}

	if (eligible.empty())
		return std::nullopt;

	return eligible[static_cast<size_t>(rng.GenerateRnd(static_cast<int32_t>(eligible.size())))];
}

struct ChosenAffixes {
	const dv::tables::AffixRow *prefix = nullptr;
	const dv::tables::AffixRow *suffix = nullptr;
};

static ChosenAffixes GetItemPowerPrefixAndSuffix(
    DiabloRng &rng,
    const dv::tables::ItemDb &db,
    int minlvl, int maxlvl,
    dv::tables::AffixItemType flgs,
    bool onlygood)
{
	using namespace dv::tables;
	ChosenAffixes out;

	bool allocatePrefix = rng.FlipCoin(4);
	bool allocateSuffix = !rng.FlipCoin(3);
	if (!allocatePrefix && !allocateSuffix) {
		if (rng.FlipCoin())
			allocatePrefix = true;
		else
			allocateSuffix = true;
	}

	goodorevil goe = GOE_ANY;
	if (!onlygood && !rng.FlipCoin(3))
		onlygood = true;

	if (allocatePrefix) {
		std::optional<const AffixRow *> p = SelectAffix(rng, db.Prefixes(), flgs, minlvl, maxlvl, onlygood, goe);
		if (p.has_value()) {
			out.prefix = *p;
			goe = out.prefix->alignment;
		}
	}

	if (allocateSuffix) {
		std::optional<const AffixRow *> s = SelectAffix(rng, db.Suffixes(), flgs, minlvl, maxlvl, onlygood, goe);
		if (s.has_value())
			out.suffix = *s;
	}

	return out;
}

static std::string GenerateMagicItemName(
    const std::string &baseName,
    const dv::tables::AffixRow *prefix,
    const dv::tables::AffixRow *suffix)
{
	std::string out;
	if (prefix != nullptr)
		out += prefix->name + " ";
	out += baseName;
	if (suffix != nullptr)
		out += " of " + suffix->name;
	return out;
}

static const dv::tables::UniqueItemRow *PickUniqueByOffset(const dv::tables::ItemDb &db, int baseItemId, int lvl, int uidOffset)
{
	// Source: DevilutionX GetValidUniques + CheckUnique selection order.
	std::vector<const dv::tables::UniqueItemRow *> valid;
	for (int i = 0;; ++i) {
		const auto *u = db.TryGetUniqueByIndex(i);
		if (u == nullptr)
			break;
		if (u->uniqueBaseItemId != baseItemId)
			continue;
		if (lvl < u->minLevel)
			continue;
		valid.push_back(u);
	}
	if (valid.empty())
		return nullptr;
	if (uidOffset < 0 || static_cast<size_t>(uidOffset) >= valid.size())
		return nullptr;
	return valid[valid.size() - 1 - static_cast<size_t>(uidOffset)];
}

UnpackedItemView RegenerateItemView(const ItemPack &pk, bool isHellfire)
{
	UnpackedItemView view;
	if (pk.idx == 0xFFFF)
		return view;

	// Packed identification bit is stable.
	const bool isIdentified = (pk.bId & 1) != 0;
	view.quality = isIdentified ? "identified" : "unidentified";

	uint16_t mappingId = pk.idx;
	if (!isHellfire)
		mappingId = RemapItemIdxFromDiablo(mappingId);

	using namespace dv::tables;
	ItemDb &db = GetItemDb();
	const ItemDataRow *base = db.IsLoaded() ? db.TryGetItemByMappingId(static_cast<int>(mappingId)) : nullptr;
	if (base == nullptr) {
		view.name = "(unknown item)";
		return view;
	}

	view.baseName = base->name;
	view.reqStr = base->minStrength;
	view.reqMag = base->minMagic;
	view.reqDex = base->minDexterity;

	const uint16_t icreate = pk.iCreateInfo;
	const int lvl = static_cast<int>(icreate & CF_LEVEL);
	view.ilvl = lvl;
	const bool onlygood = (icreate & (CF_ONLYGOOD | CF_SMITHPREMIUM | CF_BOY | CF_WITCH)) != 0;
	const bool uper15 = (icreate & CF_UPER15) != 0;
	const int uidOffset = static_cast<int>((icreate & CF_UIDOFFSET) >> 1);

	// If the item is not "created" (icreate==0), the name is always the base name.
	if (icreate == 0) {
		view.name = base->name;
		return view;
	}

	DiabloRng rng(pk.iSeed);

	// The original generation path consumes RNG values before deciding affixes/uniques.
	// We replicate the subset that affects prefix/suffix selection.
	// (See DevilutionX GetTranslatedItemNameMagical for vendor/source-specific discards.)

	// GetItemAttrs (always consumes 1)
	rng.DiscardRandomValues(1);

	int minlvl = 0;
	int maxlvl = 0;
	if ((icreate & CF_SMITHPREMIUM) != 0) {
		// RndVendorItem and GetItemAttrs
		rng.DiscardRandomValues(2);
		minlvl = lvl / 2;
		maxlvl = lvl;
	} else if ((icreate & CF_BOY) != 0) {
		rng.DiscardRandomValues(2);
		minlvl = lvl;
		maxlvl = lvl * 2;
	} else if ((icreate & CF_WITCH) != 0) {
		rng.DiscardRandomValues(2);
		int iblvl = -1;
		if (rng.GenerateRnd(100) <= 5)
			iblvl = 2 * lvl;
		if (iblvl == -1 && base->miscIdEnum == ItemMiscId::Staff)
			iblvl = 2 * lvl;
		minlvl = iblvl / 2;
		maxlvl = iblvl;
	} else {
		// GetItemBLevel + CheckUnique consumes additional RNG.
		const int iblvl = GetItemBLevel(rng, lvl, base->miscIdEnum, onlygood, uper15);
		minlvl = iblvl / 2;
		maxlvl = iblvl;
		// CheckUnique roll
		rng.DiscardRandomValues(1);
	}

	minlvl = std::min(minlvl, 25);

	// Unique regeneration: if CF_UNIQUE is set, the unique is fully determined by base item + lvl + uidOffset.
	if ((icreate & CF_UNIQUE) != 0) {
		const UniqueItemRow *u = PickUniqueByOffset(db, base->uniqueBaseItemId, maxlvl, uidOffset);
		if (u != nullptr) {
			view.name = isIdentified ? u->name : base->name;
			view.quality += ", unique";
			view.affixes = "unique";
			return view;
		}
		// Fall through to magic naming if the unique cannot be resolved.
	}

	const AffixItemType affixType = GetAffixItemType(*base);
	if (affixType == AffixItemType::None || maxlvl < 0) {
		view.name = base->name;
		return view;
	}

	const ChosenAffixes aff = GetItemPowerPrefixAndSuffix(rng, db, minlvl, maxlvl, affixType, onlygood);
	const std::string identifiedName = GenerateMagicItemName(base->name, aff.prefix, aff.suffix);
	view.name = isIdentified ? identifiedName : base->name;
	view.quality += ", magic";

	std::string a;
	if (aff.prefix != nullptr)
		a += "prefix=" + aff.prefix->name;
	if (aff.suffix != nullptr) {
		if (!a.empty())
			a += ", ";
		a += "suffix=" + aff.suffix->name;
	}
	view.affixes = std::move(a);
	return view;
}

} // namespace dv::d1
