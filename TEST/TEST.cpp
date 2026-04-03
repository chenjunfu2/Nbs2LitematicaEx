#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <algorithm>
#include <vector>

void DoublingCountingRadixSortSuffixArray(std::vector<char> strInput)
{
	size_t szArrayLength = strInput.size();
	size_t szCountMaxVal = std::max((size_t)127, szArrayLength);

	size_t szCountSize = sizeof(size_t) * (szCountMaxVal + 2);
	size_t szArraySize = sizeof(size_t) * (szArrayLength + 2);
	uint8_t *pBase = new uint8_t[szCountSize + szArraySize * (4 + 2)];

	uint8_t *pMove = pBase;
	size_t *pCount = (size_t *)pMove; pMove += szCountSize;

	size_t *pRank =			(size_t *)pMove; pMove += szArraySize * 2;
	size_t *pLastRank =		(size_t *)pMove; pMove += szArraySize * 1;
	size_t *pSufArr =		(size_t *)pMove; pMove += szArraySize * 2;
	size_t *pLastSufArr =	(size_t *)pMove; pMove += szArraySize * 1;

	memset(pCount, 0, szCountSize);

	memset(pRank, 0, szArraySize * 2);//后半额外填充
	memset(pLastRank, 0, szArraySize);
	memset(pSufArr, 0, szArraySize * 2);//后半额外填充
	memset(pLastSufArr, 0, szArraySize);

	//第一关键字排序
	//计算出现次数
	for (size_t i = 1; i <= szArrayLength; ++i)
	{
		++pCount[pRank[i] = strInput[i - 1]];
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
		memset(pCount, 0, szCountSize);//仅填充需要的部分
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

	for (size_t i = 1; i <= szArrayLength; ++i)
	{
		printf("%zu ", pSufArr[i]);
	}

	//释放
	delete[] pBase;
	pBase = nullptr;
}

int main(void)
{
	std::vector<char> strInput;
	strInput.reserve(1000010);
	int c;
	while ((c = getchar()) != EOF && c != '\n')
	{
		strInput.push_back(c);
	}

	DoublingCountingRadixSortSuffixArray(strInput);
	putchar('\n');
	return 0;
}
