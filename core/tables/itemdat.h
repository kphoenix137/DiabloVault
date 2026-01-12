// itemdat.h
#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace dv::tables {

struct ItemDataRow {
	int mappingId = -1;
	std::string itemType;       // e.g. Sword, Bow, Ring...
	std::string miscId;         // miscId column
	std::string uniqueBaseItem; // raw uniqueBaseItem value from TSV (string or number)
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
	int uniqueBaseItemMappingId = -1; // resolved to ItemDataRow.mappingId when possible
	std::string name;
	int minLevel = 0;
	int value = 0;
};

struct AffixRow {
	std::string name;
	int minLevel = 0;
	int chance = 0;
	std::vector<std::string> itemTypes; // strings from TSV, e.g. Sword, Bow
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
