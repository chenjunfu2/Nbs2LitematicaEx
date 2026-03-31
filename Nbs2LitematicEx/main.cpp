#include <nbt_cpp/NBT_All.hpp>
#include <nbs_cpp/NBS_All.hpp>

#include <vector>
#include <stdint.h>

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



int main(int argc, char *argv[])
{











	return 0;
}
