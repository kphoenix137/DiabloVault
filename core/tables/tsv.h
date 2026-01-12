// tsv.h
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace dv::tables {

struct TsvRow {
	std::unordered_map<std::string, std::string> cols;

	std::string_view get(std::string_view key) const;
	std::optional<int> getInt(std::string_view key) const;
	std::optional<unsigned> getUInt(std::string_view key) const;
	bool getBool(std::string_view key, bool defaultValue = false) const;
	std::vector<std::string> getList(std::string_view key, char delim) const;
};

struct TsvTable {
	std::vector<std::string> headers;
	std::vector<TsvRow> rows;
};

// Reads a TSV file with a header row. Lines beginning with '#' are treated as comments.
// Fields are split on '\t' with no special escaping rules.
bool ReadTsvFile(const std::string &path, TsvTable &out, std::string &err);

} // namespace dv::tables
