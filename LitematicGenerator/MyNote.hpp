#pragma once

#include <nbs_cpp/NBS_File.hpp>

#include <stdint.h>
#include <stdexcept>
#include <algorithm>
#include <unordered_map>

struct MyNote
{
	NBS_File::LONG tick;
	NBS_File::BYTE instrument;
	NBS_File::BYTE key;
};

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


MyNoteList MergeAndSortMyNoteList(const NoteLayerList &listNoteLayer)//排序，处理空白
{
	MyNoteList fullSortList{};

	for (const auto &noteLayer : listNoteLayer)
	{
		//合并所有音符
		fullSortList.insert(fullSortList.end(), noteLayer.begin(), noteLayer.end());
	}

	//排序所有音符（按照tick序-instrument序-key序）
	std::ranges::sort(fullSortList,
		[](const MyNote &l, const MyNote &r) -> bool
		{
			if (l.tick != r.tick)
			{
				return l.tick < r.tick;
			}

			if (l.key != r.key)
			{
				return l.key < r.key;
			}

			return l.instrument < r.instrument;
		}
	);

	return fullSortList;
}

//离散化映射
struct NoteVal2
{
	std::unordered_map<NBS_File::BYTE, size_t> mapInstrumentIndex;//音色->调色板索引
	std::vector<NBS_File::BYTE> listInstrumentMap;//调色板索引->音色
	//std::vector<size_t> listEncodeInstrument;//调色板索引形式的音符序列
};

NoteVal2 ToNoteVal2(const MyNoteList &listNote)
{
	NoteVal2 valNote;
	for (const auto &it : listNote)
	{
		//音域限制
		if (it.key < 33 || it.key > 33 + 24)
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
			//valNote.listEncodeInstrument.push_back(itEmplace->second);
		}
	}

	return valNote;
}

