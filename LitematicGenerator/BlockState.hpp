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


struct AirBlock
{
public:
	static inline const NBT_Type::String strBlockName = MU8STR("minecraft:air");

public:
	NBT_Type::Compound ToCompound(void)
	{
		return NBT_Type::Compound
		{
			{MU8STR("Name"), strBlockName},
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

	static inline const NBT_Type::String InstrumentName[] =
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

public:
	static NBT_Type::String GetInstrumentName(Instrument enInstrument) noexcept
	{
		return (uint8_t)enInstrument < (uint8_t)Instrument::ENUM_END
			? InstrumentName[(uint8_t)enInstrument]
			: InstrumentName[(uint8_t)Instrument::harp];
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
	NBT_Type::Compound ToCompound(void)
	{
		return NBT_Type::Compound
		{
			{MU8STR("Name"), strBlockName},
			{MU8STR("Properties"), NBT_Type::Compound
				{
					{MU8STR("instrument"),GetInstrumentName(enInstrument)},
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

	static inline const NBT_Type::String FacingName[] =
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

	static NBT_Type::String GetFacingName(Facing enFacing) noexcept
	{
		return (uint8_t)enFacing < (uint8_t)Facing::ENUM_END
			? FacingName[(uint8_t)enFacing]
			: FacingName[(uint8_t)Facing::north];
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
	NBT_Type::Compound ToCompound(void)
	{
		return NBT_Type::Compound
		{
			{MU8STR("Name"), strBlockName},
			{MU8STR("Properties"), NBT_Type::Compound
				{
					{MU8STR("delay"), GetDelayStr(u8Delay)},
					{MU8STR("facing"), GetFacingName(enFacing)},
					{MU8STR("locked"), GetLockedStr(bLocked)},
					{MU8STR("powered"), GetPoweredStr(bPowered)},
				},
			},
		};
	}

};
