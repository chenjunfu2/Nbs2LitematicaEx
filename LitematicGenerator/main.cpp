#include "LitematicFile.hpp"
#include "MyNote.hpp"

#include <nbs_cpp/NBS_All.hpp>
#include <util/CodeTimer.hpp>
#include <util/MyAssert.hpp>
#include <format>
#include <filesystem>

template<typename... Args>
void print(std::format_string<Args...> fmt, Args&&... args)
{
	std::string output = std::format(fmt, std::forward<Args>(args)...);
	fwrite(output.data(), 1, output.size(), stdout);
}

//找到一个唯一文件名
std::string GenerateUniqueFilename(const std::string &sBeg, const std::string &sEnd, uint32_t u32TryCount = 10)//默认最多重试10次
{
	while (u32TryCount != 0)
	{
		//时间用[]包围
		auto tmpPath = std::format("{}[{}]{}", sBeg, CodeTimer::GetSystemTime(), sEnd);//获取当前系统时间戳作为中间的部分
		if (!NBT_IO::IsFileExist(tmpPath))
		{
			return tmpPath;
		}

		//等几ms在继续
		CodeTimer::Sleep(std::chrono::milliseconds(10));
		--u32TryCount;
	}

	//次数到上限直接返回空
	return std::string{};
}

void PrintHelp(const char *pExeName)
{
	print("\nUsage: {} <NBS file> [option]\n"
		"Option (must be a single argument, starts with '-' or '/'):\n"
		"  -e, /e    No Exclude Space\n"
		"  -r, /r    No Repeater\n"
		"  -er, /er  No Exclude Space and No Repeater\n"
		"By default (no option), ExcludeSpace and Repeater are enabled.\n"
		"Examples:\n"
		"  {} song.nbs -e\n"
		"  {} song.nbs /er\n",
		pExeName, pExeName, pExeName);
}

