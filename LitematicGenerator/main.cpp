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
	if (argc != 2)
	{
		//PrintHelp(argv[0]);
		print("Usage: {} <NBS file>\n", argv[0]);
		return 0;
	}

	//1必须是路径
	std::filesystem::path pathInputFile{ argv[1] };

	//读取NBS文件
	NBS_File fNbs;
	MyAssert(NBS_IO::ReadNBSFromFile(fNbs, pathInputFile), std::format("NBS File: [{}] Read Fail!\n", pathInputFile.string()).c_str());

	//进行分层预处理
	//获取每一层音符数，按照出现顺序生成音符盒调色板，中继器固定1挡位
	//在最底层生成普通方块垫底，第二层生成音符盒音色方块和普通方块，第三层生成音符盒与中继器

	NBT_Type::Long lNbtTime = (NBT_Type::Long)CodeTimer::GetSystemTime();
	LitematicFile fLitematic
	{
		.stMetaData
		{
			.stEnclosingSize{},//偷懒不写
			.strAuthor{MU8STR("AutoGen")},
			.strDescription{},
			.strName{pathInputFile.filename()},
			.iRegionCount = 0,//生成完成后再修改
			.lTimeCreated = lNbtTime,
			.lTimeModified = lNbtTime,
			.iTotalBlocks = 0,//偷懒不写
			.iTotalVolume = 0,//偷懒不写
		}
	};

	//可复用方块
	//RepeaterBlock stRepeaterBlock{ .enFacing = RepeaterBlock::Facing::west };//朝向为西

	const auto nbsNoteList = MergeAndSortMyNoteList(ToMyNoteList(fNbs, false));
	NBS_File::LONG lMaxTick = nbsNoteList.empty() ? 1 : nbsNoteList.back().tick;
	
	//把音符先根据挤压情况插入层中
	NoteLayerList nbsNoteLayerList{};
	auto listLayerEmplace = [&](const MyNote &note, size_t szLayerIndex) -> void
	{
		if (szLayerIndex >= nbsNoteLayerList.size())
		{
			for (size_t i = nbsNoteLayerList.size(); i <= szLayerIndex; ++i)
			{
				nbsNoteLayerList.emplace_back(MyNoteList{});
			}
		}

		nbsNoteLayerList[szLayerIndex].emplace_back(note);
	};

	size_t szLayerIndex = 0;
	size_t szNoteSize = nbsNoteList.size();
	for (size_t i = 0; i < szNoteSize; ++i)//处理所有音符
	{
		const auto &noteCurr = nbsNoteList[i];

		size_t next_i = i + 1;
		if (next_i >= szNoteSize)//当前处理的是最后一个
		{
			listLayerEmplace(noteCurr, szLayerIndex);
			break;
		}

		const auto &noteNext = nbsNoteList[next_i];

		listLayerEmplace(noteCurr, szLayerIndex);

		
		szLayerIndex = 
			noteCurr.tick == noteNext.tick &&
			noteCurr.key == noteNext.key
			? szLayerIndex + 1//重复音，递增
			: 0;//非重复，归零
	}

	//复用，这里解释方式不同
	szLayerIndex = 0;
	for (const auto &noteLayer : nbsNoteLayerList)//处理每一层
	{
		if (noteLayer.empty())
		{
			++szLayerIndex;//留空方便发现问题
			continue;
		}

		//获取当前层最大tick得到当前层长度
		NBS_File::LONG lCurLayerMaxTick = noteLayer.back().tick + 1;//noteLayer不为空，放心访问


		//首先进行离散化，对每个非空白note分配一个值
		auto nv = ToNoteVal2(noteLayer);
		//计算投影选区调色板（大小为不同的音符数加音色种类）
		LitematicFile::Region reg;
		size_t szBlockStatePaletteSize = nv.mapInstrumentIndex.size() + 1;//+1包含空气

		reg.stSize = { (NBT_Type::Int)lCurLayerMaxTick,25,1 };//长*高*宽
		reg.stPosition = { 0, 0 ,(NBT_Type::Int)szLayerIndex };//偏移

		//初始化方块生成器
		reg.stBlocks.Init(szBlockStatePaletteSize, reg.stSize);

		size_t szBasePaletteStartIndex = reg.stBlocks.listBlockStatePalette.Size();//0

		reg.stBlocks.listBlockStatePalette.AddBack(stAirBlock.ToCompound());//0

		size_t szInstrumentPaletteStartIndex = reg.stBlocks.listBlockStatePalette.Size();//1

		//初始化音符盒音色方块
		for (const auto &it : nv.listInstrumentMap)//递增索引->不同方块
		{
			reg.stBlocks.listBlockStatePalette.AddBack(NoteBlock::GetInstrumentBlock((NoteBlock::Instrument)it));//音色直接映射
		}

		MyAssert(reg.stBlocks.listBlockStatePalette.Size() == szBlockStatePaletteSize, "WTF?");

		//到此已完成调色板准备
		//根据当前层的tick依次生成方块
		for (const auto &note : noteLayer)
		{
			//音域限制
			if (note.key < 33 || note.key > 33 + 24)
			{
				continue;//丢弃超出音域的音符
			}

			MyAssert(note.tick < lCurLayerMaxTick, std::format("tick:key = [{}]:[{}]\n", note.tick, note.key).c_str());

			//获取离散化映射索引
			size_t szNoteInstrumentMapIndex;
			{
				auto itFind = nv.mapInstrumentIndex.find(note.instrument);
				if (itFind == nv.mapInstrumentIndex.end())
				{
					continue;//丢弃无法用到的音色
				}
				szNoteInstrumentMapIndex = itFind->second;
			}

			reg.stBlocks.SetBlock(reg.stBlocks.GetSpatialIndex({ (NBT_Type::Int)note.tick, (NBT_Type::Int)note.key - 33,0 }), szInstrumentPaletteStartIndex + szNoteInstrumentMapIndex);//生成音色方块
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
