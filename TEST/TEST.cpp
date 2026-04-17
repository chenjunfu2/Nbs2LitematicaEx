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

	
	auto newRep = FragmentTrimmer::TrimBoundaries(rep, REP_SUBSTR_MIN_COUNT,
		[&vInput](size_t szIndex)->bool
		{
			return vInput[szIndex] < 10;
		}
	);
	repPrint(newRep, vInput, "[strip whitespace]");


	auto newGreedyRepPrep = GreedyAlgorithm::GreedyNonOverlapPerFragment(newRep, REP_SUBSTR_MIN_COUNT);
	repPrint(newGreedyRepPrep, vInput, "[GreedyNonOverlapPerFragment]");

	GreedyAlgorithm::GreedySortFragments(newGreedyRepPrep, GreedyAlgorithm::DefaultGreedySort);
	repPrint(newGreedyRepPrep, vInput, "[weight sorting]");


	auto newGreedyRep = GreedyAlgorithm::GreedyNonOverlapAcrossFragments(newGreedyRepPrep, REP_SUBSTR_MIN_COUNT);
	//贪心完成，newGreedyRep存储所需内容
	repPrint(newGreedyRep, vInput, "[Greedy algorithm]");

	//字符串完美周期性检测与字符串单一字符组成检测
	//对贪心完成的每个数组进行检测
	print("=========================================\n[CheckPeriodicity]\n");
	for (auto &it : newGreedyRep)
	{
		auto PmArray = PeriodicityDetector::ComputePartialMatch(it.vStartIndices);
		auto szPeriodLength = PeriodicityDetector::CheckPeriodicity(it.vStartIndices, PmArray);

		if (szPeriodLength != 0)
		{
			print("\nperiod: [{}]\nval({}):", szPeriodLength, it.szPrefixLength);
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
	}

	print("=========================================\n");
	goto main_start;

	return 0;
}
