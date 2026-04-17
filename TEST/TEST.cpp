/*
#include "../Nbs2LitematicEx/MyAlgorithm.hpp"
#include <stdio.h>
#include <iostream>
#include <string>
#include <algorithm>
#include <iomanip>

void printSAM(const SuffixAutomaton<unsigned char> &sam)
{
	const auto &states = sam.GetListState();

	std::cout << "========== Suffix Automaton Structure ==========\n";
	std::cout << std::left;
	std::cout << std::setw(10) << "State"
		<< std::setw(10) << "len"
		<< std::setw(10) << "link"
		<< "transitions\n";
	std::cout << std::string(60, '-') << "\n";

	for (size_t i = 0; i < states.size(); i++)
	{
		std::cout << std::setw(10) << i
			<< std::setw(10) << states[i].szEndposMaxStrLength
			<< std::setw(10) << states[i].szSuffixLinkTreeIndex;

		// 输出转移边
		std::cout << "{";
		bool first = true;
		for (const auto &[ch, next] : states[i].mapTransitionNextIndex)
		{
			if (!first) std::cout << ", ";
			std::cout << char(ch + 'a') << "->" << next;
			first = false;
		}
		std::cout << "}\n";
	}

	// 输出后缀链接树
	std::cout << "\n========== Suffix Link Tree ==========\n";
	for (size_t i = 0; i < states.size(); i++)
	{
		size_t link = states[i].szSuffixLinkTreeIndex;
		if (link != -1)
		{
			std::cout << i << " -> " << link << "\n";
		}
		else
		{
			std::cout << i << " -> (root)\n";
		}
	}

	// 输出每个状态代表的子串
	std::cout << "\n========== Substrings per State ==========\n";
	for (size_t i = 0; i < states.size(); i++)
	{
		std::cout << "State " << i << " (len=" << states[i].szEndposMaxStrLength
			<< "): ";

		// 通过后缀链接找到最短长度
		size_t minLen = 0;
		size_t link = states[i].szSuffixLinkTreeIndex;
		if (link != -1)
		{
			minLen = states[link].szEndposMaxStrLength + 1;
		}
		else
		{
			minLen = 1;
		}

		size_t maxLen = states[i].szEndposMaxStrLength;

		if (minLen <= maxLen && minLen > 0)
		{
			std::cout << "子串长度范围 [" << minLen << ", " << maxLen << "]";
		}
		else if (i == 0)
		{
			std::cout << "空串";
		}
		else
		{
			std::cout << "长度 = " << maxLen;
		}
		std::cout << "\n";
	}
}


int main(void)
{
	std::string s;
	std::cin >> s;

	// 初始化后缀自动机
	SuffixAutomaton<unsigned char> sam;
	sam.Init();
	sam.SetCharCount(s.length());

	// 构建SAM
	for (char c : s)
	{
		sam.AddNewChar(static_cast<unsigned char>(c - 'a'));
	}

	printSAM(sam);
	const auto &states = sam.GetListState();
	size_t n = states.size();

	// 统计每个状态的出现次数（endpos大小）
	std::vector<size_t> cnt(n, 0);

	// 每个状态的出现次数初始化为0
	// 标记每个终止状态（即每次插入新字符时创建的最后一个状态）
	// 实际上，每次AddNewChar返回的新状态就是当前整个字符串对应的状态
	// 我们需要记录所有主链上的状态
	std::vector<bool> isTerminal(n, false);

	// 获取所有主链上的状态
	size_t p = sam.GetLastStateIndex();
	while (p != -1 && p < n)
	{
		isTerminal[p] = true;
		if (states[p].szSuffixLinkTreeIndex >= n) break;
		p = states[p].szSuffixLinkTreeIndex;
	}

	// 初始化计数：每个终止状态代表一个后缀，出现次数为1
	for (size_t i = 0; i < n; i++)
	{
		if (isTerminal[i])
		{
			cnt[i] = 1;
		}
	}

	// 按长度从大到小排序
	std::vector<size_t> order(n);
	for (size_t i = 0; i < n; i++)
	{
		order[i] = i;
	}
	std::sort(order.begin(), order.end(), [&](size_t a, size_t b)
		{
			return states[a].szEndposMaxStrLength > states[b].szEndposMaxStrLength;
		});

	// 沿着后缀链接累加出现次数
	for (size_t i = 0; i < n; i++)
	{
		size_t u = order[i];
		size_t link = states[u].szSuffixLinkTreeIndex;
		if (link != -1 && link < n)
		{
			cnt[link] += cnt[u];
		}
	}

	// 计算答案：出现次数>1的子串中，长度*出现次数的最大值
	size_t ans = 0;
	for (size_t i = 1; i < n; i++)
	{ // 跳过根节点(状态0)
		if (cnt[i] > 1)
		{
			ans = std::max(ans, cnt[i] * states[i].szEndposMaxStrLength);
		}
	}

	std::cout << ans << std::endl;

	return 0;
}

int main2(void)
{
	std::vector<uint8_t> strInput;
	strInput.reserve(1000010);
	int c;
	while ((c = getchar()) != EOF && c != '\n')
	{
		strInput.push_back(c);
	}

	const auto ret = DoublingCountingRadixSortSuffixArray(INT8_MAX, strInput);
	const auto ret2 = LcpHeightArray(strInput, ret);

	for (auto &it : ret.vSuffixArray)
	{
		printf("%zu ", it);
	}

	putchar('\n');

	for (auto &it : ret2)
	{
		printf("%zu ", it);
	}

	putchar('\n');
	return 0;
}
*/


