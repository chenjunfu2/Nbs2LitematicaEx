#pragma once

#include "BlockState.hpp"

#include <nbt_cpp/NBT_All.hpp>
#include <nbs_cpp/NBS_All.hpp>

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
		NBT_Type::Compound ToCompound(void)
		{
			return {};//TODO
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
		NBT_Type::Compound ToCompound(void)
		{
			return {};//TODO
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
		NBT_Type::Compound ToCompound(void)
		{
			return {};//TODO
		}

	};

	struct Regions
	{
	public:
		std::unordered_map<NBT_Type::String, Region> mapRegion;

	public:
		NBT_Type::Compound ToCompound(void)
		{
			return {};//TODO
		}

	};


public:
	MetaData stMetaData;
	Regions stRegions;
	NBT_Type::Int iMinecraftDataVersion = 3465;//1.20.1 -> 3465
	NBT_Type::Int iVersion = 6;//Litematic Version
	NBT_Type::Int iSubVersion = 1;//Litematic SubVersion

public:
	NBT_Type::Compound ToCompound(void)
	{
		return {};//TODO
	}

};
