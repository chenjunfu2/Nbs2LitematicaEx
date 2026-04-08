#pragma once

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <vector>
#include <ranges>
#include <algorithm>
#include <numeric>
#include <type_traits>
#include <unordered_map>

class SuffixArray
{
private:
#ifdef DoublingCountingRadixSortSuffixArray_Debug
static void PrintArr(const char *pArrName, size_t *pArr, size_t szArrLength)
{
	printf("[%s](%zu): {", pArrName, szArrLength);
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

#define PRINT_ARR(arr, len) PrintArr(#arr, arr, len)
#define PRINT_INF(fmt, ...) printf(fmt __VA_OPT__(,) __VA_ARGS__)
#else
#define PRINT_ARR(arr, len)
#define PRINT_INF(fmt, ...)
#endif

public:
	template<typename T>
	using ValueList = std::vector<T>;
	
	struct ValueListPair
	{
		ValueList<size_t> vSuffixArray;
		ValueList<size_t> vRank;
	};
	
public:
	//返回值为排序在vSortArr中的下标，所以是size_t
	template<typename T>
	requires(std::is_integral_v<T> && std::is_unsigned_v<T> && sizeof(T) <= sizeof(size_t))
	static ValueListPair DoublingCountingRadixSortSuffixArray(size_t szArrValueRange, const ValueList<T> &vInputArr)//szArrValueRange是上边界，无法取到
	{
		//拒绝空值
		if (vInputArr.empty() || szArrValueRange == 0)
		{
			return {};
		}
	
		//计算长度
		size_t szArrayLength = vInputArr.size();//计算后缀、排名数组长度
		size_t szCountLength = szArrValueRange > szArrayLength ? szArrValueRange : szArrayLength;//计算排序数组长度（值域或数组长度较大的那个）
	
		//计算大小
		size_t szArraySize = sizeof(size_t) * szArrayLength;
		size_t szCountSize = sizeof(size_t) * szCountLength;
	
		//用1字节大小分配：1个排序数组+4个后缀排名数组
		uint8_t *pBase = new uint8_t[szCountSize + szArraySize * 4];
		uint8_t *pMove = pBase;
	
		//裁切内存，依次分配
		size_t *pCount =		(size_t *)pMove; pMove += szCountSize;
		size_t *pRank =			(size_t *)pMove; pMove += szArraySize;
		size_t *pLastRank =		(size_t *)pMove; pMove += szArraySize;
		size_t *pSufArr =		(size_t *)pMove; pMove += szArraySize;
		size_t *pNextSufArr =	(size_t *)pMove; pMove += szArraySize;
	
		//填0初始化
		memset(pCount,		0, szCountSize);
		memset(pRank,		0, szArraySize);
		memset(pLastRank,	0, szArraySize);
		memset(pSufArr,		0, szArraySize);
		memset(pNextSufArr,	0, szArraySize);
	
		PRINT_INF("begin\n");
	
		for (size_t szStartIndex = 0; szStartIndex < szArrayLength; ++szStartIndex)
		{
			size_t &szCurValue = pRank[szStartIndex];//这里把原始值当作起始位置的排名，把排名（原始值）进行排序统计
			szCurValue = (size_t)vInputArr[szStartIndex];//使得排名数组作为输入数组的拷贝
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
			size_t szNewRank = (size_t)-1;//利用-1环绕，使得后续可以使用前缀递增减少开销
	
			//先把倍增的溢出部分放到最前面，因为溢出值全为0，那么只能排第一
			for (size_t szStartIndex = szArrayLength - szDoublingStep; szStartIndex < szArrayLength; szStartIndex++)
			{
				pNextSufArr[++szNewRank] = szStartIndex;
			}
	
			for (size_t szRank = 0; szRank < szArrayLength; szRank++)
			{
				size_t &szStartIndex = pSufArr[szRank];
				if (szStartIndex > (szDoublingStep - 1))//获取所有排名中起始位置大于倍增量的部分
				{
					pNextSufArr[++szNewRank] = szStartIndex - szDoublingStep;//按照排名顺序，放入它的倍增配对位置的前面
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
	
		//准备返回值
		ValueListPair retPair;
		retPair.vSuffixArray.resize(szArrayLength);
		memcpy(&retPair.vSuffixArray[0], pSufArr, szArraySize);
	
		retPair.vRank.resize(szArrayLength);
		memcpy(&retPair.vRank[0], pRank, szArraySize);
	
		//释放
		delete[] pBase;
		pBase = nullptr;
	
		return retPair;
	}
	
	template<typename T>
	requires(std::is_integral_v<T> &&std::is_unsigned_v<T> && sizeof(T) <= sizeof(size_t))
	static ValueList<size_t> LcpHeightArray(const ValueList<T> &vInputArr, const ValueListPair &vlp)
	{
		if (vInputArr.empty())
		{
			return {};
		}
	
		size_t szArrayLength = vlp.vRank.size();//size is length not sizeof(value_type) * length
	
		ValueList<size_t> vHeight;
		vHeight.resize(szArrayLength);
	
		/*
		Height[排名]=排名与前一名的最长公共前缀长度
		Height[i] = LCP(SA[i-1], SA[i])
		*/
	
		for (size_t szStartIndex = 0, szMatchLength = 0; szStartIndex < szArrayLength; ++szStartIndex)
		{
			//排名0没有上一排名，跳过
			const size_t &szCurrentRank = vlp.vRank[szStartIndex];
			if (szCurrentRank == 0)
			{
				vHeight[szCurrentRank] = 0;
				continue;
			}
	
			//如果不为0则递减1
			szMatchLength -= (size_t)(szMatchLength != 0);
	
			//获取上一名的开始位置
			size_t szLastRank = szCurrentRank - 1;
			size_t szLastStartIndex = vlp.vSuffixArray[szLastRank];
	
			//如果字符串未溢出且匹配，那么继续
			while (szStartIndex + szMatchLength < szArrayLength && szLastStartIndex + szMatchLength < szArrayLength &&
				   vInputArr[szStartIndex + szMatchLength] == vInputArr[szLastStartIndex + szMatchLength])
			{
				++szMatchLength;
			}
	
			//不匹配或溢出，设置当前为最长前缀
			vHeight[szCurrentRank] = szMatchLength;
		}
		
		return vHeight;
	}
};

/*
SAM后缀自动机：有向无环图+后缀链接树
有向无环图指明了所有的子串模式，后缀链接树指明了所有符合具有相同的endpos的集合
遍历有向无环图获取子串，遍历后缀链接树获取具有更短长度的endpos集合（也就是上一节点是当前节点的子集，且共享更短的同一后缀）

每个节点作为一个状态（State），每个状态同时作为两个数据结构的节点，
其中next指明了当前节点的有向无环图的可迁移边，
length指明了后缀链接树当前节点所代表的endpos集合中最长子串的长度，长度可用于检测约束条件
suffixlink则是后缀链接树的父节点（用于反向遍历）

构建SAM的时候，对于原始字符串，需要每次更新下一个字符，
然后从上一SAM状态与新字符整体迁移到下一状态，
存在3种情况：
1.新加入的字符在当前自动机里，完全不存在，那么更新图和树的节点指向新字符节点
2.新加入的字符在当前自动机里，已经存在一个现有的转移并且是一个新后缀，更新图并判断新节点应该在树的哪一个endpos集后面，使得新节点是原先节点的超集
3.新加入的字符在当前自动机里，已经存在一个现有的转移，但不完全是现有集合的后缀，那么找出相同的后缀，拷贝原始节点并分裂出新后缀，向上依次更新所有指向原始节点的节点指向新分裂的节点，并将新字符作为分裂节点的超集
*/
template<typename T>//T是字符类型
requires(std::is_integral_v<T> && std::is_unsigned_v<T> && sizeof(T) <= sizeof(size_t))
class SuffixAutomaton
{
public:
	struct State
	{
		std::unordered_map<T, size_t> mapTransitionNextIndex{};//转移边: mapTransitionNextIndex[字符] = 到达的状态编号下标
		size_t szEndposMaxStrLength = 0;	//该状态endpos集合中最长的子串长度
		size_t szSuffixLinkTreeIndex = -1;	//当前节点在后缀链接树中的子集所在的状态下标（后缀链接树上一节点）
	};

	using StateList = std::vector<State>;
	using IndexList = std::vector<size_t>;

	struct Data
	{
		size_t szLastStateIndex;//上一个处理的状态下标
		//size_t szTotalStateIndex;//状态总数（总是递增，用于分配新节点，事实上listState已有，节约一个变量）
		StateList listState;//状态列表
		IndexList listStateIndexOfChar;//每个字符插入后的对应状态
	};

private:
	Data data;//状态机数据

public:
	SuffixAutomaton(void) = default;
	~SuffixAutomaton(void) = default;
	SuffixAutomaton(const SuffixAutomaton &) = default;
	SuffixAutomaton(SuffixAutomaton &&) = default;
	SuffixAutomaton &operator=(const SuffixAutomaton &) = default;
	SuffixAutomaton &operator=(SuffixAutomaton &&) = default;

public:
	const Data &GetData(void) const noexcept
	{
		return data;
	}

	Data &&MoveData(void) noexcept
	{
		return std::move(data);
	}

	void Reset(void)
	{
		data.szLastStateIndex = 0;
		//szTotalStateIndex = 0;
		data.listState.clear();
		data.listStateIndexOfChar.clear();
	}

	void Init(void)
	{
		//重置
		Reset();

		//添加第0元素（根）
		data.listState.emplace_back();
	}

	//从字符长度设置总状态数
	void SetCharCount(size_t szCharCount)
	{
		size_t szStateCount = szCharCount * 2 - 1;
		if (szStateCount > data.listState.size())
		{
			data.listState.reserve(szStateCount);
		}

		if (szCharCount > data.listStateIndexOfChar.size())
		{
			data.listStateIndexOfChar.reserve(szCharCount);
		}
	}

	void AddNewChar(T tNewChar)//每次调用返回当前新的状态（也相当于上一个状态）
	{
		size_t szCurStateIndex = data.szLastStateIndex;
		size_t szNewStateIndex = data.listState.size();
		data.szLastStateIndex = szNewStateIndex;
		data.listState.emplace_back
		(
			State
			{
				.mapTransitionNextIndex{},
				.szEndposMaxStrLength = data.listState[szCurStateIndex].szEndposMaxStrLength + 1,
				.szSuffixLinkTreeIndex = (size_t)-1,
			}
		);//新增元素，下标刚好就是上一个size
		data.listStateIndexOfChar.push_back(szNewStateIndex);//设置新增元素的下标

		//遍历后缀链接，给所有图上添加新转状态的转移
		size_t szNextStateIndex = 0;
		do
		{
			auto &stateCur = data.listState[szCurStateIndex];
			//尝试添加
			auto [it, b] = stateCur.mapTransitionNextIndex.try_emplace(tNewChar, szNewStateIndex);
			if (b == false)//如果添加失败，则说明图当前节点已有相同字符的不同出边
			{
				szNextStateIndex = it->second;//获取阻止插入的节点下标
				break;
			}

			//回溯节点
			szCurStateIndex = stateCur.szSuffixLinkTreeIndex;
		} while (szCurStateIndex != -1);

		//如果while因为找到头部退出，那么此字符未出现过，为情况1，链接树然后离开
		if (szCurStateIndex == -1)
		{
			data.listState[szNewStateIndex].szSuffixLinkTreeIndex = 0;//链接到根部
			return;
		}

		//这里开始使用刚才阻止插入节点的下标szNextStateIndex
		//如果阻止插入的已有转移的节点刚好是当前遍历节点+1，那么说明是连续状态，直接连接到阻止节点后
		if (data.listState[szNextStateIndex].szEndposMaxStrLength ==
			data.listState[szCurStateIndex].szEndposMaxStrLength + 1)
		{
			data.listState[szNewStateIndex].szSuffixLinkTreeIndex = szNextStateIndex;
			return;
		}

		//否则进行节点拷贝与分裂
		size_t szCloneStateIndex = data.listState.size();
		data.listState.emplace_back
		(
			State
			{
				.mapTransitionNextIndex = data.listState[szNextStateIndex].mapTransitionNextIndex,//从被克隆的节点拷贝数据
				.szEndposMaxStrLength = data.listState[szCurStateIndex].szEndposMaxStrLength + 1,//设置长度为当前状态而非克隆节点数据
				.szSuffixLinkTreeIndex = data.listState[szNextStateIndex].szSuffixLinkTreeIndex,//从被克隆的节点拷贝数据
			}
		);//从szNextStateIndex拷贝并新增元素，下标刚好就是上一个size，

		//让下一个状态和新状态都指向拷贝状态，这样树就把被克隆的原来的节点分离出来
		data.listState[szNextStateIndex].szSuffixLinkTreeIndex = szCloneStateIndex;
		data.listState[szNewStateIndex].szSuffixLinkTreeIndex = szCloneStateIndex;

		//最后把之前指向szNextStateIndex的全部移动到szCloneStateIndex
		do
		{
			auto &stateCur = data.listState[szCurStateIndex];

			//查找所有还指向szNextStateIndex的改成szCloneStateIndex
			auto it = stateCur.mapTransitionNextIndex.find(tNewChar);
			if (it != stateCur.mapTransitionNextIndex.end() &&
				it->second == szNextStateIndex)
			{
				it->second = szCloneStateIndex;
			}

			//回溯节点
			szCurStateIndex = stateCur.szSuffixLinkTreeIndex;
		} while (szCurStateIndex != -1);

		return;
	}

	static IndexList CountOccurrences(const Data &data)
	{
		//初始化：每个终止状态次数 = 1
		IndexList listOccCount(data.listState.size(), 0);//初始化填0
		for (const auto &szCharIndex : data.listStateIndexOfChar)
		{
			listOccCount[szCharIndex] = 1;//字符状态为1
		}

		//按长度降序排序（长的先处理）
		IndexList listOrderIndex(data.listState.size());
		std::iota(listOrderIndex.begin(), listOrderIndex.end(), 0);
		std::ranges::sort(listOrderIndex,
			[&](const size_t &l, const size_t &r) -> bool
			{
				return data.listState[l].szEndposMaxStrLength > data.listState[r].szEndposMaxStrLength;
			}
		);

		//累加到后缀链接父节点
		for (const auto &szOrderIndex : listOrderIndex)
		{
			const auto &szTreeIndex = data.listState[szOrderIndex].szSuffixLinkTreeIndex;
			if (szTreeIndex != -1)
			{
				listOccCount[szTreeIndex] += listOccCount[szOrderIndex];
			}
		}
		
		return listOccCount;
	}


};
