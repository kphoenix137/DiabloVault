// tsv.cpp

#include "tables/tsv.h"

#include <cctype>
#include <charconv>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace dv::tables {

static inline std::string Trim(std::string s)
{
	while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || std::isspace(static_cast<unsigned char>(s.back()))))
		s.pop_back();
	std::size_t i = 0;
	while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])))
		++i;
	if (i != 0)
		s.erase(0, i);
	return s;
}

std::string_view TsvRow::get(std::string_view key) const
{
	auto it = cols.find(std::string(key));
	if (it == cols.end())
		return {};
	return it->second;
}

static std::optional<long long> ParseInt64(std::string_view sv)
{
	while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front())))
		sv.remove_prefix(1);
	while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back())))
		sv.remove_suffix(1);
	if (sv.empty())
		return std::nullopt;
	long long v = 0;
	auto res = std::from_chars(sv.data(), sv.data() + sv.size(), v);
	if (res.ec != std::errc{})
		return std::nullopt;
	return v;
}

std::optional<int> TsvRow::getInt(std::string_view key) const
{
	auto sv = get(key);
	auto v = ParseInt64(sv);
	if (!v.has_value())
		return std::nullopt;
	return static_cast<int>(*v);
}

std::optional<unsigned> TsvRow::getUInt(std::string_view key) const
{
	auto sv = get(key);
	auto v = ParseInt64(sv);
	if (!v.has_value() || *v < 0)
		return std::nullopt;
	return static_cast<unsigned>(*v);
}

bool TsvRow::getBool(std::string_view key, bool defaultValue) const
{
	auto sv = get(key);
	if (sv.empty())
		return defaultValue;
	// Accept 0/1, true/false, yes/no.
	std::string s(sv);
	for (char &c : s)
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	s = Trim(std::move(s));
	if (s == "1" || s == "true" || s == "yes")
		return true;
	if (s == "0" || s == "false" || s == "no")
		return false;
	return defaultValue;
}

std::vector<std::string> TsvRow::getList(std::string_view key, char delim) const
{
	std::string_view sv = get(key);
	std::vector<std::string> out;
	if (sv.empty())
		return out;
	std::string s(sv);
	// Accept either the requested delimiter or ',' in practice.
	for (char &c : s) {
		if (c == ',')
			c = delim;
	}
	std::size_t start = 0;
	while (start <= s.size()) {
		std::size_t end = s.find(delim, start);
		if (end == std::string::npos)
			end = s.size();
		std::string part = Trim(s.substr(start, end - start));
		if (!part.empty())
			out.emplace_back(std::move(part));
		start = end + 1;
	}
	return out;
}

static void SplitTabs(const std::string &line, std::vector<std::string> &out)
{
	out.clear();
	std::size_t start = 0;
	while (start <= line.size()) {
		std::size_t end = line.find('\t', start);
		if (end == std::string::npos)
			end = line.size();
		out.emplace_back(line.substr(start, end - start));
		start = end + 1;
	}
}

bool ReadTsvFile(const std::string &path, TsvTable &out, std::string &err)
{
	out = {};
	std::ifstream f(path, std::ios::binary);
	if (!f) {
		err = "Failed to open TSV: " + path;
		return false;
	}
	std::string line;
	std::vector<std::string> parts;
	bool haveHeader = false;
	while (std::getline(f, line)) {
		line = Trim(line);
		if (line.empty())
			continue;
		if (!line.empty() && line[0] == '#')
			continue;
		SplitTabs(line, parts);
		if (!haveHeader) {
			haveHeader = true;
			out.headers = parts;
			continue;
		}
		TsvRow row;
		for (std::size_t i = 0; i < out.headers.size(); ++i) {
			std::string value;
			if (i < parts.size())
				value = parts[i];
			row.cols.emplace(out.headers[i], Trim(std::move(value)));
		}
		out.rows.emplace_back(std::move(row));
	}
	if (!haveHeader) {
		err = "TSV missing header row: " + path;
		return false;
	}
	return true;
}

} // namespace dv::tables
