#pragma once

#include "NBS_File.hpp"
#include "NBS_Endian.hpp"

#include <stddef.h>
#include <concepts>
#include <algorithm>

namespace NBS_Writer_Helper
{
	template<typename T>
	concept OutputStreamLike = requires(T & t, const void *pData, size_t szSize)
	{
		{
			t.AddReserve(szSize)
		} -> std::same_as<bool>;
		{
			t.PutRange(pData, szSize)
		} -> std::same_as<bool>;
	};
}

class NBS_Writer
{
	NBS_Writer(void) = delete;
	~NBS_Writer(void) = delete;

private:
	template<typename T, typename Stream>
	requires(NBS_File::IsNumeric_Type<T>)
	static bool WriteToStream(const T &tData, Stream &tStream)
	{
		T tTmp = NBS_Endian::NativeToLittleAny(tData);

		if (!tStream.AddReserve(sizeof(tTmp)) ||
			!tStream.PutRange((char *)&tTmp, sizeof(T)))
		{
			return false;
		}

		return true;
	}

	template<typename Stream>
	static bool WriteToStream(const NBS_File::STR &strData, Stream &tStream)
	{
		if (strData.size() * sizeof(*strData.data()) > UINT32_MAX)
		{
			return false;
		}

		NBS_File::INT u32Length = (NBS_File::INT)strData.size() * sizeof(*strData.data());
		if (!WriteToStream<NBS_File::INT>(u32Length, tStream))
		{
			return false;
		}

		if (!tStream.AddReserve(u32Length) ||
			!tStream.PutRange(strData.data(), u32Length * sizeof(*strData.data())))
		{
			return false;
		}

		return true;
	}

private:
#define FAIL_RETURN(func)\
do\
{\
	if(!(func))\
	{\
		return false;\
	}\
}while(false)

#define NORM_WRITE(field)\
FAIL_RETURN(WriteToStream((field), tStream))

#define COND_WRITE(field,cond)\
do\
{\
	if((cond))\
	{\
		FAIL_RETURN(WriteToStream((field), tStream));\
	}\
}while(false)

	template<typename Stream>
	static bool WriteHeader(const NBS_File::Header &header, Stream &tStream)
	{
		// 写入歌曲长度标记
		if (header.version > 0)
		{
			NORM_WRITE((NBS_File::SHORT)0);//新版本格式标记为0
			NORM_WRITE(header.version);
			NORM_WRITE(header.default_instruments);
			COND_WRITE(header.song_length, header.version >= 3);//如果还大于3，那么存在
		}
		else// header.version == 0
		{
			NORM_WRITE(header.song_length);
		}

		//写入信息字段
		NORM_WRITE(header.song_layers);

		//写入字符串字段
		NORM_WRITE(header.song_name);
		NORM_WRITE(header.song_author);
		NORM_WRITE(header.original_author);
		NORM_WRITE(header.description);

		//写入数值字段
		NORM_WRITE(header.tempo);
		NORM_WRITE(header.auto_save);
		NORM_WRITE(header.auto_save_duration);
		NORM_WRITE(header.time_signature);

		NORM_WRITE(header.minutes_spent);
		NORM_WRITE(header.left_clicks);
		NORM_WRITE(header.right_clicks);
		NORM_WRITE(header.blocks_added);
		NORM_WRITE(header.blocks_removed);
		NORM_WRITE(header.song_origin);

		//写入v4及以上字段
		if (header.version >= 4)
		{
			NORM_WRITE(header.loop);
			NORM_WRITE(header.max_loop_count);
			NORM_WRITE(header.loop_start);
		}

		return true;
	}

	template<typename Stream>
	static bool WriteNotes(const NBS_File::Header &header, const NBS_File::ListNote &listNote, Stream &tStream)
	{
		// 按tick排序音符，同tick按照layer排序
		std::vector<NBS_File::Note> sortedNoteList = listNote;
		size_t szSortedNoteSize = sortedNoteList.size();
		std::sort(sortedNoteList.begin(), sortedNoteList.end(),
			[](const NBS_File::Note &a, const NBS_File::Note &b)
			{
				return a.tick != b.tick ? a.tick < b.tick : a.layer < b.layer;
			}
		);

		//遍历tick
		size_t noteIndex = 0;
		NBS_File::LONG cur_tick = -1;
		while (noteIndex < szSortedNoteSize)
		{
			//获取当前音符的tick
			NBS_File::LONG tick = sortedNoteList[noteIndex].tick;

			//写入tick相对偏移（SHORT）
			NORM_WRITE((NBS_File::SHORT)(tick - cur_tick));
			cur_tick = tick;

			//写入当前tick的所有音符作为一个layer层
			NBS_File::SHORT cur_layer = -1;
			do
			{
				const auto &note = sortedNoteList[noteIndex];

				//写入layer偏移
				NORM_WRITE((NBS_File::SHORT)(note.layer - cur_layer));
				cur_layer = note.layer;

				//写入音符数据
				NORM_WRITE(note.instrument);
				NORM_WRITE(note.key);

				//v4数据
				if (header.version >= 4)
				{
					NORM_WRITE(note.velocity);
					NORM_WRITE(note.panning);
					NORM_WRITE(note.pitch);
				}

				++noteIndex;
			} while (noteIndex < szSortedNoteSize && sortedNoteList[noteIndex].tick == tick);//第一次肯定不会失败，使用do while

			//写入layer结束标记
			NORM_WRITE((NBS_File::SHORT)0);
		}
		//写入tick结束标记
		NORM_WRITE((NBS_File::SHORT)0);

		return true;
	}

	template<typename Stream>
	static bool WriteLayers(const NBS_File::Header &header, const NBS_File::ListLayer &listLayer, Stream &tStream)
	{
		//写入层
		for (const auto &layer : listLayer)
		{
			NORM_WRITE(layer.name);
			COND_WRITE(layer.lock, header.version >= 4);
			NORM_WRITE(layer.volume);
			COND_WRITE(layer.panning, header.version >= 2);
		}

		return true;
	}

	template<typename Stream>
	static bool WriteInstruments(const NBS_File::Header &header, const NBS_File::ListInstrument &listInstrument, Stream &tStream)
	{
		//写入乐器数量
		if (listInstrument.size() > UINT8_MAX)
		{
			return false;
		}
		NORM_WRITE((NBS_File::BYTE)listInstrument.size());

		//写入每个乐器
		for (const auto &instrument : listInstrument)
		{
			NORM_WRITE(instrument.name);
			NORM_WRITE(instrument.file);
			NORM_WRITE(instrument.pitch);
			NORM_WRITE(instrument.press_key);
		}

		return true;
	}

public:
	template<typename Stream>
	requires(NBS_Writer_Helper::OutputStreamLike<Stream>)
	static bool WriteNBS(const NBS_File &fileNBS, Stream &tStream)
	{
		return
			WriteHeader(fileNBS.header, tStream) &&
			WriteNotes(fileNBS.header, fileNBS.listNote, tStream) &&
			WriteLayers(fileNBS.header, fileNBS.listLayer, tStream) &&
			WriteInstruments(fileNBS.header, fileNBS.listInstrument, tStream);
	}

#undef NORM_WRITE
#undef COND_WRITE
#undef FAIL_RETURN
};
