#pragma once

#include <nbt_cpp/NBT_All.hpp>

#include <stdint.h>
#include <charconv>


namespace BlockStateUtil
{
	static const NBT_Type::String &BoolToNbtString(bool b) noexcept
	{
		static const NBT_Type::String BoolName[2] =
		{
			MU8STR("false"),//[0]
			MU8STR("true"),//[1]
		};

		return BoolName[(uint8_t)b];//bool->u8 index
	}
	
	static NBT_Type::String Uint8ToNbtString(uint8_t u8) noexcept
	{
		char cstrBuffer[4] = { 0 };//0~255 -> 最大3位 + '\0' -> 共4个元素
		auto [ptr, ec] = std::to_chars(std::begin(cstrBuffer), std::end(cstrBuffer), u8);
		if (ec != std::errc{})
		{
			return MU8STR("0");
		}
		size_t szBufferSize = ptr - &cstrBuffer[0];

		return NBT_Type::String{ std::basic_string_view{cstrBuffer, szBufferSize},NBT_Type::String::USE_RAW_DATA{} };
	}
}


struct NormalBlock
{
public:
	NBT_Type::String strBlockName;

public:
	NBT_Type::Compound ToCompound(void) const
	{
		return NBT_Type::Compound
		{
			{MU8STR("Name"), strBlockName},
		};
	}

};

const NormalBlock stAirBlock = NormalBlock{ MU8STR("minecraft:air")};
const NormalBlock stSmoothStoneBlock = NormalBlock{ MU8STR("minecraft:smooth_stone")};


struct DirectionBlock
{
public:
	enum class Direction : uint8_t
	{
		x,//default
		y,
		z,

		ENUM_END,//end flag
	};

	static inline const NBT_Type::String DirectionStr[] =
	{
		MU8STR("x"),
		MU8STR("y"),
		MU8STR("z"),
	};

public:
	static NBT_Type::String GetDirectionStr(Direction enDirection) noexcept
	{
		return (uint8_t)enDirection < (uint8_t)Direction::ENUM_END
			? DirectionStr[(uint8_t)enDirection]
			: DirectionStr[(uint8_t)Direction::x];
	}

public:
	NBT_Type::String strBlockName;
	Direction enDirection = Direction::x;

public:
	NBT_Type::Compound ToCompound(void) const
	{
		return NBT_Type::Compound
		{
			{MU8STR("Name"), strBlockName},
			{MU8STR("axis"), GetDirectionStr(enDirection)},
		};
	}

};

struct NoteBlock
{
public:
	static inline const NBT_Type::String strBlockName = MU8STR("minecraft:note_block");

	enum class Instrument : uint8_t
	{
		harp,//default
		bass,
		basedrum,
		snare,
		hat,
		guitar,
		flute,
		bell,
		chime,
		xylophone,
		iron_xylophone,
		cow_bell,
		didgeridoo,
		bit,
		banjo,
		pling,

		ENUM_END,//end flag
	};

	static inline const NBT_Type::String InstrumentStr[] =
	{
		MU8STR("harp"),
		MU8STR("bass"),
		MU8STR("basedrum"),
		MU8STR("snare"),
		MU8STR("hat"),
		MU8STR("guitar"),
		MU8STR("flute"),
		MU8STR("bell"),
		MU8STR("chime"),
		MU8STR("xylophone"),
		MU8STR("iron_xylophone"),
		MU8STR("cow_bell"),
		MU8STR("didgeridoo"),
		MU8STR("bit"),
		MU8STR("banjo"),
		MU8STR("pling"),
	};

