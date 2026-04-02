#include <nbt_cpp/NBT_All.hpp>
#include <nbs_cpp/NBS_All.hpp>

#include <util/MyAssert.hpp>

#include <stdint.h>
#include <stdexcept>
#include <algorithm>
#include <ranges>
#include <compare>

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

struct MyNoteSub
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
	MyNoteSub(NBS_File::BYTE _instrument, NBS_File::BYTE _key) : enType(Type::Note), instrument(_instrument), key(_key)
	{}

	MyNoteSub(NBS_File::SHORT _tick) : enType(Type::Blank), tick(_tick)
	{}
	
	~MyNoteSub(void) = default;
public:
	bool operator==(const MyNoteSub &_Right) const
	{
		switch (enType)
		{
		case MyNoteSub::Type::Note:
			return instrument == _Right.instrument && key == _Right.key;
			break;
		case MyNoteSub::Type::Blank:
			return tick == _Right.tick;
			break;
		default:
			throw std::runtime_error("Switched Unknown Type!");
			break;
		}
	}

	bool operator!=(const MyNoteSub &_Right) const
	{
		switch (enType)
		{
		case MyNoteSub::Type::Note:
			return instrument != _Right.instrument || key != _Right.key;
			break;
		case MyNoteSub::Type::Blank:
			return tick != _Right.tick;
			break;
		default:
			throw std::runtime_error("Switched Unknown Type!");
			break;
		}
	}

	std::strong_ordering operator<=>(const MyNoteSub &_Right) const
	{
		switch (enType)
		{
		case MyNoteSub::Type::Note:
			if (auto tmp = instrument <=> _Right.instrument; tmp != 0)
			{
				return tmp;
			}
			else
			{
				return key <=> _Right.key;
			}
			break;
		case MyNoteSub::Type::Blank:
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
		case MyNoteSub::Type::Note:
			{
				return std::hash<uint16_t>{}((uint16_t)instrument << 8 | (uint16_t)key << 0);
			}
			break;
		case MyNoteSub::Type::Blank:
			{
				return std::hash<uint16_t>{}(tick);
			}
			break;
		default:
			throw std::runtime_error("Switched Unknown Type!");
			break;
		}
	}

	void Print(std::string_view beg = "", std::string_view end = "\n") const
	{
		switch (enType)
		{
		case MyNoteSub::Type::Note:
			print("{}instrument: [{:02}], key: [{:02}]{}", beg, instrument, key, end);
			break;
		case MyNoteSub::Type::Blank:
			print("{}Δtick: [{:02}]{}", beg, tick, end);
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
	struct hash<MyNoteSub>
	{
		size_t operator()(const MyNoteSub &_Hash) const noexcept
		{
			return _Hash.Hash();
		}
	};
}

struct MyNote
{
public:
	NBS_File::LONG tick;
	NBS_File::BYTE instrument;
	NBS_File::BYTE key;

public:
	void Print(std::string_view beg = "", std::string_view end = "\n") const
	{
		print("{}tick: [{:02}], instrument: [{:02}], key: [{:02}]{}", beg, tick, instrument, key, end);
	}
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

using MyNoteSubList = std::vector<MyNoteSub>;

//转化为音符-空白的序列方便子串匹配
MyNoteSubList ToMyNoteSubList(const MyNoteList &listNote)
{
	MyNoteList sortedListNote = listNote;

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

	//列表
	MyNoteSubList listNoteSub;
	listNoteSub.reserve(szSortedNoteSize);
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
			listNoteSub.emplace_back(deleta_tick);//tick是绝对值的差值
		}

		//插入音符
		listNoteSub.emplace_back(curNote.instrument, curNote.key);
	}

	return listNoteSub;
}

struct NoteVal
{
	std::vector<MyNoteSub> listNoteSubMap;
	std::vector<size_t> listEncodeNoteSub;
};

NoteVal ToNoteVal(const MyNoteSubList &listNoteSub)
{
	NoteVal valNote;
	std::unordered_map<MyNoteSub, size_t> mapNoteSubIndex;
	for (const auto &it : listNoteSub)
	{
		auto [itEmplace, bSuccess] = mapNoteSubIndex.try_emplace(it, valNote.listNoteSubMap.size());//如果插入成功，那么size相当于index（因为下面插入）
		if (bSuccess)
		{
			valNote.listNoteSubMap.push_back(it);
		}

		//如果插入失败，那么itEmplace获得的是阻止插入的元素，相当于查找到它的映射，否则是刚才插入的元素
		valNote.listEncodeNoteSub.push_back(itEmplace->second);//插入编码数组中，这样编码数组利用size值替代了listNoteSub
	}

	//map用完即弃
	return valNote;
}



