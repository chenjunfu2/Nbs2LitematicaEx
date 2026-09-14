#pragma once

#include <nbs_cpp/NBS_File.hpp>

#include <stdint.h>
#include <stdexcept>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

struct MyNote
{
	NBS_File::LONG tick;
	NBS_File::BYTE instrument;
	NBS_File::BYTE key;
};

struct MyNote2
{
public:
	enum class Type : uint8_t
	{
		Note,
		Blank,
	};

public:
	const Type enType;
	union
	{
		struct
		{
			const NBS_File::BYTE instrument;
			const NBS_File::BYTE key;
		};
		struct
		{
			const NBS_File::SHORT tick;//delta tick
		};
	};

public:
	MyNote2(NBS_File::BYTE _instrument, NBS_File::BYTE _key) : enType(Type::Note), instrument(_instrument), key(_key)
	{}

	MyNote2(NBS_File::SHORT _tick) : enType(Type::Blank), tick(_tick)
	{}

	~MyNote2(void) = default;
public:
	bool operator==(const MyNote2 &_Right) const
	{
		switch (enType)
		{
		case MyNote2::Type::Note:
			return instrument == _Right.instrument && key == _Right.key;
			break;
		case MyNote2::Type::Blank:
			return tick == _Right.tick;
			break;
		default:
			throw std::runtime_error("Switched Unknown Type!");
			break;
		}
	}

	bool operator!=(const MyNote2 &_Right) const
	{
		switch (enType)
		{
		case MyNote2::Type::Note:
			return instrument != _Right.instrument || key != _Right.key;
			break;
		case MyNote2::Type::Blank:
			return tick != _Right.tick;
			break;
		default:
			throw std::runtime_error("Switched Unknown Type!");
			break;
		}
	}

	std::strong_ordering operator<=>(const MyNote2 &_Right) const
	{
		switch (enType)
		{
		case MyNote2::Type::Note:
			if (auto tmp = instrument <=> _Right.instrument; tmp != 0)
			{
				return tmp;
			}
			else
			{
				return key <=> _Right.key;
			}
			break;
		case MyNote2::Type::Blank:
			return tick <=> _Right.tick;
			break;
		default:
			throw std::runtime_error("Switched Unknown Type!");
			break;
		}
	}

	size_t Hash(void) const
	{
		switch (enType)
		{
		case MyNote2::Type::Note:
			{
				return std::hash<uint16_t>{}((uint16_t)instrument << 8 | (uint16_t)key << 0);
			}
			break;
		case MyNote2::Type::Blank:
			{
				return std::hash<uint16_t>{}(tick);
			}
			break;
		default:
			throw std::runtime_error("Switched Unknown Type!");
			break;
		}
	}
};


namespace std
{
	template <>// 特化 hash
	struct hash<MyNote2>
	{
		size_t operator()(const MyNote2 &_Hash) const noexcept
		{
			return _Hash.Hash();
		}
	};
}

//--------------------------------------------------------------------------------//

using MyNoteList = std::vector<MyNote>;
using NoteLayerList = std::vector<MyNoteList>;

NoteLayerList ToMyNoteList(const NBS_File &fNBS, bool bNoExcludeSpace)
{
	size_t szLayerSize = fNBS.listLayer.size();

	NoteLayerList listNoteLayer;
	listNoteLayer.resize(szLayerSize);

	for (auto &it : fNBS.listNote)
	{
		if (it.layer >= szLayerSize && fNBS.listLayer[it.layer].lock != 0)
		{
			continue;//溢出或被静音，跳过
		}

		listNoteLayer[it.layer].emplace_back(it.tick, it.instrument, it.key);
	}

	if (!bNoExcludeSpace)
	{
		//裁剪静音的层
		size_t last = 0;
		for (size_t i = 0; i < szLayerSize; ++i)
		{
			if (!listNoteLayer[i].empty())
			{
				if (last != i)
				{
					listNoteLayer[last] = std::move(listNoteLayer[i]);//把找到的不为静音的层替换
				}
				++last;//移动到下一个位置
			}
		}

		listNoteLayer.resize(last);
	}
	
	return listNoteLayer;
}

