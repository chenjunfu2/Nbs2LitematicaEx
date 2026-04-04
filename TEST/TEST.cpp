#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <algorithm>
#include <vector>

void DoublingCountingRadixSortSuffixArray(size_t szArrValueRange, std::vector<size_t> vSortArr)
{
	size_t szArrayLength = vSortArr.size();//计算后缀、排名数组长度
	size_t szCountLength = std::max(szArrValueRange, szArrayLength);//计算排序数组长度（值域或数组长度较大的那个）

	//计算它们的大小
	size_t szArraySize = sizeof(size_t) * szArrayLength;
	size_t szCountSize = sizeof(size_t) * szCountLength;
	
	//用1字节大小分配：1个排序数组+4个后缀排名数组
	uint8_t *pBase = new uint8_t[szCountSize + szArraySize * 4];
	uint8_t *pMove = pBase;

	//裁切内存，依次分配
	size_t *pCount =		(size_t *)pMove; pMove += szCountSize;
	size_t *pRank =			(size_t *)pMove; pMove += szArraySize * 1;
	size_t *pLastRank =		(size_t *)pMove; pMove += szArraySize * 1;
	size_t *pSufArr =		(size_t *)pMove; pMove += szArraySize * 1;
	size_t *pNextSufArr =	(size_t *)pMove; pMove += szArraySize * 1;

	//填0
	memset(pCount, 0, szCountSize);
	memset(pRank, 0, szArraySize);
	memset(pLastRank, 0, szArraySize);
	memset(pSufArr, 0, szArraySize);
	memset(pNextSufArr, 0, szArraySize);

	//使得排名数组作为输入数组的拷贝
	memcpy(&pRank[0], &vSortArr[0], szArraySize);

	for (size_t szStartIndex = 0; szStartIndex < szArrayLength; ++szStartIndex)
	{
		auto &curValue = pRank[szStartIndex];//这里把原始值当作起始位置的排名
		++pCount[curValue];//排序统计出现次数
	}
	for (size_t szValue = 1; szValue < szCountLength; ++szValue)
	{
		pCount[szValue] += pCount[szValue - 1];//根据出现次数计算前缀和
	}
	for (size_t szReverseIndex = szArrayLength; szReverseIndex >= 1; --szReverseIndex)//倒序遍历以获得稳定排序顺序
	{
		auto szStartIndex = szReverseIndex - 1;//获取倒序下标当前的起始位置
		auto &curValue = pRank[szStartIndex];//获取对应起始位置的排名
		auto &curCount = pCount[curValue];//获取值的出现计数前缀和
	
		//按照i-1的值进行排名
		pSufArr[curCount] = szStartIndex;//根据前缀和，获得当前szStartIndex位置值最后一个排名位置curCount，设置最后一个排名位置curCount的开始下标为zStartIndex
		--curCount;//移动到前面，这样下一次遇到相同的值则放在前面
	}

	/*
	备注：
	起始位置：当前子串在原始数组中开始的下标
	排名：当前（下标指代的）子串在原始数组中的排序位置

	SufArr[排名]=起始位置
	Rank[起始位置]=排名
	（SufArr与Rank互为反相）

	Count[数值]=出现次数/出现次数的前缀和
	*/

	//进行倍增
	for (size_t szDoublingStep = 1; true; szDoublingStep *= 2)
	{
		//第二关键字排序
		size_t szSufArrIndex = 0;

		//先把倍增的溢出部分放到最前面，因为溢出值全为0，那么只能排第一
		for (size_t i = szArrayLength - szDoublingStep; i < szArrayLength; i++)
		{
			pNextSufArr[++szSufArrIndex] = i;
		}

		for (size_t i = 0; i < szArrayLength; i++)
		{
			if (pSufArr[i] > (szDoublingStep - 1))//获取所有排名i大于倍增其实位置的部分
			{
				pNextSufArr[++szSufArrIndex] = pSufArr[i] - (szDoublingStep - 1);//按照排名顺序，放入它的倍增配对位置的前面
			}
		}

		//每次计算并仅填充需要的部分（szCountLength会变化）
		memset(pCount, 0, sizeof(size_t) * szCountLength);
		//计算出现次数
		for (size_t i = 0; i < szArrayLength; ++i)
		{
			auto &curStartIndex = pNextSufArr[i];//依次获取当前排名i的起始位置
			auto &curValue = pRank[curStartIndex];//根据起始位置依排名顺序得到排序的值
			++pCount[curValue];//统计排序的值
		}
		for (size_t i = 1; i < szCountLength; ++i)
		{
			pCount[i] += pCount[i - 1];//前缀和
		}
		for (size_t szReverseIndex = szArrayLength; szReverseIndex >= 1; --szReverseIndex)
		{
			auto szStartIndex = szReverseIndex - 1;//获取倒序下标当前的起始位置
			auto &curValue = pRank[szStartIndex];//获取对应起始位置的排名
			auto &curCount = pCount[curValue];//获取值的出现计数前缀和

			pSufArr[curCount] = pNextSufArr[i];//设置原先的排名位置为新的
		}

		size_t szCurRank = 0;
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

		szCountLength = szCurRank;
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