//using InstGroupNote = std::vector<MyNoteList>;
//
//InstGroupNote ToInstMap(MyNoteList &listNote)
//{
//	if (listNote.empty())
//	{
//		return {};
//	}
//
//	std::ranges::sort(listNote,
//		[](const MyNote &l, const MyNote &r) -> bool //升序排列，先按照组，然后按照tick，最后按照key
//		{
//			if (l.instrument != r.instrument)
//			{
//				return l.instrument < r.instrument;
//			}
//			else if (l.tick != r.tick)
//			{
//				return l.tick < r.tick;
//			}
//			else
//			{
//				return l.key < r.key;
//			}
//		}
//	);
//
//	InstGroupNote groupNote;
//	NBS_File::BYTE bLastInstrument = listNote.front().instrument;
//	MyNoteList tempList;
//	for (auto &it : listNote)
//	{
//		if (bLastInstrument != it.instrument)
//		{
//			bLastInstrument = it.instrument;
//			groupNote.push_back(std::move(tempList));
//			tempList.clear();
//		}
//		//不论如何都插入，如果遇到不等的情况，上面会进行清理
//		tempList.push_back(std::move(it));
//	}
//
//	//如果最后一个不为空，那么插入最终的值
//	if (!tempList.empty())
//	{
//		groupNote.push_back(std::move(tempList));
//	}
//
//	return groupNote;
//}
	////根据音符类型拆散
	//auto groupNote = ToInstMap(noteList);
	//noteList.~vector();
	//
	////现在，map的每个音，后面跟着若干音符
	////输出一下看看
	//for (auto &it : groupNote)
	//{
	//	print("========================================\nInst: [{}], Count: [{}]\n", it.front().instrument, it.size());
	//	for (auto &note : it)
	//	{
	//		print("\tTick: [{}], Key: [{}]\n", note.tick, note.key);
	//	}
	//}
	//print("========================================\n");

struct SAHI
{
public:
	using SuffixArray = std::vector<size_t>;
	using HeightArray = std::vector<size_t>;

public:
	SuffixArray SuffArr;
	HeightArray HighArr;
};


