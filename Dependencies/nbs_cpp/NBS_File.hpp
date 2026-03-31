#pragma once

#include <stdint.h>
#include <string>
#include <vector>

class NBS_File
{
public:
	using BYTE = uint8_t;
	using SHORT = uint16_t;
	using SSHORT = int16_t;
	using INT = uint32_t;
	using STR = std::string;

	using BOOL = BYTE;
	using STRLEN = INT;

	constexpr const static inline INT CURRENT_NBS_VERSION = 5;

	template<typename T>
	static constexpr bool IsNumeric_Type =
		std::is_same_v<T, BYTE> ||
		std::is_same_v<T, SHORT> ||
		std::is_same_v<T, SSHORT> ||
		std::is_same_v<T, INT> ||
		std::is_same_v<T, BOOL> ||
		std::is_same_v<T, STRLEN>;

	//extern type
	using FLOAT = double;
	using SBYTE = int8_t;
	using LONG = uint64_t;

public:
	//备注，所有STR都以cp1252编码
	struct Header
	{
		//版本信息
		BYTE version;					//NBS文件版本号（0-5，当前最新为5）

		//基础设置
		BYTE default_instruments;		//version > 0：默认乐器数量（通常为16）
		SHORT song_length;				//version >= 3：歌曲总长度（最大tick值）
		SHORT song_layers;				//歌曲总层数

		//歌曲元数据
		STR song_name;			//歌曲名称
		STR song_author;		//歌曲作者
		STR original_author;	//原曲作者（改编时使用）
		STR description;		//歌曲描述/备注

		//播放设置
		SHORT tempo;					//速度值，存储为整数，实际速度 = tempo / 100.0，默认1000对应10.0，读取后float ftempo = tempo/100.0，存储时tempo = (SHORT)(ftempo*100.0)
		BOOL auto_save;					//是否开启自动保存
		BYTE auto_save_duration;		//自动保存间隔（分钟）
		BYTE time_signature;			//拍号分母（如4表示4/4拍）

		//统计信息
		INT minutes_spent;				//编辑总耗时（分钟）
		INT left_clicks;				//左键点击次数
		INT right_clicks;				//右键点击次数
		INT blocks_added;				//添加的方块总数
		INT blocks_removed;				//移除的方块总数

		//来源信息
		STR song_origin;		//歌曲来源标识

		//循环设置
		BOOL loop;						//version >= 4：是否启用循环播放
		BYTE max_loop_count;			//version >= 4：最大循环次数
		SHORT loop_start;				//version >= 4：循环起始位置

	public:
		FLOAT Get_tempo_ActualValue(void) const
		{
			return (FLOAT)tempo / (FLOAT)100.0;
		}

		void Set_tempo_ActualValue(FLOAT dNewActualVal)
		{
			tempo = (SHORT)(dNewActualVal * (FLOAT)100.0);
		}
	};

	struct Note
	{
		//位置信息
		LONG tick;			//音符所在的 tick 绝对位置（写入时使用SHORT仅存储相对偏移）
		SHORT layer;		//所属层索引

		//音符参数
		BYTE instrument;	//乐器索引 (0-255)
		BYTE key;			//音符键位 (0-87, 对应钢琴键)
		BYTE velocity;		//力度 (0-100, 默认100)
		BYTE panning;		//声像 (0-200, 0为最左, 200为最右, 100为中央, 默认100)，使用的时候需-100获得实际值，存储时无符号需要+100
		SSHORT pitch;		//音高微调 (-32768 to 32767, 默认0)

	public:
		SBYTE Get_panning_ActualValue(void) const
		{
			return (SBYTE)((SSHORT)panning - (SSHORT)100);
		}

		void Set_panning_ActualValue(SBYTE sbNewActualVal)
		{
			panning = (BYTE)((SSHORT)sbNewActualVal + (SSHORT)100);
		}
	};

	struct Layer
	{
		SHORT id;			//层索引
		STR name;	//层名称
		BOOL lock;			//是否锁定（静音） (v4+)
		BYTE volume;		//音量 (0-100)
		BYTE panning;		//声像 (-100 到 100, 0为中央, 默认100)，使用的时候需-100获得实际值，存储时无符号需要+100

	public:
		SBYTE Get_panning_ActualValue(void) const
		{
			return (SBYTE)((SSHORT)panning - (SSHORT)100);
		}

		void Set_panning_ActualValue(SBYTE sbNewActualVal)
		{
			panning = (BYTE)((SSHORT)sbNewActualVal + (SSHORT)100);
		}
	};

	struct Instrument
	{
		INT id;				//乐器ID（在乐器列表中的索引，从0开始）
		STR name;	//乐器名称
		STR file;	//自定义音效文件路径（空字符串表示使用默认音色）
		BYTE pitch;			//音高偏移（默认45，对应C#，范围0-255）
		BOOL press_key;		//是否按下按键（默认true）
	};

	using ListNote = std::vector<Note>;
	using ListLayer = std::vector<Layer>;
	using ListInstrument = std::vector<Instrument>;

public:
	Header header;
	ListNote listNote;
	ListLayer listLayer;
	ListInstrument listInstrument;
};
