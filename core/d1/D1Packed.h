// D1Packed.h
#pragma once

#include <cstdint>

namespace dv::d1 {

// These are the legacy packed structs used by Diablo/DevilutionX "hero" save files.
// We duplicate only what DiabloVault needs (name + basic packed items).

constexpr int PlayerNameLength = 32;
constexpr int InventoryGridCells = 40;
constexpr int MaxBeltItems = 8;
constexpr int NumInvLoc = 7;

#pragma pack(push, 1)

struct ItemPack {
	uint32_t iSeed;
	uint16_t iCreateInfo;
	uint16_t idx;
	uint8_t bId;
	uint8_t bDur;
	uint8_t bMDur;
	uint8_t bCh;
	uint8_t bMCh;
	uint16_t wValue;
	uint32_t dwBuff;
};

struct PlayerPack {
	uint32_t dwLowDateTime;
	uint32_t dwHighDateTime;
	int8_t destAction;
	int8_t destParam1;
	int8_t destParam2;
	uint8_t plrlevel;
	uint8_t px;
	uint8_t py;
	uint8_t targx;
	uint8_t targy;
	char pName[PlayerNameLength];
	uint8_t pClass;
	uint8_t pBaseStr;
	uint8_t pBaseMag;
	uint8_t pBaseDex;
	uint8_t pBaseVit;
	uint8_t pLevel;
	uint8_t pStatPts;
	uint32_t pExperience;
	int32_t pGold;
	int32_t pHPBase;
	int32_t pMaxHPBase;
	int32_t pManaBase;
	int32_t pMaxManaBase;
	uint8_t pSplLvl[37];
	uint64_t pMemSpells;
	ItemPack InvBody[NumInvLoc];
	ItemPack InvList[InventoryGridCells];
	int8_t InvGrid[InventoryGridCells];
	uint8_t _pNumInv;
	ItemPack SpdList[MaxBeltItems];
	int8_t pTownWarps;
	int8_t pDungMsgs;
	int8_t pLvlLoad;
	uint8_t pBattleNet;
	uint8_t pManaShield;
	uint8_t pDungMsgs2;
	int8_t bIsHellfire;
	uint8_t reserved;
	uint16_t wReflections;
	uint8_t reserved2[2];
	uint8_t pSplLvl2[10];
	int16_t wReserved8;
	uint32_t pDiabloKillLevel;
	uint32_t pDifficulty;
	uint32_t pDamAcFlags;
	uint8_t reserved3[20];
};

#pragma pack(pop)

} // namespace dv::d1
