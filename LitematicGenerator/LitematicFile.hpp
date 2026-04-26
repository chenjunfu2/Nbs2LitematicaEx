#pragma once

#include "BlockState.hpp"

#include <nbt_cpp/NBT_All.hpp>

#include <vector>
#include <unordered_map>

struct LitematicFile
{
public:
	struct Vec3I
	{
	public:
		NBT_Type::Int x;
		NBT_Type::Int y;
		NBT_Type::Int z;

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

	struct MetaData
	{
	public:
		Vec3I stEnclosingSize;//Compound
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
		NBT_Type::List listBlockStatePalette;//仅接受已转换为NBT_Compound的方块，方块对象可复用
		NBT_Type::List listEntities;
		NBT_Type::List listPendingBlockTicks;
		NBT_Type::List listPendingFluidTicks;
		NBT_Type::List listTileEntities;
		NBT_Type::LongArray larrBlockStates;

	public:
		NBT_Type::Compound ToCompound(void) const &
		{
			return NBT_Type::Compound
			{
				{MU8STR("Position"),			stPosition.ToCompound()},
				{MU8STR("Size"),				stSize.ToCompound()},
				{MU8STR("BlockStatePalette"),	listBlockStatePalette},
				{MU8STR("Entities"),			listEntities},
				{MU8STR("PendingBlockTicks"),	listPendingBlockTicks},
				{MU8STR("PendingFluidTicks"),	listPendingFluidTicks},
				{MU8STR("TileEntities"),		listTileEntities},
				{MU8STR("BlockStates"),			larrBlockStates},
			};
		}

		NBT_Type::Compound ToCompound(void) &&
		{
			return NBT_Type::Compound
			{
				{MU8STR("Position"),			std::move(stPosition).ToCompound()},
				{MU8STR("Size"),				std::move(stSize).ToCompound()},
				{MU8STR("BlockStatePalette"),	std::move(listBlockStatePalette)},
				{MU8STR("Entities"),			std::move(listEntities)},
				{MU8STR("PendingBlockTicks"),	std::move(listPendingBlockTicks)},
				{MU8STR("PendingFluidTicks"),	std::move(listPendingFluidTicks)},
				{MU8STR("TileEntities"),		std::move(listTileEntities)},
				{MU8STR("BlockStates"),			std::move(larrBlockStates)},
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
