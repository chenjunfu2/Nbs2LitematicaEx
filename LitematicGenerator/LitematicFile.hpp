#pragma once

#include "BlockState.hpp"

#include <nbt_cpp/NBT_All.hpp>

#include <vector>
#include <unordered_map>

struct Vec3I
{
public:
	NBT_Type::Int x{};
	NBT_Type::Int y{};
	NBT_Type::Int z{};

public:
	NBT_Type::Compound ToCompound(void) const//builtin无需move特化
	{
		return NBT_Type::Compound
		{
			{MU8STR("x"), x},
			{MU8STR("y"), y},
			{MU8STR("z"), z},
		};
	}
};

struct LitematicaBlocks
{
public:
	size_t szLayerSize;//一层方块数 = x*z
	size_t szTotalSize;//总方块数 = x*y*z
	size_t szXSize;//x大小

	size_t szBitsPerEntry;//调色板索引占用的位数
	size_t szMaxEntryValue;//当前占用的位数可存储的最大值（用于位掩码）

	NBT_Type::List listBlockStatePalette;//方块调色板
	NBT_Type::LongArray larrBlockStates;//方块位数组

public:
	//计算前导0数量
	static uint32_t NumberOfLeadingZeros(uint32_t i)
	{
		if (i == 0)
		{
			return 32;
		}

		uint32_t n = 31;
		if (i >= 1 << 16)
		{
			n -= 16;
			i >>= 16;
		}

		if (i >= 1 << 8)
		{
			n -= 8;
			i >>= 8;
		}

		if (i >= 1 << 4)
		{
			n -= 4;
			i >>= 4;
		}

		if (i >= 1 << 2)
		{
			n -= 2;
			i >>= 2;
		}

		return n - (i >> 1);
	}

	uint64_t RoundUpToPowerOfTwo(uint64_t value, uint64_t interval)
	{
		return (value + interval - 1) & ~(interval - 1);
	}

public:
	//初始化后，调色板需自行赋值
	void Init(size_t szPaletteSize, Vec3I v3iRegionSize)
	{
		szLayerSize = (size_t)v3iRegionSize.x * (size_t)v3iRegionSize.z;
		szTotalSize = szLayerSize * (size_t)v3iRegionSize.y;
		szXSize = v3iRegionSize.x;

		//调色板大小-1后，用32-二进制最高位的前导0位数，得出调色板内的方块数量至少需要多少bit才能存储，这一步是为了获取位索引最大大小，max为了确保至少为2bit
		szBitsPerEntry = std::max((uint32_t)2, (uint32_t)32 - NumberOfLeadingZeros(szPaletteSize - 1));
		szMaxEntryValue = ((size_t)1 << szBitsPerEntry) - 1;
		//计算两个数组大小
		listBlockStatePalette.Reserve(szPaletteSize);
		larrBlockStates.resize(RoundUpToPowerOfTwo(szTotalSize * szBitsPerEntry, 64) / 64, 0);
	}

	size_t GetSpatialIndex(Vec3I v3iCoord)
	{
		return (v3iCoord.y * szLayerSize) + v3iCoord.z * szXSize + v3iCoord.x;
	}

	void SetBlock(size_t szSpatialIndex, size_t szBlockPaletteIndex)
	{
		size_t szStartOffset = szSpatialIndex * szBitsPerEntry;
		size_t szStartArrIndex = szStartOffset / 64;
		size_t szEndArrIndex = (((szSpatialIndex + 1) * szBitsPerEntry - 1) >> 6);
		size_t szStartBitOffset = szStartOffset % 64;
		larrBlockStates[szStartArrIndex] = larrBlockStates[szStartArrIndex] & ~(szMaxEntryValue << szStartBitOffset) | (szBlockPaletteIndex & szMaxEntryValue) << szStartBitOffset;

		if (szStartArrIndex != szEndArrIndex)
		{
			size_t szEndOffset = 64 - szStartBitOffset;
			size_t szClearLowBitsOffset = szBitsPerEntry - szEndOffset;
			larrBlockStates[szEndArrIndex] = larrBlockStates[szEndArrIndex] >> szClearLowBitsOffset << szClearLowBitsOffset | (szBlockPaletteIndex & szMaxEntryValue) >> szEndOffset;
		}
	}

	size_t GetBlock(size_t szSpatialIndex)
	{
		size_t szStartOffset = szSpatialIndex * szBitsPerEntry;
		size_t szStartArrIndex = szStartOffset / 64;
		size_t szEndArrIndex = (((szSpatialIndex + 1L) * szBitsPerEntry - 1L) >> 6);
		size_t szStartBitOffset = szStartOffset % 64;

		if (szStartArrIndex == szEndArrIndex)
		{
			return (larrBlockStates[szStartArrIndex] >> szStartBitOffset & szMaxEntryValue);
		}
		else
		{
			size_t szEndOffset = 64 - szStartBitOffset;
			return ((larrBlockStates[szStartArrIndex] >> szStartBitOffset | larrBlockStates[szEndArrIndex] << szEndOffset) & szMaxEntryValue);
		}
	}

};

struct LitematicFile
{
public:
	struct MetaData
	{
	public:
		Vec3I stEnclosingSize;
		NBT_Type::String strAuthor;
		NBT_Type::String strDescription;
		NBT_Type::String strName;
		NBT_Type::Int iRegionCount;
		NBT_Type::Long lTimeCreated;
		NBT_Type::Long lTimeModified;
		NBT_Type::Int iTotalBlocks;
		NBT_Type::Int iTotalVolume;

