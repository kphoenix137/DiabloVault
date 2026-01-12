// itemdat.cpp

#include "tables/itemdat.h"
#include "tables/tsv.h"

#include <filesystem>
#include <unordered_map>

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
	// Parsed enums/IDs are filled after we load all items.
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

static ItemType ParseItemType(std::string_view value)
{
	// Values are from DevilutionX itemdat.tsv "itemType".
	if (value == "Misc") return ItemType::Misc;
	if (value == "Sword") return ItemType::Sword;
	if (value == "Axe") return ItemType::Axe;
	if (value == "Bow") return ItemType::Bow;
	if (value == "Mace") return ItemType::Mace;
	if (value == "Shield") return ItemType::Shield;
	if (value == "LightArmor") return ItemType::LightArmor;
	if (value == "Helm") return ItemType::Helm;
	if (value == "MediumArmor") return ItemType::MediumArmor;
	if (value == "HeavyArmor") return ItemType::HeavyArmor;
	if (value == "Staff") return ItemType::Staff;
	if (value == "Gold") return ItemType::Gold;
	if (value == "Ring") return ItemType::Ring;
	if (value == "Amulet") return ItemType::Amulet;
	if (value == "None") return ItemType::None;
	return ItemType::None;
}

static ItemMiscId ParseMiscId(std::string_view value)
{
	// Values are from DevilutionX itemdat.tsv "miscId".
	if (value == "Staff") return ItemMiscId::Staff;
	if (value == "Ring") return ItemMiscId::Ring;
	if (value == "Amulet") return ItemMiscId::Amulet;
	if (value == "Book") return ItemMiscId::Book;
	if (value == "Ear") return ItemMiscId::Ear;
	if (value == "None") return ItemMiscId::None;
	return ItemMiscId::None;
}

static goodorevil ParseAlignment(std::string_view value)
{
	if (value == "Any") return GOE_ANY;
	if (value == "Evil") return GOE_EVIL;
	if (value == "Good") return GOE_GOOD;
	return GOE_ANY;
}

static AffixItemType ParseAffixItemTypes(const std::vector<std::string> &tokens)
{
	AffixItemType mask = AffixItemType::None;
	for (const std::string &t : tokens) {
		if (t == "Misc") mask = mask | AffixItemType::Misc;
		else if (t == "Bow") mask = mask | AffixItemType::Bow;
		else if (t == "Staff") mask = mask | AffixItemType::Staff;
		else if (t == "Weapon") mask = mask | AffixItemType::Weapon;
		else if (t == "Shield") mask = mask | AffixItemType::Shield;
		else if (t == "Armor") mask = mask | AffixItemType::Armor;
	}
	return mask;
}

static void ReadAffixRow(const TsvRow &row, AffixRow &out)
{
	out.name = std::string(row.get("name"));
	out.minLevel = row.getInt("minLevel").value_or(0);
	out.chance = row.getInt("chance").value_or(0);
	// itemTypes is a "|" separated list in DevilutionX TSVs.
	out.itemTypes = ParseAffixItemTypes(row.getList("itemTypes", '|'));
	out.alignment = ParseAlignment(row.get("alignment"));
	out.minVal = row.getInt("minVal").value_or(0);
	out.maxVal = row.getInt("maxVal").value_or(0);
	out.multVal = row.getInt("multVal").value_or(0);
}

static int ResolveOrAddUniqueBaseItemId(std::unordered_map<std::string, int> &map, std::string_view token)
{
	if (token.empty())
		return -1;
	const std::string key(token);
	auto it = map.find(key);
	if (it != map.end())
		return it->second;
	const int id = static_cast<int>(map.size());
	map.emplace(key, id);
	return id;
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

	// Build unique base-item ID map and parse enums now that we have all item rows.
	std::unordered_map<std::string, int> uniqueBaseItemIds;
	uniqueBaseItemIds.reserve(items_.size());
	for (ItemDataRow &it : items_) {
		it.itemTypeEnum = ParseItemType(it.itemType);
		it.miscIdEnum = ParseMiscId(it.miscId);
		it.uniqueBaseItemId = ResolveOrAddUniqueBaseItemId(uniqueBaseItemIds, it.uniqueBaseItem);
	}

	if (!ReadTsvFile(uniquePath, t, err))
		return false;
	uniques_.reserve(t.rows.size());
	int uniqueMappingId = 0;
	for (const TsvRow &r : t.rows) {
		UniqueItemRow u;
		u.mappingId = uniqueMappingId++;
		u.name = std::string(r.get("name"));
		u.uniqueBaseItemId = ResolveOrAddUniqueBaseItemId(uniqueBaseItemIds, r.get("uniqueBaseItem"));
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
