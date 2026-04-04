#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <algorithm>
#include <vector>


void PrintArr(const char *pArrName, size_t *pArr, size_t szArrLength)
{
	printf("%s[%zu]: {", pArrName, szArrLength);
	for (auto *p = pArr, *pEnd = pArr + szArrLength; p != pEnd; ++p)
	{
		printf("%zu, ", *p);
	}
	if (szArrLength != 0)
	{
		printf("\b\b");
	}

	printf("}\n");
}

#ifdef _DEBUG
#define PRINT_ARR(arr, len) PrintArr(#arr, arr, len)
#define PRINT_INF(fmt, ...) printf(fmt __VA_OPT__(,) __VA_ARGS__)
#else
#define PRINT_ARR(arr, len)
#define PRINT_INF(fmt, ...)
#endif

void DoublingCountingRadixSortSuffixArray(size_t szArrValueRange, std::vector<size_t> vSortArr)
{
	if (vSortArr.empty() || szArrValueRange == 0)
	{
		return;
	}

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

	PRINT_INF("begin\n");

	//使得排名数组作为输入数组的拷贝
	memcpy(&pRank[0], &vSortArr[0], szArraySize);

	for (size_t szStartIndex = 0; szStartIndex < szArrayLength; ++szStartIndex)
	{
		size_t &szCurValue = pRank[szStartIndex];//这里把原始值当作起始位置的排名，把排名（原始值）进行排序统计
		++pCount[szCurValue];//排序统计出现次数
	}
	for (size_t szValue = 1; szValue < szCountLength; ++szValue)
	{
		pCount[szValue] += pCount[szValue - 1];//根据值出现次数计算前缀和
	}
	for (size_t szReverseIndex = szArrayLength; szReverseIndex >= 1; --szReverseIndex)//倒序遍历以获得稳定排序顺序
	{
		size_t szStartIndex = szReverseIndex - 1;//获取倒序下标当前的起始位置
		size_t &szCurValue = pRank[szStartIndex];//获取对应起始位置的排名
		size_t &szCurLastRank = pCount[szCurValue];//获取值的出现计数前缀和
	
		//按照i-1的值进行排名
		//移动到前面（把计数值转化为索引），修改原始值，这样下一次遇到相同的值则放在前面
		pSufArr[--szCurLastRank] = szStartIndex;//根据前缀和，获得当前szStartIndex位置值最后一个排名位置szCurLastRank，设置最后一个排名位置szCurLastRank的开始下标为zStartIndex
	}
	
	PRINT_ARR(pRank, szArrayLength);
	PRINT_ARR(pSufArr, szArrayLength);

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
		PRINT_INF("for[%zu]\n", szDoublingStep);

		//第二关键字排序
		size_t szNewRank = 0;

		//先把倍增的溢出部分放到最前面，因为溢出值全为0，那么只能排第一
		for (size_t szStartIndex = szArrayLength - szDoublingStep; szStartIndex < szArrayLength; szStartIndex++)
		{
			pNextSufArr[szNewRank++] = szStartIndex;
		}

		for (size_t szRank = 0; szRank < szArrayLength; szRank++)
		{
			size_t &szStartIndex = pSufArr[szRank];
			if (szStartIndex > (szDoublingStep - 1))//获取所有排名中起始位置大于倍增量的部分
			{
				pNextSufArr[szNewRank++] = szStartIndex - szDoublingStep;//按照排名顺序，放入它的倍增配对位置的前面
			}
		}

		PRINT_ARR(pNextSufArr, szArrayLength);

		//每次计算并仅填充需要的部分（szCountLength会变化）
		memset(pCount, 0, sizeof(size_t) * szCountLength);
		//计算出现次数
		for (size_t szStartIndex = 0; szStartIndex < szArrayLength; ++szStartIndex)
		{
			size_t &szCurValue = pRank[szStartIndex];//遍历排名，把排名进行排序统计
			++pCount[szCurValue];//排序统计出现次数
		}
		for (size_t szValue = 1; szValue < szCountLength; ++szValue)
		{
			pCount[szValue] += pCount[szValue - 1];//根据值出现次数计算前缀和
		}
		for (size_t szReverseRankIndex = szArrayLength; szReverseRankIndex >= 1; --szReverseRankIndex)//根据新的后缀数组，排序原先的后缀数组
		{
			size_t szReverseRank = szReverseRankIndex - 1;//获取倒序索引对应的实际排名
			size_t &szStartIndex = pNextSufArr[szReverseRank];//获取当前排名的起始位置下标
			size_t &szCurValue = pRank[szStartIndex];//从下标获取之前对应起始位置的排名
			size_t &szCurLastRank = pCount[szCurValue];//获取值的出现计数前缀和

			//这里pNextSufArr是第二关键字的排序，根据第二关键字的排序的倒序，以及第一关键字的排序，合并为pSufArr的排序
			//移动到前面（把计数值转化为索引），修改原始值，这样下一次遇到相同的值则放在前面
			pSufArr[--szCurLastRank] = pNextSufArr[szReverseRank];//设置原先的排名位置为新的排序好的起始位置
		}

		PRINT_ARR(pSufArr, szArrayLength);
		PRINT_ARR(pRank, szArrayLength);

		//拷贝之前的排名
		memcpy(pLastRank, pRank, szArraySize);
		size_t szCurRank = 0;

		//注意因为保证szArrayLength至少为1，所以无需担心溢出

		//第0次for，特殊处理，计算上一次迭代值
		size_t szLastFirstStartIndex = pSufArr[0];//第0排名
		size_t szLastSecondStartIndex = szLastFirstStartIndex + szDoublingStep;
		size_t szLastFirstRank = pLastRank[szLastFirstStartIndex];
		size_t szLastSecondRank = szLastSecondStartIndex < szArrayLength ? pLastRank[szLastSecondStartIndex] : (size_t)-1;
		//特殊设置
		pRank[szLastFirstStartIndex] = szCurRank;

		//从1开始运行
		for (size_t szRank = 1; szRank < szArrayLength; ++szRank)//重新设置排名
		{
			//获取第一关键字起始索引，上一个在迭代中设置
			size_t szCurrentFirstStartIndex = pSufArr[szRank];

			//获取第二关键字起始索引，上一个在迭代中设置
			size_t szCurrentSecondStartIndex = szCurrentFirstStartIndex + szDoublingStep;

			//获取第一关键字排名，上一个在迭代中设置
			size_t szCurrentFirstRank = pLastRank[szCurrentFirstStartIndex];

			//获取第二关键字排名，上一个在迭代中设置
			size_t szCurrentSecondRank = szCurrentSecondStartIndex < szArrayLength ? pLastRank[szCurrentSecondStartIndex] : (size_t)-1;

			//第一、第二关键字相同判断
			if (szCurrentFirstRank == szLastFirstRank && szCurrentSecondRank == szLastSecondRank)
			{
				pRank[szCurrentFirstStartIndex] = szCurRank;//排名不变
			}
			else
			{
				pRank[szCurrentFirstStartIndex] = ++szCurRank;//排名递增
			}

			//设置当前为上一个，减少重复计算
			szLastFirstStartIndex = szCurrentFirstStartIndex;
			szLastSecondStartIndex = szCurrentSecondStartIndex;
			szLastFirstRank = szCurrentFirstRank;
			szLastSecondRank = szCurrentSecondRank;
		}

		PRINT_ARR(pRank, szArrayLength);

		//如果当前排名已经和输入数组大小相同，那么无需再次计算，直接结束
		if ((szCurRank + 1) == szArrayLength)
		{
			break;
		}

		//裁剪前缀和计算范围为当前（最后一个）排名+1
		szCountLength = szCurRank + 1;
	}

	PRINT_INF("end\n");

	for (size_t i = 0; i < szArrayLength; ++i)
	{
		printf("%zu ", pSufArr[i]);
	}

	//释放
	delete[] pBase;
	pBase = nullptr;
}

int main(void)
{
	std::vector<size_t> strInput;
	strInput.reserve(1000010);
	int c;
	while ((c = getchar()) != EOF && c != '\n')
	{
		strInput.push_back((size_t)c);
	}

	DoublingCountingRadixSortSuffixArray(INT8_MAX, strInput);
	putchar('\n');
	return 0;
}