/*
#include <iostream>
#include <type_traits>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include "..\Nbs2LitematicEx\MyAlgorithm.hpp"

int main()
{
	std::ios::sync_with_stdio(false);
	std::cin.tie(nullptr);

	std::string s;
	std::cin >> s;

	using CharType = unsigned char;
	SuffixAutomaton<CharType> sam;
	sam.Init();
	sam.SetCharCount(s.size());

	for (char c : s)
	{
		sam.AddNewChar(static_cast<CharType>(c));
	}

	const auto data = sam.MoveData();

	const auto &states = data.listState;
	const auto &new_state_indices = data.listStateIndexOfChar;
	size_t state_num = states.size();
	std::vector<size_t> occurrence(state_num, 0);

	// 非克隆节点（每次添加字符产生的新节点）初始出现次数为 1
	for (size_t idx : new_state_indices)
	{
		occurrence[idx] = 1;
	}

	// 桶排序，按 len 降序
	size_t max_len = s.size();
	std::vector<std::vector<size_t>> buckets(max_len + 1);
	for (size_t i = 0; i < state_num; ++i)
	{
		buckets[states[i].szEndposMaxStrLength].push_back(i);
	}
	std::vector<size_t> order;
	order.reserve(state_num);
	for (size_t len = max_len; len > 0; --len)
	{
		for (size_t idx : buckets[len])
		{
			order.push_back(idx);
		}
	}

	// 沿后缀链接累加出现次数
	for (size_t v : order)
	{
		size_t link = states[v].szSuffixLinkTreeIndex;
		if (link != static_cast<size_t>(-1))
		{
			occurrence[link] += occurrence[v];
		}
	}

	long long ans = 0;
	for (size_t i = 0; i < state_num; ++i)
	{
		if (occurrence[i] > 1)
		{
			long long val = static_cast<long long>(occurrence[i]) * states[i].szEndposMaxStrLength;
			if (val > ans) ans = val;
		}
	}

	std::cout << ans << std::endl;
	return 0;
}
*/


#include "..\Nbs2LitematicEx\MyAlgorithm.hpp"

#include "..\Dependencies\nbt_cpp\NBT_Print.hpp"
#include "..\Dependencies\util\MyAssert.hpp"
#include <stdio.h>
#include <vector>
#include <stdint.h>
#include <ctype.h>
#include <format>
#include <cmath>
#include <algorithm>
#include <ranges>

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

uint8_t InputMapToIndex(uint8_t i)
{
	if (i >= '0' && i <= '9')
	{
		return i - (uint8_t)'0' + 0;//0~9 num --> 0-based (count 10)
	}
	else if (i >= 'a' && i <= 'z')
	{
		return i - (uint8_t)'a' + 10;//a~z alpha --> 10-based (count 26)
	}
	else
	{
		return (uint8_t)-1;
	}
}

uint8_t IndexMapToOutput(uint8_t i)
{
	if (i >= 0 && i <= 9)
	{
		return i - 0 + (uint8_t)'0';//0~9 num --> 0-based (count 10)
	}
	else if (i >= 10 && i <= 36)
	{
		return i - 10 + (uint8_t)'a';//a~z alpha --> 10-based (count 26)
	}
	else
	{
		return (uint8_t)-1;
	}
}


