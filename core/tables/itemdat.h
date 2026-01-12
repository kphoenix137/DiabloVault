// itemdat.h
#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace dv::tables {

// Subset of DevilutionX enums needed for deterministic name regeneration.
enum class ItemType {
	None,
	Misc,
	Sword,
	Axe,
	Bow,
	Mace,
	Shield,
	LightArmor,
	Helm,
	MediumArmor,
	HeavyArmor,
	Staff,
	Gold,
	Ring,
	Amulet,
};

enum class ItemMiscId {
	None,
	Staff,
	Ring,
	Amulet,
	Book,
	Ear,
};

enum goodorevil {
	GOE_ANY,
	GOE_EVIL,
	GOE_GOOD,
};

enum class AffixItemType : uint8_t {
	None   = 0,
	Misc   = 1 << 0,
	Bow    = 1 << 1,
	Staff  = 1 << 2,
	Weapon = 1 << 3,
	Shield = 1 << 4,
	Armor  = 1 << 5,
};

inline AffixItemType operator|(AffixItemType a, AffixItemType b)
{
	return static_cast<AffixItemType>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline bool HasAnyOf(AffixItemType needle, AffixItemType haystack)
{
	return (static_cast<uint8_t>(needle) & static_cast<uint8_t>(haystack)) != 0;
}

struct ItemDataRow {
	int mappingId = -1;
	std::string itemType;       // raw "itemType" column (string)
	std::string miscId;         // raw "miscId" column (string)
	std::string uniqueBaseItem; // raw "uniqueBaseItem" column (string)
	ItemType itemTypeEnum = ItemType::None;
	ItemMiscId miscIdEnum = ItemMiscId::None;
	int uniqueBaseItemId = -1; // resolved ID shared between itemdat/unique_itemdat
	std::string name;
	std::string shortName;
	int minMonsterLevel = 0;
	int durability = 0;
	int minDamage = 0;
	int maxDamage = 0;
	int minArmor = 0;
	int maxArmor = 0;
	int minStrength = 0;
	int minMagic = 0;
	int minDexterity = 0;
	int value = 0;
};

struct UniqueItemRow {
	int mappingId = -1;
	int uniqueBaseItemId = -1; // resolved to ItemDataRow.uniqueBaseItemId
	std::string name;
	int minLevel = 0;
	int value = 0;
};

struct AffixRow {
	std::string name;
	int minLevel = 0;
	int chance = 0;
	AffixItemType itemTypes = AffixItemType::None; // bitmask, e.g. Weapon|Armor
	goodorevil alignment = GOE_ANY;
	int minVal = 0;
	int maxVal = 0;
	int multVal = 0;
};

// Loads and holds TSV-based item data.
class ItemDb {
public:
	bool LoadFromDirectory(const std::string &txtdataDir, std::string &err);
	bool IsLoaded() const { return loaded_; }
	std::string_view TxtdataDir() const { return txtdataDir_; }

	// In DevilutionX, the packed idx is a mapping id corresponding to row order in itemdat.tsv.
	const ItemDataRow *TryGetItemByMappingId(int mappingId) const;
	const UniqueItemRow *TryGetUniqueByIndex(int uniqueIndex) const;
	const AffixRow *TryGetPrefixByIndex(int idx) const;
	const AffixRow *TryGetSuffixByIndex(int idx) const;

	// Direct accessors used by the regeneration logic.
	const std::vector<AffixRow> &Prefixes() const { return prefixes_; }
	const std::vector<AffixRow> &Suffixes() const { return suffixes_; }

private:
	std::string txtdataDir_;
	bool loaded_ = false;
	std::vector<ItemDataRow> items_;
	std::vector<UniqueItemRow> uniques_;
	std::vector<AffixRow> prefixes_;
	std::vector<AffixRow> suffixes_;
};

// Singleton used by the core library.
ItemDb &GetItemDb();

} // namespace dv::tables
