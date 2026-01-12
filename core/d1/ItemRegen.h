#pragma once

#include "d1/D1Packed.h"

#include <string>

namespace dv::d1 {

// Human-readable view derived from a packed Diablo item.
struct UnpackedItemView {
	std::string name;       // full display name (prefix/base/suffix or unique name)
	std::string baseName;   // base item name
	std::string quality;    // e.g. "identified"/"unidentified", "unique", "magic"
	std::string affixes;    // textual affix info (names only for now)
	int reqStr = 0;
	int reqMag = 0;
	int reqDex = 0;
	int ilvl = 0;
};

// Regenerates a best-effort item view using the packed seed/create-info and TSV data.
// This is intended for display in DiabloVault, not as a full game-accurate simulation.
UnpackedItemView RegenerateItemView(const ItemPack &pk, bool isHellfire);

} // namespace dv::d1