int main(void)
{
	std::vector<uint8_t> vInput;

main_start:
re_try:
	print("> ");
	vInput.clear();
	bool bSkip = false;
	int iGet;
	while ((iGet = getchar()) != EOF && (char)iGet != '\n')
	{
		if (bSkip)
		{
			continue;
		}

		if (isspace(iGet))
		{
			continue;
		}

		if (auto u8Map = InputMapToIndex((uint8_t)iGet); u8Map != (uint8_t)-1)
		{
			vInput.emplace_back(u8Map);
		}
		else
		{
			print("Only pure numbers and lowercase letters are accepted as input!\n");
			bSkip = true;
			continue;
		}
	}

	if (iGet == EOF)
	{
		return 0;
	}

	if (bSkip || vInput.empty())
	{
		goto re_try;
	}

	print("=========================================\n");

	//读取完成，进行计算
	auto sa_rk = SuffixArray::DoublingCountingRadixSortSuffixArray(10 + 26, vInput);
	auto lcph = SuffixArray::LcpHeightArray(vInput, sa_rk);

	MyAssert(sa_rk.vSuffixArray.size() == sa_rk.vRank.size());
	MyAssert(sa_rk.vSuffixArray.size() == lcph.size());

	//根据sa_rk输出sa，一行一个
	size_t szSize = sa_rk.vSuffixArray.size();
	size_t szZeroPerfCount = (size_t)std::log10l((long double)szSize) + 1;
	print("size: {}\n", szSize);

	print("[{:<{}}] ({:<{}} - {:<{}}): {:<{}}\n", "i", szZeroPerfCount, "SA", szZeroPerfCount, "Hi", szZeroPerfCount, "Suffix", szZeroPerfCount);
	for (size_t i = 0; i < szSize; ++i)
	{
		const auto &itSA = sa_rk.vSuffixArray[i];
		const auto &itHI = lcph[i];

		print("[{:0{}}] ({:0{}} - {:0{}}): ", i, szZeroPerfCount, itSA, szZeroPerfCount, itHI, szZeroPerfCount);
		for (size_t i = itSA; i < szSize; ++i)
		{
			putchar((uint32_t)IndexMapToOutput(vInput[i]));
		}
		putchar('\n');
	}

#define REP_SUBSTR_MIN_LENGTH 2
#define REP_SUBSTR_MIN_COUNT 2

	auto repPrint =
	[](const SuffixArray::RepeatFragmentList &rep, const auto &vInput, const char *pInfo = "") -> void
	{
		print("=========================================\n{}\n", pInfo);
		print("size: {}\n", rep.size());

		for (const auto &it : rep)
		{
			print("start index({}): ", it.vStartIndices.size());
			for (const auto &it : it.vStartIndices)
			{
				print("[{}], ", it);
			}

			print("\nval({}):", it.szPrefixLength);
			if (it.vStartIndices.empty())
			{
				print("[NULL]\n");
				continue;
			}
			for (size_t i = it.vStartIndices.front(), end = i + it.szPrefixLength; i < end; ++i)
			{
				putchar((uint32_t)IndexMapToOutput(vInput[i]));
			}
			putchar('\n');
		}
	};

	auto rep = SuffixArray::AggregateMaximalRepeats(sa_rk.vSuffixArray, lcph, REP_SUBSTR_MIN_LENGTH);
	repPrint(rep, vInput, "[AggregateMaximalRepeats]");

	//去首尾空白（数字模拟）
	//先去尾部，再去头部，效率更高
	SuffixArray::RepeatFragmentList newRep;
	newRep.reserve(rep.size());
	for (auto &it : rep)
	{
		if (it.vStartIndices.empty())
		{
			continue;
		}
		
		size_t szFrontBlankLength = 0;
		size_t szBackBlankLength = 0;
		size_t szFirstSubStrStart = it.vStartIndices.front();

		//只取其中一个重复序列判断（因为都一样，取第一即可）
		for (size_t i = szFirstSubStrStart, end = i + it.szPrefixLength; i < end; ++i)
		{
			if (vInput[i] < 10)//0~9是数字，模拟空白
			{
				++szFrontBlankLength;
			}
			else
			{
				break;//遇到非数字，跳出
			}
		}

		//判断尾部
		for (size_t end = szFirstSubStrStart + szFrontBlankLength + 1, i = szFirstSubStrStart + it.szPrefixLength; i > end; --i)//从头部空白后作为结束点
		{
			if (vInput[i - 1] < 10)//0~9是数字，模拟空白
			{
				++szBackBlankLength;
			}
			else
			{
				break;//遇到非数字，跳出
			}
		}

		//头尾都去除后，检查是否为0
		if (szFrontBlankLength == 0 && szBackBlankLength == 0)
		{
			newRep.push_back(std::move(it));//移动
			continue;//为0，那么这个序列不以空白开头、结尾，插入并跳过
		}

		//先处理尾部
		size_t szNewPrefixLength = it.szPrefixLength;
		if (szBackBlankLength != 0)
		{
			szNewPrefixLength -= szBackBlankLength;
		}

		//再处理头部，注意，头部需要遍历it.vStartIndices进行索引递增裁切，并且，如果整个串都为空则会出现
		//szNewPrefixLength小等于szFrontBlankLength，那么当前值应该被丢弃
		if (szNewPrefixLength <= szFrontBlankLength)
		{
			continue;//跳过并丢弃
		}
		szNewPrefixLength -= szFrontBlankLength;

		//串不为空，那么至少需要大等于设定值，否则丢弃
		if (szNewPrefixLength < REP_SUBSTR_MIN_LENGTH)
		{
			continue;//跳过并丢弃
		}

		//串裁切后依旧符合模式
		auto &newVal = newRep.emplace_back(szNewPrefixLength, std::move(it.vStartIndices));//提前插入，然后再处理
		for (auto &it2 : newVal.vStartIndices)//对每个索引增加szFrontBlankLength以裁切开头
		{
			it2 += szFrontBlankLength;
		}
	}

	repPrint(newRep, vInput, "[strip whitespace]");

	//贪心查找不重叠集合
	//对于每一个家族（小集合）内部的多个重复序列，先进行一次顺序贪心（集合本身需要按照索引顺序排序）
	//内部贪心因为是定长关系，可以直接根据起始索引差小于长度，直接排除重叠且不需要的子序列
	//然后对每个家族之间的重复序列进行全量长度排序，再进行一次完整的贪心，求出最终的不重叠循环串集合
	//对于最终的全量家族之间的贪心来说，如果某个家族因为被其它家族挤占生存空间而完全淘汰，
	//那么应该回滚贪心数组中被淘汰家族占用的位置以便其它家族抢占
	SuffixArray::RepeatFragmentList newGreedyRepPrep;//贪心结果
	for (auto &it : newRep)
	{
		if (it.vStartIndices.size() < 2)//至少2个元素才有筛选必要
		{
			continue;
		}

		std::ranges::sort(it.vStartIndices, std::less<>());//索引升序

		//预分配贪心筛选结果数组
		std::vector<size_t> vNewStartIndices;
		vNewStartIndices.reserve(it.vStartIndices.size());

		size_t szLastIndex = it.vStartIndices.front();//贪心起始选择开头
		vNewStartIndices.emplace_back(szLastIndex);//先行插入

		for (size_t i = 1; i < it.vStartIndices.size(); ++i)//从第二个开始比较
		{
			auto &szCurIndex = it.vStartIndices[i];

			if (szCurIndex - szLastIndex >= it.szPrefixLength)
			{
				szLastIndex = szCurIndex;
				vNewStartIndices.emplace_back(szLastIndex);
			}
			//else //跳过
			//{}
		}

		//检测剩余数量是否符合要求
		if (vNewStartIndices.size() < REP_SUBSTR_MIN_COUNT)
		{
			continue;//不符合要求，去除
		}

		//否则插入
		newGreedyRepPrep.emplace_back(it.szPrefixLength, std::move(vNewStartIndices));
	}

	repPrint(newGreedyRepPrep, vInput, "[Greedy algorithm preprocessing]");


	//区间（左闭右开）
	struct Section
	{
		size_t szBeg;//包含
		size_t szEnd;//不包含
	};
	std::vector<Section> vGreedyOccupiedArray;//区间抢占排序列表

	//使用区间二分进行贪心占座
	//首先按照L*k -> K -> L降序排序家族

	std::ranges::sort(newGreedyRepPrep,
		[](const SuffixArray::RepeatFragment &l, const SuffixArray::RepeatFragment &r)-> bool
		{
			size_t szLeftWeight = l.szPrefixLength * l.vStartIndices.size();
			size_t szRightWeight = r.szPrefixLength * r.vStartIndices.size();

			if (auto cmp = szLeftWeight <=> szRightWeight; cmp != 0)
			{
				return cmp > 0;
			}
			else if (auto cmp = l.vStartIndices.size() <=> r.vStartIndices.size(); cmp != 0)
			{
				return cmp > 0;
			}
			else
			{
				return l.szPrefixLength > r.szPrefixLength;
			}
		}
	);

	repPrint(newGreedyRepPrep, vInput, "[weight sorting]");
	
	//查找第一个区间起始下标
	auto FindSection =
	[]<typename T>(T &vGreedyOccupiedArray, size_t szSecStart) -> std::conditional_t<std::is_const_v<T>, std::vector<Section>::const_iterator, std::vector<Section>::iterator>
	requires(std::is_same_v<std::remove_cv_t<T>, std::vector<Section>>)
	{
		return std::upper_bound(vGreedyOccupiedArray.begin(), vGreedyOccupiedArray.end(), szSecStart,
			[](size_t szSecStart, const Section &info)->bool
			{
				return szSecStart < info.szBeg;
			}
		);
	};
	
	
	//判断区间是否出现碰撞，是返回true否则false
	auto IsSectionCollision =
	[&FindSection](const std::vector<Section> &vGreedyOccupiedArray, const Section &sec) -> bool
	{
		//二分查找，vGreedyOccupiedArray使用区间起始进行排序，且保证区间末尾至少小等于下一个区间起始（左闭右开保证区间末尾无法取到）
		auto itFind = FindSection(vGreedyOccupiedArray, sec.szBeg);//找到第一个后向元素

		//如果是头部，那么没有前向判断，否则判断前向区间
		if (itFind != vGreedyOccupiedArray.begin())
		{
			auto itForward = itFind - 1;
			if (sec.szBeg < itForward->szEnd)//可以等于（因为可取，如果小于则区间碰撞）
			{
				return true;
			}
		}

		//如果是末尾，那么没有后向判断，否则判断后向区间
		if (itFind != vGreedyOccupiedArray.end())//当前Find其实就是后向位置（第一个大于前向begin的元素）
		{
			const auto &itBackward = itFind;
			if (sec.szEnd > itBackward->szBeg)//可以等于（因为可取）
			{
				return true;
			}
		}

		//都未碰撞，返回false
		return false;
	};


	//此api调用需要保证：先进性过碰撞判断，确保无碰撞再插入，否则行为未定义
	auto InsertSection =
	[&FindSection]<typename T>(std::vector<Section> &vGreedyOccupiedArray, T &&sec) -> std::vector<Section>::iterator
	{
		//二分查找，vGreedyOccupiedArray使用区间起始进行排序，且保证区间末尾至少小等于下一个区间起始（左闭右开保证区间末尾无法取到）
		auto itFind = FindSection(vGreedyOccupiedArray, sec.szBeg);//找到第一个后向元素

		//在后向元素之前插入区间
		return vGreedyOccupiedArray.insert(itFind, std::forward<T>(sec));
	};

	//进行贪心抢占
	//对于每个家族来说，首先遍历家族成员，在区间抢占排序列表查找碰撞
	//如果出现碰撞则淘汰对应元素，并在临时列表中保留未碰撞的元素，
	//如果最终保留元素的数量至少大于要求K次，那么家族符合贪心要求
	//加入占座列表并抢占空位，然后处理下一家族

	//在这里，newGreedyRepPrep以按照贪心筛选规则进行排序，按序遍历并依次贪心即可获取目标结果
	std::vector<Section> vTempIndexList;//用于临时保存符合要求的家族成员范围，可复用
	SuffixArray::RepeatFragmentList newGreedyRep;//贪心结果
	for (const auto &it : newGreedyRepPrep)
	{
		vTempIndexList.clear();//清空
		for (const auto &it2 : it.vStartIndices)
		{
			Section newSec{ .szBeg = it2, .szEnd = it2 + it.szPrefixLength };

			//碰撞，跳过
			if (IsSectionCollision(vGreedyOccupiedArray, newSec))
			{
				continue;
			}

			vTempIndexList.push_back(std::move(newSec));//仅保留未碰撞
		}

		//现在已经筛选出未碰撞的所有成员，检测剩余成员数量是否符合要求
		if (vTempIndexList.size() < REP_SUBSTR_MIN_COUNT)
		{
			continue;//跳过（删除整个家族）
		}

		//否则保留家族
		//先插入长度
		auto &vNewStartIndices = newGreedyRep.emplace_back(it.szPrefixLength, SuffixArray::ValueList<size_t>{}).vStartIndices;
		vNewStartIndices.reserve(vTempIndexList.size());//提前扩容

		//成功，那么依序插入原始列表
		for (auto &it2 : vTempIndexList)
		{
			vNewStartIndices.emplace_back(it2.szBeg);//这里的big就是刚才初始化的it.vStartIndices元素
			InsertSection(vGreedyOccupiedArray, std::move(it2));//移动转移所有权
		}
	}

	//贪心完成，newGreedyRep存储所需内容
	repPrint(newGreedyRep, vInput, "[Greedy algorithm]");

	//字符串完美周期性检测与字符串单一字符组成检测


	//TODOTODOTODO















	print("=========================================\n");
	goto main_start;

	return 0;
}