using MyNoteList2 = std::vector<MyNote2>;
using NoteLayerList2 = std::vector<MyNoteList2>;

NoteLayerList2 ToMyNoteList2(const NoteLayerList &listNoteLayer)//排序，处理空白
{
	NoteLayerList2 listNoteLayer2;
	listNoteLayer2.reserve(listNoteLayer.size());

	//处理每一层的音符
	for (const auto &noteLayer : listNoteLayer)
	{
		MyNoteList sortedListNote = noteLayer;

		//排序所有音符（按照tick序-instrument序-key序）
		std::ranges::sort(sortedListNote,
			[](const MyNote &l, const MyNote &r) -> bool
			{
				if (l.tick != r.tick)
				{
					return l.tick < r.tick;
				}
				else if (l.instrument != r.instrument)
				{
					return l.instrument < r.instrument;
				}
				else
				{
					return l.key < r.key;
				}
			}
		);
		size_t szSortedNoteSize = sortedListNote.size();

		MyNoteList2 listNote2;
		listNote2.reserve(szSortedNoteSize);

		//计算tick差值，作为空白音符
		NBS_File::LONG last_tick = 0;
		for (size_t noteIndex = 0; noteIndex < szSortedNoteSize; ++noteIndex)
		{
			const auto &curNote = sortedListNote[noteIndex];

			//插入空白，如果空白为0则跳过
			NBS_File::SHORT deleta_tick = (NBS_File::SHORT)(curNote.tick - last_tick);
			last_tick = curNote.tick;
			if (deleta_tick != 0)
			{
				listNote2.emplace_back(deleta_tick);//tick是绝对值的差值
			}

			//插入音符
			listNote2.emplace_back(curNote.instrument, curNote.key);
		}

		listNoteLayer2.push_back(std::move(listNote2));
	}

	return listNoteLayer2;
}

//离散化映射
struct NoteVal
{
	std::unordered_map<MyNote2, size_t> mapNote2Index;
	std::vector<MyNote2> listNote2Map;
	std::vector<size_t> listEncodeNote2;

	std::unordered_map<NBS_File::BYTE, size_t> mapInstrumentIndex;
	std::vector<NBS_File::BYTE> listInstrumentMap;
	std::vector<size_t> listEncodeInstrument;
};

NoteVal ToNoteVal(const MyNoteList2 &listNote2)
{
	NoteVal valNote;
	for (const auto &it : listNote2)
	{
		//空白丢弃
		if (it.enType == MyNote2::Type::Blank)
		{
			continue;//丢弃空白，空白不作为音符出现
		}

		//音域限制
		if (it.enType == MyNote2::Type::Note &&
			(it.key < 33 || it.key > 33 + 24))
		{
			continue;//丢弃超出音域的音符
		}

		//统计不同的音色种类
		{
			auto [itEmplace, bSuccess] = valNote.mapInstrumentIndex.try_emplace(it.instrument, valNote.listInstrumentMap.size());
			if (bSuccess)
			{
				valNote.listInstrumentMap.push_back(it.instrument);
			}
			valNote.listEncodeInstrument.push_back(itEmplace->second);
		}
		
		{
			//统计不同的音符盒种类并分配调色板id
			auto [itEmplace, bSuccess] = valNote.mapNote2Index.try_emplace(it, valNote.listNote2Map.size());//如果插入成功，那么size相当于index（因为下面插入）
			if (bSuccess)
			{
				valNote.listNote2Map.push_back(it);
			}
			//如果插入失败，那么itEmplace获得的是阻止插入的元素，相当于查找到它的映射，否则是刚才插入的元素
			valNote.listEncodeNote2.push_back(itEmplace->second);//插入编码数组中，这样编码数组利用size值替代了listNote2
		}
	}

	return valNote;
}
