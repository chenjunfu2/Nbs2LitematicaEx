#include <nbt_cpp/NBT_All.hpp>
#include <nbs_cpp/NBS_All.hpp>

#include <util/MyAssert.hpp>

#include <stdint.h>
#include <stdexcept>
#include <algorithm>
#include <ranges>

#include <vector>
#include <unordered_map>

template<typename... Args>
void print(std::format_string<Args...> fmt, Args&&... args)
{
	std::string output = std::format(fmt, std::forward<Args>(args)...);
	fwrite(output.data(), 1, output.size(), stdout);
}

template<typename... Args>
void printerr(std::format_string<Args...> fmt, Args&&... args)
{
	std::string output = std::format(fmt, std::forward<Args>(args)...);
	fwrite(output.data(), 1, output.size(), stderr);
}

struct MyNote
{
	NBS_File::LONG tick;
	NBS_File::BYTE instrument;
	NBS_File::BYTE key;
};

using MyNoteList = std::vector<MyNote>;

MyNoteList ToMyNoteList(const NBS_File &fNBS)
{
	std::vector<uint8_t> blistLocked;
	blistLocked.reserve(fNBS.listLayer.size());
	for (auto &it : fNBS.listLayer)
	{
		blistLocked.push_back(it.lock);
	}

	MyNoteList listMyNote;
	listMyNote.reserve(fNBS.listNote.size());
	for (auto &it : fNBS.listNote)
	{
		if (it.layer < blistLocked.size() && blistLocked[it.layer] == 0)//没有溢出范围且未上锁（静音）
		{
			listMyNote.emplace_back(it.tick, it.instrument, it.key);
		}
	}

	return listMyNote;
}

using InstGroupNote = std::vector<MyNoteList>;

InstGroupNote ToInstMap(MyNoteList &listNote)
{
	if (listNote.empty())
	{
		return {};
	}

	std::ranges::sort(listNote,
		[](const MyNote &l, const MyNote &r) -> bool //升序排列，先按照组，然后按照tick，最后按照key
		{
			if (l.instrument != r.instrument)
			{
				return l.instrument < r.instrument;
			}
			else if (l.tick != r.tick)
			{
				return l.tick < r.tick;
			}
			else
			{
				return l.key < r.key;
			}
		}
	);

	InstGroupNote groupNote;
	NBS_File::BYTE bLastInstrument = listNote.front().instrument;
	MyNoteList tempList;
	for (auto &it : listNote)
	{
		if (bLastInstrument != it.instrument)
		{
			bLastInstrument = it.instrument;
			groupNote.push_back(std::move(tempList));
			tempList.clear();
		}
		//不论如何都插入，如果遇到不等的情况，上面会进行清理
		tempList.push_back(std::move(it));
	}

	//如果最后一个不为空，那么插入最终的值
	if (!tempList.empty())
	{
		groupNote.push_back(std::move(tempList));
	}

	return groupNote;
}

int main(int argc, char *argv[]) try
{
	MyAssert(argc == 2);
	NBS_File nbs;
	MyAssert(NBS_IO::ReadNBSFromFile(nbs, argv[1]));

	//提取关键信息
	auto noteList = ToMyNoteList(nbs);
	nbs.~NBS_File();

	//根据音符类型拆散
	auto groupNote = ToInstMap(noteList);
	noteList.~vector();

	//现在，map的每个音，后面跟着若干音符
	//输出一下看看
	for (auto &it : groupNote)
	{
		print("========================================\nInst: [{}], Count: [{}]\n", it.front().instrument, it.size());
		for (auto &note : it)
		{
			print("\tTick: [{}], Key: [{}]\n", note.tick, note.key);
		}
	}
	print("========================================\n");



	//后缀数组sa+LCP查找所有重复子串，使用贪心匹配最大不重叠子串集合
	//根据集合完成重复序列收集
	//对乐器分组做一次，对完整组做一次，然后分别输出







	return 0;
}
catch (const std::exception &e)
{
	printerr("catch std::exception: [{}]", e.what());
	throw e;
}