int main(int argc, char *argv[]) try
{
	if (argc < 2 || argc > 3)//至少2个命令，最多3个
	{
		if (argc != 1)
		{
			print("Error: Invalid argument count (expected 1 or 2, got {}).\n", argc - 1);
		}
		PrintHelp(argv[0]);
		return 0;
	}

	//1必须是路径
	std::filesystem::path pathInputFile{ argv[1] };

	//命令行解析2
	bool bNoExcludeSpace = false;//-e
	bool bNoRepeater = false;//-r

	if (argc > 2)
	{
		std::string_view svArg = { argv[2] };
		if (svArg.size() < 2 || svArg.size() > 3)//至少要-开头或/开头并跟随一个选项，并且不能选项过多
		{
			print("Error: Option '{}' has invalid length (must be 2 or 3 characters, e.g. -e, -r, -er).\n", svArg);
			PrintHelp(argv[0]);
			return 0;
		}

		//参数验证失败
		if (svArg[0] != '-' && svArg[0] != '/')
		{
			print("Error: Option '{}' must start with '-' or '/'.\n", svArg);
			PrintHelp(argv[0]);
			return 0;
		}

		for (size_t i = 1; i < svArg.size(); ++i)
		{
			switch (svArg[i])
			{
			case 'e':
				if (bNoExcludeSpace)
				{
					print("Error: Duplicate option 'e'.\n");
					PrintHelp(argv[0]);
					return 0;
				}
				bNoExcludeSpace = true;
				break;
			case 'r':
				if (bNoRepeater)
				{
					print("Error: Duplicate option 'r'.\n");
					PrintHelp(argv[0]);
					return 0;
				}
				bNoRepeater = true;
				break;
			default:
				print("Error: Unknown option character '{}' in '{}' (allowed: e, r).\n", svArg[i], svArg);
				PrintHelp(argv[0]);
				return 0;
				break;
			}
		}
	}

	//读取NBS文件
	NBS_File fNbs;
	MyAssert(NBS_IO::ReadNBSFromFile(fNbs, pathInputFile), std::format("NBS File: [{}] Read Fail!\n", pathInputFile.string()).c_str());

	//进行分层预处理
	//获取每一层音符数，按照出现顺序生成音符盒调色板，中继器固定1挡位
	//在最底层生成普通方块垫底，第二层生成音符盒音色方块和普通方块，第三层生成音符盒与中继器

	LitematicFile fLitematic
	{
		.stMetaData
		{
			.stEnclosingSize{},//偷懒不写
			.strAuthor{MU8STR("AutoGen")},
			.strDescription{},
			.strName{pathInputFile.filename()},
			.iRegionCount = 0,//生成完成后再修改
			.lTimeCreated = (NBT_Type::Long)CodeTimer::GetSystemTime(),
			.lTimeModified = (NBT_Type::Long)CodeTimer::GetSystemTime(),
			.iTotalBlocks = 0,//偷懒不写
			.iTotalVolume = 0,//偷懒不写
		}
	};

	//可复用方块
	RepeaterBlock stRepeaterBlock{ .enFacing = RepeaterBlock::Facing::west };//朝向为西

	auto nbsNoteLayerList = ToMyNoteList2(ToMyNoteList(fNbs, bNoExcludeSpace));
	size_t szLayerIndex = 0;
	for (const auto &noteLayer : nbsNoteLayerList)//处理每一层
	{
		if (noteLayer.empty())//空层直接移动
		{
			++szLayerIndex;
			continue;
		}

		//遍历当前层，计算最终长度
		bool bFirstBlank = noteLayer.front().enType == MyNote2::Type::Blank;//空白开头添加额外虚拟空音符
		size_t szLineLong = 0;
		szLineLong += (size_t)bFirstBlank;//虚拟音
		for (const auto &note : noteLayer)
		{
			if (note.enType == MyNote2::Type::Note)
			{
				++szLineLong;
			}
			else if (note.enType == MyNote2::Type::Blank)
			{
				szLineLong += (size_t)note.tick * 2 - 1;//nbs中的tick是redstone tick，根据运算得到矩阵宽度
			}
		}

		//首先进行离散化，对每个非空白note分配一个值
		auto nv = ToNoteVal(noteLayer);
		//计算投影选区调色板（大小为不同的音符数加音色种类加3（3的组成有空气方块、实体方块、1挡中继器方块））
		LitematicFile::Region reg;
		size_t szBlockStatePaletteSize = nv.mapInstrumentIndex.size() + nv.mapNote2Index.size() + 3;

		reg.stSize = { (NBT_Type::Int)szLineLong,3,1 };//长*高*宽
		reg.stPosition = { 0,0,(NBT_Type::Int)szLayerIndex * 2 };//偏移
		reg.stBlocks.Init(szBlockStatePaletteSize, reg.stSize);

		size_t szBasePaletteStartIndex = reg.stBlocks.listBlockStatePalette.Size();//0

		reg.stBlocks.listBlockStatePalette.AddBack(stAirBlock.ToCompound());//0
		reg.stBlocks.listBlockStatePalette.AddBack(stSmoothStoneBlock.ToCompound());//1
		reg.stBlocks.listBlockStatePalette.AddBack(stRepeaterBlock.ToCompound());//2

		size_t szInstrumentPaletteStartIndex = reg.stBlocks.listBlockStatePalette.Size();//3

		//初始化音符盒音色方块
		for (const auto &it: nv.listInstrumentMap)//递增索引->不同方块
		{
			reg.stBlocks.listBlockStatePalette.AddBack(NoteBlock::GetInstrumentBlock((NoteBlock::Instrument)it));//音色直接映射
		}

		size_t szNoteBlockPaletteStartIndex = reg.stBlocks.listBlockStatePalette.Size();

		//初始化音符盒方块
		for (const auto &it : nv.listNote2Map)
		{
			reg.stBlocks.listBlockStatePalette.AddBack(NoteBlock{ .enInstrument = (NoteBlock::Instrument)it.instrument,.u8Note = (uint8_t)(it.key - 33) }.ToCompound());//key-33映射
		}

		MyAssert(reg.stBlocks.listBlockStatePalette.Size() == szBlockStatePaletteSize, "WTF?");

		//到此已完成调色板准备

		//设置第二层与第三层（同时）
		size_t x = 0;

		if (bFirstBlank)//虚拟音
		{
			if (!bNoRepeater)
			{
				reg.stBlocks.SetBlock(reg.stBlocks.GetSpatialIndex({ (NBT_Type::Int)x,2,0 }), 1);//3层 -> 平滑石
			}
			++x;
		}
		
		for (const auto &note : noteLayer)
		{
			//根据乐谱信息设置
			
			//如果当前是空白，那么生成等量的方块与中继器（第2~3层）
			if (note.enType == MyNote2::Type::Blank)
			{
				if (!bNoRepeater)
				{
					bool bGenBlock = false;
					for (size_t i = 0; i < (size_t)note.tick * 2 - 1; ++i)
					{
						if (bGenBlock)
						{
							reg.stBlocks.SetBlock(reg.stBlocks.GetSpatialIndex({ (NBT_Type::Int)x,2,0 }), 1);//3层 -> 平滑石
						}
						else
						{

							reg.stBlocks.SetBlock(reg.stBlocks.GetSpatialIndex({ (NBT_Type::Int)x,1,0 }), 1);//2层 -> 平滑石
							reg.stBlocks.SetBlock(reg.stBlocks.GetSpatialIndex({ (NBT_Type::Int)x,2,0 }), 2);//3层 -> 中继器
						}
						bGenBlock = !bGenBlock;

						++x;
					}
				}
				else
				{
					x += (size_t)note.tick * 2 - 1;
				}
			}
			else if (note.enType == MyNote2::Type::Note)//当前是音符（空白不被索引），查找索引，然后生成
			{//设置第1~3层
				//获取离散化映射索引
				size_t szNoteMapIndex;
				{
					auto itFind = nv.mapNote2Index.find(note);
					//因为映射过程去除了无法生成的超音域音，所以需要在音无法找到的时候，生成为方块以传递信号
					if (itFind == nv.mapNote2Index.end())//MyAssert(itFind != nv.mapNote2Index.end(), "WTF?");
					{
						reg.stBlocks.SetBlock(reg.stBlocks.GetSpatialIndex({ (NBT_Type::Int)x,2,0 }), 1);//3层 -> 平滑石
						++x;
						continue;//跳过剩余生成
					}
					szNoteMapIndex = itFind->second;
				}

				size_t szNoteInstrumentMapIndex;
				{
					auto itFind = nv.mapInstrumentIndex.find(note.instrument);
					MyAssert(itFind != nv.mapInstrumentIndex.end(), "WTF?");//音色目前依旧是必须找到的，因为对应音色的音符盒至少存在一个，否则上面就会返回
					szNoteInstrumentMapIndex = itFind->second;
				}

				if ((uint8_t)note.instrument == (uint8_t)NoteBlock::Instrument::snare)//重力方块
				{
					reg.stBlocks.SetBlock(reg.stBlocks.GetSpatialIndex({ (NBT_Type::Int)x,0,0 }), 1);//生成1层音色方块垫底->stSmoothStoneBlock
				}
				reg.stBlocks.SetBlock(reg.stBlocks.GetSpatialIndex({ (NBT_Type::Int)x,1,0 }), szInstrumentPaletteStartIndex + szNoteInstrumentMapIndex);//2层，生成音符盒垫底方块
				reg.stBlocks.SetBlock(reg.stBlocks.GetSpatialIndex({ (NBT_Type::Int)x,2,0 }), szNoteBlockPaletteStartIndex + szNoteMapIndex);//3层，生成音符盒方块
				++x;
			}
		}

		//完成，插入区域中
		MyAssert(fLitematic.stRegions.mapRegion.emplace(std::format("layer-{}", szLayerIndex++), std::move(reg)).second, "WTF?");
	}

	//填入元信息
	fLitematic.stMetaData.iRegionCount = fLitematic.stRegions.mapRegion.size();

	//准备NBT生成
	std::vector<uint8_t> vStream, vStreamComp;
	MyAssert(NBT_Writer::WriteNBT(vStream, 0, NBT_Type::Compound{ {MU8STR(""), std::move(fLitematic).ToCompound() } }));
	MyAssert(NBT_IO::CompressDataNoThrow(vStreamComp, vStream));
	vStream.clear();
	vStream.shrink_to_fit();

#ifndef _DEBUG
	std::string sFilePath = pathInputFile.string();
	{
		//找到后缀名
		size_t szPos = sFilePath.find_last_of('.');

		//'.'前面的部分，不包含'.'
		std::string sNewFileName = sFilePath.substr(0, szPos).append("_");
		//'.'后面的部分，包含'.'
		std::string sNewFileExten = ".litematic";

		//唯一文件名
		sFilePath = GenerateUniqueFilename(sNewFileName, sNewFileExten);
		if (sFilePath.empty())
		{
			print("Unable to find a valid file name or lack of permission!\n");
			return false;
		}
	}
#else
	std::string sFilePath{ "test.litematic" };
#endif
	MyAssert(NBT_IO::WriteFile(sFilePath, vStreamComp));

	return 0;
}
catch (const std::exception &e)
{
	print("ERROR!\ncatch std::exception: [{}]\n", e.what());
	throw e;
}
