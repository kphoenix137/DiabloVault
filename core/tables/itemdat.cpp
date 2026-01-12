// itemdat.cpp

#include "tables/itemdat.h"
#include "tables/tsv.h"

#include <filesystem>

namespace fs = std::filesystem;

namespace dv::tables {

static std::string Join(const std::string &a, const std::string &b)
{
	fs::path p = fs::path(a) / b;
	// Avoid std::u8string (char8_t) to keep MSVC /std:c++20 builds simple.
	return p.lexically_normal().string();
}

static void ReadCommonItemRow(const TsvRow &row, ItemDataRow &out)
{
	out.itemType = std::string(row.get("itemType"));
	out.miscId = std::string(row.get("miscId"));
	out.uniqueBaseItem = std::string(row.get("uniqueBaseItem"));
	out.name = std::string(row.get("name"));
	out.shortName = std::string(row.get("shortName"));
	out.minMonsterLevel = row.getInt("minMonsterLevel").value_or(0);
	out.durability = row.getInt("durability").value_or(0);
	out.minDamage = row.getInt("minDamage").value_or(0);
	out.maxDamage = row.getInt("maxDamage").value_or(0);
	out.minArmor = row.getInt("minArmor").value_or(0);
	out.maxArmor = row.getInt("maxArmor").value_or(0);
	out.minStrength = row.getInt("minStrength").value_or(0);
	out.minMagic = row.getInt("minMagic").value_or(0);
	out.minDexterity = row.getInt("minDexterity").value_or(0);
	out.value = row.getInt("value").value_or(0);
}

static void ReadAffixRow(const TsvRow &row, AffixRow &out)
{
	out.name = std::string(row.get("name"));
	out.minLevel = row.getInt("minLevel").value_or(0);
	out.chance = row.getInt("chance").value_or(0);
	// itemTypes is a "|" separated list in DevilutionX TSVs.
	out.itemTypes = row.getList("itemTypes", '|');
	out.minVal = row.getInt("minVal").value_or(0);
	out.maxVal = row.getInt("maxVal").value_or(0);
	out.multVal = row.getInt("multVal").value_or(0);
}

static int TryResolveBaseItemMappingId(const std::vector<ItemDataRow> &items, std::string_view token)
{
	// Accept either an integer mapping id or a name/shortName match.
	if (token.empty())
		return -1;
	bool allDigits = true;
	for (char c : token) {
		if (c < '0' || c > '9') {
			allDigits = false;
			break;
		}
	}
	if (allDigits) {
		try {
			return std::stoi(std::string(token));
		} catch (...) {
			return -1;
		}
	}
	for (const ItemDataRow &r : items) {
		if (r.name == token || r.shortName == token)
			return r.mappingId;
	}
	return -1;
}

bool ItemDb::LoadFromDirectory(const std::string &txtdataDir, std::string &err)
{
	const std::string itemdatPath = Join(txtdataDir, "itemdat.tsv");
	const std::string uniquePath = Join(txtdataDir, "unique_itemdat.tsv");
	const std::string prefixPath = Join(txtdataDir, "item_prefixes.tsv");
	const std::string suffixPath = Join(txtdataDir, "item_suffixes.tsv");

	TsvTable t;
	items_.clear();
	uniques_.clear();
	prefixes_.clear();
	suffixes_.clear();

	if (!ReadTsvFile(itemdatPath, t, err))
		return false;
	items_.reserve(t.rows.size());
	int mappingId = 0;
	for (const TsvRow &r : t.rows) {
		ItemDataRow row;
		row.mappingId = mappingId++;
		ReadCommonItemRow(r, row);
		items_.emplace_back(std::move(row));
	}

	if (!ReadTsvFile(uniquePath, t, err))
		return false;
	uniques_.reserve(t.rows.size());
	int uniqueMappingId = 0;
	for (const TsvRow &r : t.rows) {
		UniqueItemRow u;
		u.mappingId = uniqueMappingId++;
		u.name = std::string(r.get("name"));
		u.uniqueBaseItemMappingId = TryResolveBaseItemMappingId(items_, r.get("uniqueBaseItem"));
		u.minLevel = r.getInt("minLevel").value_or(0);
		u.value = r.getInt("value").value_or(0);
		uniques_.emplace_back(std::move(u));
	}

	if (!ReadTsvFile(prefixPath, t, err))
		return false;
	prefixes_.reserve(t.rows.size());
	for (const TsvRow &r : t.rows) {
		AffixRow a;
		ReadAffixRow(r, a);
		prefixes_.emplace_back(std::move(a));
	}

	if (!ReadTsvFile(suffixPath, t, err))
		return false;
	suffixes_.reserve(t.rows.size());
	for (const TsvRow &r : t.rows) {
		AffixRow a;
		ReadAffixRow(r, a);
		suffixes_.emplace_back(std::move(a));
	}

	txtdataDir_ = txtdataDir;
	loaded_ = true;
	return true;
}

const ItemDataRow *ItemDb::TryGetItemByMappingId(int mappingId) const
{
	if (mappingId < 0)
		return nullptr;
	const std::size_t idx = static_cast<std::size_t>(mappingId);
	if (idx >= items_.size())
		return nullptr;
	return &items_[idx];
}

const UniqueItemRow *ItemDb::TryGetUniqueByIndex(int uniqueIndex) const
{
	if (uniqueIndex < 0)
		return nullptr;
	const std::size_t idx = static_cast<std::size_t>(uniqueIndex);
	if (idx >= uniques_.size())
		return nullptr;
	return &uniques_[idx];
}

const AffixRow *ItemDb::TryGetPrefixByIndex(int idx) const
{
	if (idx < 0)
		return nullptr;
	const std::size_t i = static_cast<std::size_t>(idx);
	if (i >= prefixes_.size())
		return nullptr;
	return &prefixes_[i];
}

const AffixRow *ItemDb::TryGetSuffixByIndex(int idx) const
{
	if (idx < 0)
		return nullptr;
	const std::size_t i = static_cast<std::size_t>(idx);
	if (i >= suffixes_.size())
		return nullptr;
	return &suffixes_[i];
}

ItemDb &GetItemDb()
{
	static ItemDb g;
	return g;
}

} // namespace dv::tables
