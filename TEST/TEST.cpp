#include "../Nbs2LitematicEx/MyAlgorithm.hpp"
#include <stdio.h>
#include <iostream>
#include <string>

using namespace std;
int main()
{
	string s;
	std::getline(std::cin, s, '\n');

	SuffixAutomaton<unsigned char> sam;
	sam.SetCharCount(s.size());

	vector<size_t> lastState(s.size());  // 记录每个前缀对应的状态索引

	// 构建 SAM，同时记录每个 np
	for (size_t i = 0; i < s.size(); i++)
	{
		lastState[i] = sam.AddNewChar(static_cast<unsigned char>(s[i] - 'a'));
	}

	const auto &states = sam.GetListState();
	size_t tot = states.size();

	// 外部 cnt 数组
	vector<size_t> cnt(tot, 0);

	// 每个 np 出现次数 +1
	for (size_t idx : lastState)
	{
		cnt[idx]++;
	}

	// 基数排序（按 szLongestLen 从大到小）
	vector<size_t> tax(tot + 1, 0);
	vector<size_t> id(tot, 0);

	for (size_t i = 0; i < tot; i++)
	{
		tax[states[i].szEndposMaxStrLength]++;
	}
	for (size_t i = 1; i <= tot; i++)
	{
		tax[i] += tax[i - 1];
	}
	for (size_t i = 0; i < tot; i++)
	{
		id[--tax[states[i].szEndposMaxStrLength]] = i;
	}

	// 按长度从大到小遍历，累加 cnt 到后缀链接父节点
	size_t ans = 0;
	for (int i = tot - 1; i >= 0; i--)
	{
		size_t u = id[i];
		size_t f = states[u].szSuffixLinkTreeIndex;
		if (f != -1)
		{
			cnt[f] += cnt[u];
		}
		if (cnt[u] > 1)
		{
			ans = max(ans, 1LL * cnt[u] * states[u].szEndposMaxStrLength);
		}
	}

	printf("%lld\n", ans);
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
