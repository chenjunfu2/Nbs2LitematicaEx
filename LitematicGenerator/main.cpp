#include "LitematicGenerator.hpp"

#include <nbs_cpp/NBS_All.hpp>
#include <util/CodeTimer.hpp>
#include <util/MyAssert.hpp>
#include <format>

//int main(void)
//{
//	auto ab = AirBlock{}.ToCompound();
//	auto nb = NoteBlock{}.ToCompound();
//	auto rb = RepeaterBlock{}.ToCompound();
//
//	NBT_Print{}("AirBlock:\n");
//	NBT_Helper::Print(ab);
//	NBT_Print{}("\n\nNoteBlock:\n");
//	NBT_Helper::Print(nb);
//	NBT_Print{}("\n\nRepeaterBlock:\n");
//	NBT_Helper::Print(rb);
//	NBT_Print{}("\n");
//
//	return 0;
//}

template<typename... Args>
void print(std::format_string<Args...> fmt, Args&&... args)
{
	std::string output = std::format(fmt, std::forward<Args>(args)...);
	fwrite(output.data(), 1, output.size(), stdout);
}


int main(int argc, char *argv[]) try
{
	MyAssert(argc == 2 && argv[1] != NULL, std::format("Use:\n{} [NBS File Name]\n", argv[0]).c_str());

	NBS_File fNbs;
	MyAssert(NBS_IO::ReadNBSFromFile(fNbs, argv[1]), std::format("NBS File: [{}] Read Fail!\n").c_str());

	//进行分层预处理
	//获取每一层音符数，按照出现顺序生成音符盒调色板，中继器固定1挡位
	//在最底层生成普通方块垫底，第二层生成音符盒音色方块和普通方块，第三层生成音符盒与中继器










	return 0;
}
catch (const std::exception &e)
{
	print("ERROR!\ncatch std::exception: [{}]\n", e.what());
	throw e;
}
