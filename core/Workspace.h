// Workspace.h
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace dv {

// Bump this whenever the core-side API/data model changes in a way that the GUI depends on.
constexpr int kCoreVersion = 2;

int CoreVersion();

enum class ContainerKind : uint8_t {
	Unknown = 0,
	CharacterSave,
	SharedStash,
};

struct Container {
	std::string id;          // stable ID (derived from filename)
	std::string displayName; // user-facing label
	std::string path;        // absolute file path
	ContainerKind kind = ContainerKind::Unknown;
};

struct ItemRecord {
	std::string name;
	std::string baseType;
	std::string quality;
	std::string affixes;
	int ilvl = 0;
	int reqLvl = 0;
	std::string location;
	std::string sourcePath; // where this came from (tooltip)
};

class Workspace {
public:
	bool Open(const std::string &rootDir);
	const std::string &RootDir() const { return rootDir_; }
	const std::vector<Container> &Containers() const { return containers_; }

	// For now this returns placeholder rows per container.
	// Next milestone: parse actual save/stash contents and return real item fields.
	std::vector<ItemRecord> LoadItemsFor(std::string_view containerId) const;

	std::optional<Container> FindContainer(std::string_view containerId) const;

private:
	std::string rootDir_;
	std::vector<Container> containers_;
};

} // namespace dv
