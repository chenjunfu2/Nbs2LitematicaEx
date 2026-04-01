#pragma once

#include "NBS_File.hpp"
#include "NBS_Endian.hpp"

#include <stddef.h>
#include <concepts>

namespace NBS_Reader_Helper
{
	template<typename T>
	concept InputStreamLike = requires(T & t, void *pData, size_t szSize)
	{
		{
			t.HasValidData(szSize)
		} -> std::same_as<bool>;
		{
			t.GetRange(pData, szSize)
		} -> std::same_as<bool>;
	};
}

class NBS_Reader
{
	NBS_Reader(void) = delete;
	~NBS_Reader(void) = delete;

private:
	template<typename T, typename Stream>
	requires(NBS_File::IsNumeric_Type<T>)
	static bool ReadFromStream(T &tData, Stream &tStream)
	{
		T tTmp{};
		if (!tStream.HasValidData(sizeof(tTmp)) ||
			!tStream.GetRange((void *)&tTmp, sizeof(tTmp)))
		{
			return false;
		}

		tData = NBS_Endian::LittleToNativeAny(tTmp);

		return true;
	}

	template<typename Stream>
	static bool ReadFromStream(NBS_File::STR &strData, Stream &tStream)
	{
		NBS_File::INT u32Length{};
		if (!ReadFromStream<NBS_File::INT>(u32Length, tStream))
		{
			return false;
		}

		strData.resize(u32Length);
		size_t szReadSize = u32Length * sizeof(*strData.data());

		if (!tStream.HasValidData(szReadSize) ||
			!tStream.GetRange((void *)strData.data(), szReadSize))
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

#define NORM_READ(field)\
FAIL_RETURN(ReadFromStream((field), tStream))

#define COND_READ(field,cond,defval)\
do\
{\
	if((cond))\
	{\
		FAIL_RETURN(ReadFromStream((field), tStream));\
	}\
	else\
	{\
		(field) = (defval);\
	}\
}while(false)

	template<typename Stream>
	static bool ReadHeader(NBS_File::Header &header, Stream &tStream)
	{
		NBS_File::SHORT song_length{};
		NORM_READ(song_length);

		NBS_File::BYTE version{};
		COND_READ(version, song_length == 0, (NBS_File::BYTE)0);//读入版本号，如果是新版本，则song_length长度为标记值0

		//版本太高，无法处理
		if (version > NBS_File::CURRENT_NBS_VERSION)
		{
			return false;
		}

		//初始化文件头
		header.version = version;

		COND_READ(header.default_instruments, version > 0, 10);
		COND_READ(header.song_length, version >= 3, song_length);
		NORM_READ(header.song_layers);

		NORM_READ(header.song_name);
		NORM_READ(header.song_author);
		NORM_READ(header.original_author);
		NORM_READ(header.description);

		NORM_READ(header.tempo);
		NORM_READ(header.auto_save);
		NORM_READ(header.auto_save_duration);
		NORM_READ(header.time_signature);

		NORM_READ(header.minutes_spent);
		NORM_READ(header.left_clicks);
		NORM_READ(header.right_clicks);
		NORM_READ(header.blocks_added);
		NORM_READ(header.blocks_removed);

		NORM_READ(header.song_origin);

		COND_READ(header.loop, version >= 4, (NBS_File::BOOL)false);
		COND_READ(header.max_loop_count, version >= 4, (NBS_File::BYTE)0);
		COND_READ(header.loop_start, version >= 4, (NBS_File::SHORT)0);

		return true;
	}

	template<typename Stream>
	static bool ReadNotes(const NBS_File::Header &header, NBS_File::ListNote &listNote, Stream &tStream)
	{
		listNote.clear();

		NBS_File::LONG cur_tick = -1;
		while (true)
		{
			NBS_File::SHORT delta_tick{};//读取使用相对偏移（SHORT）
			NORM_READ(delta_tick);
			if (delta_tick == 0)
			{
				break;
			}
			cur_tick += delta_tick;

			NBS_File::SHORT cur_layer = -1;
			while (true)
			{
				NBS_File::SHORT delta_layer{};
				NORM_READ(delta_layer);
				if (delta_layer == 0)
				{
					break;
				}
				cur_layer += delta_layer;

				NBS_File::Note note;
				note.tick = cur_tick;
				note.layer = cur_layer;
				NORM_READ(note.instrument);
				NORM_READ(note.key);
				COND_READ(note.velocity, header.version >= 4, (NBS_File::BYTE)100);
				COND_READ(note.panning, header.version >= 4, (NBS_File::BYTE)0);
				COND_READ(note.pitch, header.version >= 4, (NBS_File::SSHORT)0);

				listNote.push_back(std::move(note));
			}
		}

		return true;
	}

	template<typename Stream>
	static bool ReadLayers(const NBS_File::Header &header, NBS_File::ListLayer &listLayer, Stream &tStream)
	{
		listLayer.clear();
		listLayer.reserve(header.song_layers);

		for (NBS_File::SHORT i = 0; i < header.song_layers; ++i)
		{
			NBS_File::Layer layer;
			layer.id = i;
			NORM_READ(layer.name);
			COND_READ(layer.lock, header.version >= 4, (NBS_File::BOOL)false);
			NORM_READ(layer.volume);
			COND_READ(layer.panning, header.version >= 2, (NBS_File::BYTE)0);

			listLayer.push_back(std::move(layer));
		}

		return true;
	}

	template<typename Stream>
	static bool ReadInstruments(const NBS_File::Header &header, NBS_File::ListInstrument &listInstrument, Stream &tStream)
	{
		listInstrument.clear();

		NBS_File::BYTE u8InstrumentCount{};
		NORM_READ(u8InstrumentCount);

		for (NBS_File::BYTE i = 0; i < u8InstrumentCount; ++i)
		{
			NBS_File::Instrument instrument;
			instrument.id = i;
			NORM_READ(instrument.name);
			NORM_READ(instrument.file);
			NORM_READ(instrument.pitch);
			NORM_READ(instrument.press_key);

			listInstrument.push_back(std::move(instrument));
		}

		return true;
	}

public:
	template<typename Stream>
	requires(NBS_Reader_Helper::InputStreamLike<Stream>)
	static bool ReadNBS(NBS_File &fileNBS, Stream &tStream)
	{
		return
			ReadHeader(fileNBS.header, tStream) &&
			ReadNotes(fileNBS.header, fileNBS.listNote, tStream) &&
			ReadLayers(fileNBS.header, fileNBS.listLayer, tStream) &&
			ReadInstruments(fileNBS.header, fileNBS.listInstrument, tStream);
	}

#undef NORM_READ
#undef COND_READ
#undef FAIL_RETURN
};