SAHI DoublingCountingRadixSortSuffixArray(const NoteVal &valNote)
{
	size_t szCountMaxVal = valNote.listNoteSubMap.size() + 1;
	size_t szArrayLength = valNote.listEncodeNoteSub.size();

	size_t szArraySize = sizeof(size_t) * (szArrayLength + 1);//从1~length，所以多分配一个
	uint8_t *pBase = new uint8_t[szArraySize * (5 + 2)];

	size_t szBegIndex = 0;
	size_t *pCount =		(size_t *)&pBase[szArraySize * szBegIndex]; szBegIndex += 1;
	size_t *pRank =			(size_t *)&pBase[szArraySize * szBegIndex]; szBegIndex += 2;
	size_t *pLastRank =		(size_t *)&pBase[szArraySize * szBegIndex]; szBegIndex += 1;
	size_t *pSufArr =		(size_t *)&pBase[szArraySize * szBegIndex]; szBegIndex += 2;
	size_t *pLastSufArr =	(size_t *)&pBase[szArraySize * szBegIndex]; szBegIndex += 1;

	memset(pCount, 0, szArraySize);

	memset(pRank, 0, szArraySize * 2);//后半额外填充
	memset(pLastRank, 0, szArraySize);
	memset(pSufArr, 0, szArraySize * 2);//后半额外填充
	memset(pLastSufArr, 0, szArraySize);

	//第一关键字排序
	//计算出现次数
	for (size_t i = 1; i <= szArrayLength; ++i)
	{
		++pCount[pRank[i] = valNote.listEncodeNoteSub[i - 1] + 1];
	}
	//前缀和获取值最后下标
	for (size_t i = 1; i <= szCountMaxVal; ++i)
	{
		pCount[i] += pCount[i - 1];
	}
	//1格排序
	for (size_t i = szArrayLength; i >= 1; --i)
	{
		pSufArr[pCount[pRank[i]]--] = i;
	}

	size_t szDoublingStep = 1, szCurRank = 0;
	while (true)
	{
		//第二关键字排序
		size_t cur = 0;

		for (size_t i = szArrayLength - szDoublingStep + 1; i <= szArrayLength; i++)
		{
			pLastSufArr[++cur] = i;
		}

		for (size_t i = 1; i <= szArrayLength; i++)
		{
			if (pSufArr[i] > szDoublingStep)
			{
				pLastSufArr[++cur] = pSufArr[i] - szDoublingStep;
			}
		}

		//第一排序
		memset(pCount, 0, (szCountMaxVal + 1) * sizeof(size_t));//仅填充需要的部分
		//计算出现次数
		for (size_t i = 1; i <= szArrayLength; ++i)
		{
			++pCount[pRank[pLastSufArr[i]]];
		}
		//前缀和获取值最后下标
		for (size_t i = 1; i <= szCountMaxVal; ++i)
		{
			pCount[i] += pCount[i - 1];
		}
		//排序
		for (size_t i = szArrayLength; i >= 1; --i)
		{
			pSufArr[pCount[pRank[pLastSufArr[i]]]--] = pLastSufArr[i];
		}

		szCurRank = 0;
		memcpy(pLastRank, pRank, szArraySize);//pRank是pLastRank的2倍大小，但是仅前半部分有用
		for (size_t i = 1; i <= szArrayLength; ++i)
		{
			if (pLastRank[pSufArr[i]] == pLastRank[pSufArr[i - 1]] &&
				pLastRank[pSufArr[i] + szDoublingStep] == pLastRank[pSufArr[i - 1] + szDoublingStep])
			{
				pRank[pSufArr[i]] = szCurRank;
			}
			else
			{
				pRank[pSufArr[i]] = ++szCurRank;
			}
		}
		if (szCurRank == szArrayLength)
		{
			break;
		}

		//迭代
		szCountMaxVal = szCurRank;//szCurRank相当于MaxRank
		szDoublingStep *= 2;//倍增
	}

	//准备返回
	SAHI ret;

	ret.SuffArr.reserve(szArrayLength);
	for (size_t i = 1; i <= szArrayLength; ++i)
	{
		ret.SuffArr.push_back(pSufArr[i] - 1);
	}

	ret.HighArr.resize(szArrayLength);
	for (size_t i = 1, k = 0; i <= szArrayLength; ++i)
	{
		if (pRank[i] == 0)
		{
			continue;
		}

		if (k != 0)
		{
			--k;
		}


		auto j = pSufArr[pRank[i] - 1];
		while (j + k != 0 && i + k != 0 &&
			i - 1 + k < valNote.listEncodeNoteSub.size() && j - 1 + k < valNote.listEncodeNoteSub.size() &&
			valNote.listEncodeNoteSub[i - 1 + k] == valNote.listEncodeNoteSub[j - 1 + k])
		{
			++k;
		}

		ret.HighArr[pRank[i] - 1] = k;
	}

	//释放
	delete[] pBase;
	pBase = nullptr;

	return ret;
}

struct RepeatSubNote
{
	size_t start;      // 在原串中的起始位置（0-based）
	size_t length;     // 子串长度
	std::vector<size_t> occurrences;  // 所有出现位置（0-based）
};

using RepeatSubNoteList = std::vector<RepeatSubNote>;