	public:
		NBT_Type::Compound ToCompound(void) const &
		{
			return NBT_Type::Compound
			{
				{MU8STR("EnclosingSize"),	stEnclosingSize.ToCompound()},
				{MU8STR("Author"),			strAuthor},
				{MU8STR("Description"),		strDescription},
				{MU8STR("Name"),			strName},
				{MU8STR("RegionCount"),		iRegionCount},
				{MU8STR("TimeCreated"),		lTimeCreated},
				{MU8STR("TimeModified"),	lTimeModified},
				{MU8STR("TotalBlocks"),		iTotalBlocks},
				{MU8STR("TotalVolume"),		iTotalVolume},
			};
		}

		NBT_Type::Compound ToCompound(void) &&
		{
			return NBT_Type::Compound
			{
				{MU8STR("EnclosingSize"),	std::move(stEnclosingSize).ToCompound()},
				{MU8STR("Author"),			std::move(strAuthor)},
				{MU8STR("Description"),		std::move(strDescription)},
				{MU8STR("Name"),			std::move(strName)},
				{MU8STR("RegionCount"),		iRegionCount},//builtin无需move
				{MU8STR("TimeCreated"),		lTimeCreated},
				{MU8STR("TimeModified"),	lTimeModified},
				{MU8STR("TotalBlocks"),		iTotalBlocks},
				{MU8STR("TotalVolume"),		iTotalVolume},
			};
		}
	};

	struct Region
	{
	public:
		Vec3I stPosition;
		Vec3I stSize;
		LitematicaBlocks stBlocks;
		NBT_Type::List listEntities;
		NBT_Type::List listTileEntities;
		NBT_Type::List listPendingBlockTicks;
		NBT_Type::List listPendingFluidTicks;

	public:
		NBT_Type::Compound ToCompound(void) const &
		{
			return NBT_Type::Compound
			{
				{MU8STR("Position"),			stPosition.ToCompound()},
				{MU8STR("Size"),				stSize.ToCompound()},
				{MU8STR("BlockStatePalette"),	stBlocks.listBlockStatePalette},
				{MU8STR("BlockStates"),			stBlocks.larrBlockStates},
				{MU8STR("Entities"),			listEntities},
				{MU8STR("TileEntities"),		listTileEntities},
				{MU8STR("PendingBlockTicks"),	listPendingBlockTicks},
				{MU8STR("PendingFluidTicks"),	listPendingFluidTicks},
			};
		}

		NBT_Type::Compound ToCompound(void) &&
		{
			return NBT_Type::Compound
			{
				{MU8STR("Position"),			std::move(stPosition).ToCompound()},
				{MU8STR("Size"),				std::move(stSize).ToCompound()},
				{MU8STR("BlockStatePalette"),	std::move(stBlocks.listBlockStatePalette)},
				{MU8STR("BlockStates"),			std::move(stBlocks.larrBlockStates)},
				{MU8STR("Entities"),			std::move(listEntities)},
				{MU8STR("TileEntities"),		std::move(listTileEntities)},
				{MU8STR("PendingBlockTicks"),	std::move(listPendingBlockTicks)},
				{MU8STR("PendingFluidTicks"),	std::move(listPendingFluidTicks)},
			};
		}

	};

	struct Regions
	{
	public:
		std::unordered_map<NBT_Type::String, Region> mapRegion;

	public:
		NBT_Type::Compound ToCompound(void) const &
		{
			NBT_Type::Compound cpdRet;
			for (const auto &[regionName, regionData] : mapRegion)
			{
				cpdRet.PutCompound(regionName, regionData.ToCompound());
			}
			return cpdRet;
		}

		NBT_Type::Compound ToCompound(void) &&
		{
			NBT_Type::Compound cpdRet;
			for (auto &[regionName, regionData] : mapRegion)
			{
				cpdRet.PutCompound(regionName, std::move(regionData).ToCompound());
			}

			mapRegion.clear();//move后需要清理空键值
			return cpdRet;
		}

	};


public:
	MetaData stMetaData;
	Regions stRegions;
	NBT_Type::Int iMinecraftDataVersion = 3465;//1.20.1 -> 3465
	NBT_Type::Int iSubVersion = 1;//Litematic SubVersion
	NBT_Type::Int iVersion = 6;//Litematic Version

public:
	NBT_Type::Compound ToCompound(void) const &
	{
		return NBT_Type::Compound
		{
			{MU8STR("Metadata"),				stMetaData.ToCompound()},
			{MU8STR("Regions"),					stRegions.ToCompound()},
			{MU8STR("MinecraftDataVersion"),	iMinecraftDataVersion},
			{MU8STR("SubVersion"),				iSubVersion},
			{MU8STR("Version"),					iVersion},
		};
	}

	NBT_Type::Compound ToCompound(void) &&
	{
		return NBT_Type::Compound
		{
			{MU8STR("Metadata"),				std::move(stMetaData).ToCompound()},
			{MU8STR("Regions"),					std::move(stRegions).ToCompound()},
			{MU8STR("MinecraftDataVersion"),	iMinecraftDataVersion},//builtin无需move
			{MU8STR("SubVersion"),				iSubVersion},
			{MU8STR("Version"),					iVersion},
		};
	}

};






