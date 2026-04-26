#pragma once

#include "BlockState.hpp"

#include <nbt_cpp/NBT_All.hpp>
#include <nbs_cpp/NBS_All.hpp>

#include <vector>

struct LitematicFile
{
public:
	struct MetaData
	{




	public:
		NBT_Type::Compound ToCompound(void)
		{
			return {};//TODO
		}

	};

	struct Region
	{



	public:
		NBT_Type::Compound ToCompound(void)
		{
			return {};//TODO
		}

	};

	using Regions = std::vector<Region>;

public:
	MetaData stMetaData;
	Regions listRegion;
	NBT_Type::Int iMinecraftDataVersion = 3465;//1.20.1 -> 3465
	NBT_Type::Int iVersion = 6;//Litematic Version
	NBT_Type::Int iSubVersion = 1;//Litematic SubVersion

public:
	NBT_Type::Compound ToCompound(void)
	{
		return {};//TODO
	}

};