	static inline const NBT_Type::Compound InstrumentBlock[] =
	{
		NormalBlock{MU8STR("minecraft:dirt")}.ToCompound(),
		NormalBlock{MU8STR("minecraft:oak_planks")}.ToCompound(),
		NormalBlock{MU8STR("minecraft:cobblestone")}.ToCompound(),
		NormalBlock{MU8STR("minecraft:sand")}.ToCompound(),
		NormalBlock{MU8STR("minecraft:glass")}.ToCompound(),
		NormalBlock{MU8STR("minecraft:white_wool")}.ToCompound(),
		NormalBlock{MU8STR("minecraft:clay")}.ToCompound(),
		NormalBlock{MU8STR("minecraft:gold_block")}.ToCompound(),
		NormalBlock{MU8STR("minecraft:packed_ice")}.ToCompound(),
		DirectionBlock{MU8STR("minecraft:bone_block"),DirectionBlock::Direction::y }.ToCompound(),
		NormalBlock{MU8STR("minecraft:iron_block")}.ToCompound(),
		NormalBlock{MU8STR("minecraft:soul_sand")}.ToCompound(),
		NormalBlock{MU8STR("minecraft:pumpkin")}.ToCompound(),
		NormalBlock{MU8STR("minecraft:emerald_block")}.ToCompound(),
		DirectionBlock{MU8STR("minecraft:hay_block"), DirectionBlock::Direction::y}.ToCompound(),
		NormalBlock{MU8STR("minecraft:glowstone")}.ToCompound(),
	};

public:
	static NBT_Type::Compound GetInstrumentBlock(Instrument enInstrument) noexcept
	{
		return (uint8_t)enInstrument < (uint8_t)Instrument::ENUM_END
			? InstrumentBlock[(uint8_t)enInstrument]
			: InstrumentBlock[(uint8_t)Instrument::harp];
	}

	static NBT_Type::String GetInstrumentStr(Instrument enInstrument) noexcept
	{
		return (uint8_t)enInstrument < (uint8_t)Instrument::ENUM_END
			? InstrumentStr[(uint8_t)enInstrument]
			: InstrumentStr[(uint8_t)Instrument::harp];
	}

	static NBT_Type::String GetNoteStr(uint8_t u8Note) noexcept
	{
		if (u8Note > 24)//音符盒只有0~24挡位
		{
			u8Note = 0;//默认挡位
		}

		return BlockStateUtil::Uint8ToNbtString(u8Note);
	}

	static NBT_Type::String GetPoweredStr(bool bPowered) noexcept
	{
		return BlockStateUtil::BoolToNbtString(bPowered);
	}

public:
	Instrument enInstrument = Instrument::harp;
	uint8_t u8Note = 0;//0~24
	bool bPowered = false;//t/f

public:
	NBT_Type::Compound ToCompound(void) const
	{
		return NBT_Type::Compound
		{
			{MU8STR("Name"), strBlockName},
			{MU8STR("Properties"), NBT_Type::Compound
				{
					{MU8STR("instrument"),GetInstrumentStr(enInstrument)},
					{MU8STR("note"), GetNoteStr(u8Note)},
					{MU8STR("powered"),GetPoweredStr(bPowered)},
				},
			},
		};
	}

};


struct RepeaterBlock
{
public:
	static inline const NBT_Type::String strBlockName = MU8STR("minecraft:repeater");

	enum class Facing : uint8_t
	{
		north,//default
		east,
		south,
		west,

		ENUM_END,//end flag
	};

	static inline const NBT_Type::String FacingStr[] =
	{
		MU8STR("north"),
		MU8STR("east"),
		MU8STR("south"),
		MU8STR("west"),
	};

public:
	uint8_t u8Delay = 1;
	Facing enFacing = Facing::north;
	bool bLocked = false;
	bool bPowered = false;

public:
	static NBT_Type::String GetDelayStr(uint8_t u8Delay) noexcept
	{
		if (u8Delay < 1 || u8Delay > 4)//中继器只有1、2、3、4挡位
		{
			u8Delay = 1;//默认挡位
		}

		return BlockStateUtil::Uint8ToNbtString(u8Delay);
	}

	static NBT_Type::String GetFacingStr(Facing enFacing) noexcept
	{
		return (uint8_t)enFacing < (uint8_t)Facing::ENUM_END
			? FacingStr[(uint8_t)enFacing]
			: FacingStr[(uint8_t)Facing::north];
	}

	static NBT_Type::String GetLockedStr(bool bLocked) noexcept
	{
		return BlockStateUtil::BoolToNbtString(bLocked);
	}

	static NBT_Type::String GetPoweredStr(bool bPowered) noexcept
	{
		return BlockStateUtil::BoolToNbtString(bPowered);
	}

public:
	NBT_Type::Compound ToCompound(void) const
	{
		return NBT_Type::Compound
		{
			{MU8STR("Name"), strBlockName},
			{MU8STR("Properties"), NBT_Type::Compound
				{
					{MU8STR("delay"), GetDelayStr(u8Delay)},
					{MU8STR("facing"), GetFacingStr(enFacing)},
					{MU8STR("locked"), GetLockedStr(bLocked)},
					{MU8STR("powered"), GetPoweredStr(bPowered)},
				},
			},
		};
	}

};
