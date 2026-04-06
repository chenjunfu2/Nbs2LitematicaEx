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