class RepeatSubNoteFinder
{
public:
	static RepeatSubNoteList FindAll(const NoteVal &valNote, const SAHI &sahi)
	{
		size_t n = valNote.listEncodeNoteSub.size();
		const auto &encode = valNote.listEncodeNoteSub;  // 编码数组，作为"字符串"使用

		std::vector<RepeatSubNote> result;
		std::vector<bool> covered(n, false);

		// 使用并查集优化覆盖检查
		std::vector<size_t> next_free(n + 1);
		for (size_t i = 0; i <= n; ++i) next_free[i] = i;

		// 按长度从大到小处理
		for (int len = n / 2; len >= 1; --len)
		{
			size_t i = 1;

			while (i < n)
			{
				if (sahi.HighArr[i] < len)
				{
					++i;
					continue;
				}

				// 收集连续区间（这些后缀至少有 len 的公共前缀）
				std::vector<size_t> positions;
				positions.push_back(sahi.SuffArr[i - 1]);

				while (i < n && sahi.HighArr[i] >= len)
				{
					positions.push_back(sahi.SuffArr[i]);
					++i;
				}

				// 排序并去重
				std::sort(positions.begin(), positions.end());
				positions.erase(std::unique(positions.begin(), positions.end()),
					positions.end());

				// 贪心选择不重叠的位置
				std::vector<size_t> selected;
				size_t last_end = 0;

				for (size_t pos : positions)
				{
					// 找到下一个未被覆盖的位置
					size_t actual_pos = findNextFree(pos, next_free, n);
					if (actual_pos >= n) continue;

					// 检查是否能放下整个子串
					if (actual_pos + len <= n)
					{
						// 检查这个区间是否完全未被覆盖
						bool valid = true;
						for (size_t j = actual_pos; j < actual_pos + len; ++j)
						{
							if (covered[j])
							{
								valid = false;
								break;
							}
						}

						if (valid && (selected.empty() || actual_pos >= last_end))
						{
							selected.push_back(actual_pos);
							last_end = actual_pos + len;
						}
					}
				}

				// 记录结果
				if (selected.size() >= 2)
				{
					RepeatSubNote sub;
					sub.start = selected[0];
					sub.length = len;
					sub.occurrences = selected;
					result.push_back(sub);

					// 标记覆盖
					for (size_t pos : selected)
					{
						for (size_t j = pos; j < pos + len; ++j)
						{
							covered[j] = true;
							next_free[j] = j + len;
						}
					}
					rebuildNextFree(next_free, n);
				}
			}
		}

		return result;
	}

private:
	static size_t findNextFree(size_t pos, const std::vector<size_t> &next_free, size_t n)
	{
		if (pos >= n) return n;
		if (next_free[pos] == pos) return pos;
		return findNextFree(next_free[pos], next_free, n);
	}

	static void rebuildNextFree(std::vector<size_t> &next_free, size_t n)
	{
		for (size_t i = n; i > 0; --i)
		{
			if (next_free[i - 1] != i - 1)
			{
				next_free[i - 1] = findNextFree(next_free[i - 1], next_free, n);
			}
		}
	}
};


int main(int argc, char *argv[]) try
{
	MyAssert(argc == 2);
	NBS_File nbs;
	MyAssert(NBS_IO::ReadNBSFromFile(nbs, argv[1]));

	//提取关键信息
	auto noteList = ToMyNoteList(nbs);
	nbs.~NBS_File();

	for (auto &it : noteList)
	{
		it.Print();
	}

	print("\n==========================================\n\n");

	//拆分为排序序列，同tick音符合并，跨tick音符转为静音tick
	auto listNoteSub = ToMyNoteSubList(noteList);
	for (auto &it : listNoteSub)
	{
		it.Print();
	}

	print("\n==========================================\n\n");

	//后缀数组sa+LCP查找所有重复子串，使用贪心匹配最大不重叠子串集合，相似性匹配递归找变化子序列
	//根据集合完成重复序列收集
	//目前考虑算法先对完整组做一次，再对每个乐器分组做一次，最后分别输出

	//首先，对每个音符、空白音符做唯一值ID映射，映射为一段连续的值域
	//通过unordered map进行查重和下标映射，vector对应下标存储具体音符值
	NoteVal valNote = ToNoteVal(listNoteSub);

	//后缀数组：https://oi-wiki.org/string/sa/
	//SA-IS：https://www.luogu.com.cn/article/eugf8e8y

	//先实现基础版本，后续再考虑是否优化SA-IS

	//在这里，NoteVal存储离散化的双向映射
	//从0~valNote.listNoteSubMap.size()即为值域大小
	//根据值域选择使用的算法（较小使用基数计数排序，较大使用普通排序）

	auto sahi = DoublingCountingRadixSortSuffixArray(valNote);

	auto listRepeatSubNote = RepeatSubNoteFinder::FindAll(valNote, sahi);

	// 输出结果（解码后的实际内容）
	for (const auto &sub : listRepeatSubNote)
	{
		print("sub length: {}\n", sub.length);
		print("pos: ");
		for (size_t pos : sub.occurrences)
		{
			print("{} ", pos);
		}
		print("\n");

		// 输出实际内容（解码）
		print("notes: ");
		for (size_t j = sub.start; j < sub.start + sub.length; ++j)
		{
			size_t encodeVal = valNote.listEncodeNoteSub[j];
			const auto &note = valNote.listNoteSubMap[encodeVal];
			note.Print();
		}
		print("\n\n");
	}

	return 0;
}
catch (const std::exception &e)
{
	printerr("catch std::exception: [{}]", e.what());
	throw e;
}
