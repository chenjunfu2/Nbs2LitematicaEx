#pragma once

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <vector>
#include <span>
#include <ranges>
#include <algorithm>
#include <numeric>
#include <type_traits>
#include <unordered_map>


//后缀数组求解相关
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

	template<typename T>
	using ValueView = std::span<const T>;
	
	struct ValueListPair
	{
		ValueList<size_t> vSuffixArray;
		ValueList<size_t> vRank;
	};
	
	struct RepeatFragment
	{
		size_t szPrefixLength;
		ValueList<size_t> vStartIndices;
	};

	using RepeatFragmentList = std::vector<RepeatFragment>;

public:
	//返回值为排序在vSortArr中的下标，所以是size_t
	template<typename T>
	requires(std::is_integral_v<T> && std::is_unsigned_v<T> && sizeof(T) <= sizeof(size_t))
	static ValueListPair DoublingCountingRadixSortSuffixArray(const ValueView<T> &vInputArr, size_t szArrValueRange)//szArrValueRange是上边界，无法取到
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
	static ValueList<size_t> LcpHeightArray(const ValueView<T> &vInputArr, const ValueListPair &vlp)
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

	static RepeatFragmentList AggregateMaximalRepeats(const std::vector<size_t> &vSuffixArray, const std::vector<size_t> &vLcpHeight, size_t szMinLength)
	{
		RepeatFragmentList vRepeatFragment;
		size_t szLcpLength = vLcpHeight.size();

		//LCP数组长度等于序列长度N，且lcp[0]==0
		if (szLcpLength < 2)//lcp长度至少为2，否则值为空
		{
			return vRepeatFragment;
		}

		for (size_t i = 1; i < szLcpLength; ++i)
		{
			const size_t &szCurHeigh = vLcpHeight[i];

			//阈值过滤：低于最小有效长度直接跳过
			if (szCurHeigh < szMinLength)
			{
				continue;
			}

			//平台起点判定：仅当当前值大于左侧邻居时，视为新家族起点，否则跳过
			if (szCurHeigh <= vLcpHeight[i - 1])//小于等于，跳过
			{
				continue;
			}

			//现在i-1是左边界起始索引
			size_t szLeftBound = i - 1;

			//右边界扩张：寻找当前平台的最右延伸点
			//szRightBound在循环结束后是右边界索引结束位置（右边界不包含）
			size_t szRightBound = i + 1;//从当前的下一个开始，因为已经判断过当前小于前一个
			while (szRightBound < szLcpLength && vLcpHeight[szRightBound] >= szCurHeigh)//下一个还大于那么继续
			{
				++szRightBound;
			}

			//收集位置：SA中左闭右开区间[i-1, j)对应的所有起始索引
			std::vector<size_t> vPositions;
			vPositions.reserve(szRightBound - szLeftBound);
			for (size_t k = szLeftBound; k < szRightBound; ++k)
			{
				vPositions.push_back(vSuffixArray[k]);
			}

			//完成，放入返回列表
			vRepeatFragment.emplace_back(szCurHeigh, std::move(vPositions));
		}

		return vRepeatFragment;
	}

#undef PRINT_INF
#undef PRINT_ARR
};


//后缀自动机
template<typename T>//T是字符或数值类型
requires(std::is_integral_v<T>)
class SuffixAutomaton
{
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
public:
	struct State
	{
		std::unordered_map<T, size_t> mapTransitionNextIndex{};//转移边: mapTransitionNextIndex[字符] = 到达的状态编号下标
		size_t szEndposMaxStrLength = 0;	//该状态endpos集合中最长的子串长度
		size_t szSuffixLinkTreeIndex = -1;	//当前节点在后缀链接树中的子集所在的状态下标（后缀链接树上一节点）
	};

	using StateList = std::vector<State>;
	using IndexList = std::vector<size_t>;
	using CountList = std::vector<size_t>;

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

	//返回值listOccCount[szStateIndex] = 该状态代表的子串在全文中的出现次数
	static CountList CountOccurrences(const Data &data)
	{
		//初始化：每个终止状态次数 = 1
		CountList listOccCount(data.listState.size(), 0);//初始化填0
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


//片段头尾模式去除与筛选
class FragmentTrimmer
{
	FragmentTrimmer(void) = delete;
	~FragmentTrimmer(void) = delete;

public:
	//去首尾空白（数字模拟）
	//先去尾部，再去头部，效率更高

	template<typename TrimFunc_T>
	static SuffixArray::RepeatFragmentList TrimBoundaries(const SuffixArray::RepeatFragmentList &listRepeatFragment, size_t szMinRepeatCount, TrimFunc_T funcTrim)
	{
		SuffixArray::RepeatFragmentList listTrimRepeatFragment;
		listTrimRepeatFragment.reserve(listRepeatFragment.size());

		for (const auto &it : listRepeatFragment)
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
				if (funcTrim(i))//需要去除
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
				if (funcTrim(i - 1))//需要去除
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
				listTrimRepeatFragment.push_back(std::move(it));//移动
				continue;//为0，那么这个序列不以去除内容开头、结尾，插入并跳过
			}

			//先处理尾部，尾部仅需要修改总长度即可去除所有
			size_t szNewPrefixLength = it.szPrefixLength;
			if (szBackBlankLength != 0)
			{
				szNewPrefixLength -= szBackBlankLength;
			}

			//再处理头部，注意，头部需要遍历it.vStartIndices进行索引递增裁切，
			//并且，如果整个串裁切后为空则会出现szNewPrefixLength小等于szFrontBlankLength，
			//那么这种情况下，当前值应该被丢弃
			if (szNewPrefixLength <= szFrontBlankLength)
			{
				continue;//跳过并丢弃
			}
			szNewPrefixLength -= szFrontBlankLength;

			//串不为空，那么至少需要大等于设定值，否则丢弃
			if (szNewPrefixLength < szMinRepeatCount)
			{
				continue;//跳过并丢弃
			}

			//串裁切后依旧符合模式
			auto &newVal = listTrimRepeatFragment.emplace_back(szNewPrefixLength, std::move(it.vStartIndices));//提前插入，然后再处理
			for (auto &it2 : newVal.vStartIndices)//对每个索引增加szFrontBlankLength以裁切开头
			{
				it2 += szFrontBlankLength;
			}
		}

		return listTrimRepeatFragment;
	}
};


//贪心求不重叠
class GreedyAlgorithm
{
	GreedyAlgorithm(void) = delete;
	~GreedyAlgorithm(void) = delete;

private:
	struct GreedySectionOccupiedArray
	{
	public:
		//区间（左闭右开）
		struct Section
		{
			size_t szBeg;//包含
			size_t szEnd;//不包含
		};

		using SectionList = std::vector<Section>;

	public:
		SectionList listSection;

	public:
		//查找第一个大于szSecStart的区间起始迭代器
		SectionList::iterator FindSection(size_t szSecStart)
		{
			return std::upper_bound(listSection.begin(), listSection.end(), szSecStart,
				[](size_t szSecStart, const Section &info) -> bool
				{
					return szSecStart < info.szBeg;
				}
			);
		}

		//查找第一个大于szSecStart的区间起始迭代器
		SectionList::const_iterator FindSection(size_t szSecStart) const
		{
			return std::upper_bound(listSection.begin(), listSection.end(), szSecStart,
				[](size_t szSecStart, const Section &info) -> bool
				{
					return szSecStart < info.szBeg;
				}
			);
		}

		//判断区间是否出现碰撞，是返回true否则false
		bool IsSectionCollision(const Section &sec) const
		{
			//二分查找，vGreedyOccupiedArray使用区间起始进行排序，且保证区间末尾至少小等于下一个区间起始（左闭右开保证区间末尾无法取到）
			auto itFind = FindSection(sec.szBeg);//找到第一个后向元素

			//如果是头部，那么没有前向判断，否则判断前向区间
			if (itFind != listSection.begin())
			{
				auto itForward = itFind - 1;
				if (sec.szBeg < itForward->szEnd)//可以等于（因为可取，如果小于则区间碰撞）
				{
					return true;
				}
			}

			//如果是末尾，那么没有后向判断，否则判断后向区间
			if (itFind != listSection.end())//当前Find其实就是后向位置（第一个大于前向begin的元素）
			{
				const auto &itBackward = itFind;
				if (sec.szEnd > itBackward->szBeg)//可以等于（因为可取）
				{
					return true;
				}
			}

			//都未碰撞，返回false
			return false;
		}

		//此api调用需要保证：先进性过碰撞判断，确保无碰撞再插入，否则行为未定义
		SectionList::iterator InsertSection(Section &&sec)
		{
			//二分查找，vGreedyOccupiedArray使用区间起始进行排序，且保证区间末尾至少小等于下一个区间起始（左闭右开保证区间末尾无法取到）
			auto itFind = FindSection(sec.szBeg);//找到第一个后向元素

			//在后向元素之前插入区间
			return listSection.insert(itFind, std::move(sec));
		}

		//此api调用需要保证：先进性过碰撞判断，确保无碰撞再插入，否则行为未定义
		SectionList::iterator InsertSection(const Section &sec)
		{
			//二分查找，vGreedyOccupiedArray使用区间起始进行排序，且保证区间末尾至少小等于下一个区间起始（左闭右开保证区间末尾无法取到）
			auto itFind = FindSection(sec.szBeg);//找到第一个后向元素

			//在后向元素之前插入区间
			return listSection.insert(itFind, sec);
		}
	};

public:
	//默认排序函数：按照L * F -> L -> F降序排序家族，L是长度Length，F是出现频率Frequency
	static bool DefaultGreedySort(const SuffixArray::RepeatFragment &l, const SuffixArray::RepeatFragment &r)
	{
		size_t szLeftWeight = l.szPrefixLength * l.vStartIndices.size();
		size_t szRightWeight = r.szPrefixLength * r.vStartIndices.size();

		if (auto cmpWeight = szLeftWeight <=> szRightWeight; cmpWeight != 0)
		{
			return cmpWeight > 0;
		}
		else if (auto cmpLength = l.szPrefixLength <=> r.szPrefixLength; cmpLength != 0)
		{
			return cmpLength > 0;
		}
		else
		{
			auto cmpFrequency = l.vStartIndices.size() <=> r.vStartIndices.size();
			return cmpFrequency > 0;
		}
	}

public:
	//贪心查找不重叠集合：
	//对于每一个家族（小集合）内部的多个重复序列，先进行一次顺序贪心（集合本身需要按照索引顺序排序）
	//内部贪心因为是定长关系，可以直接根据起始索引差小于长度，直接排除重叠且不需要的子序列
	//然后对每个家族之间的重复序列进行全量长度排序，再进行一次完整的贪心，求出最终的不重叠循环串集合
	//使用区间二分进行贪心占座：首先按照L*k -> K -> L降序排序家族，然后进行贪心抢占
	//对于每个家族来说，首先遍历家族成员，在区间抢占排序列表查找碰撞
	//如果出现碰撞则淘汰对应元素，并在临时列表中保留未碰撞的元素，
	//如果最终保留元素的数量至少大于要求K次，那么家族符合贪心要求
	//加入占座列表并抢占空位，然后处理下一家族


	//对每个Fragment内的vStartIndices进行贪心区间排重
	static SuffixArray::RepeatFragmentList GreedyNonOverlapPerFragment(const SuffixArray::RepeatFragmentList &listRepeatFragment, size_t szMinRepeatCount)
	{
		if (szMinRepeatCount < 2)//小于2个重复没有计算必要
		{
			return {};
		}

		SuffixArray::RepeatFragmentList listGreedyRepeatFragment;//贪心结果
		for (const auto &it : listRepeatFragment)
		{
			if (it.vStartIndices.size() < szMinRepeatCount)//至少szMinRepeatCount个元素才有筛选必要
			{
				continue;//直接丢弃
			}

			//拷贝并排序
			std::vector<size_t> vSortStartIndices = it.vStartIndices;
			std::ranges::sort(vSortStartIndices, std::less<>());//索引升序

			//预分配贪心筛选结果数组
			std::vector<size_t> vNewStartIndices;
			vNewStartIndices.reserve(vSortStartIndices.size());

			size_t szLastIndex = vSortStartIndices.front();//贪心起始选择开头
			vNewStartIndices.emplace_back(szLastIndex);//先行插入

			for (size_t i = 1; i < vSortStartIndices.size(); ++i)//从第二个开始比较
			{
				auto &szCurIndex = vSortStartIndices[i];

				if (szCurIndex - szLastIndex < it.szPrefixLength)//发生碰撞，差值小于长度
				{
					continue;//跳过并丢弃
				}
				
				//否则更新并插入
				szLastIndex = szCurIndex;
				vNewStartIndices.emplace_back(szLastIndex);
			}

			//检测剩余数量是否符合要求
			if (vNewStartIndices.size() < szMinRepeatCount)
			{
				continue;//不符合要求，去除
			}

			//否则插入
			listGreedyRepeatFragment.emplace_back(it.szPrefixLength, std::move(vNewStartIndices));
		}

		return listGreedyRepeatFragment;
	}

	template<typename SortFunc_T = decltype(DefaultGreedySort)>
	static void GreedySortFragments(SuffixArray::RepeatFragmentList &listRepeatFragment, SortFunc_T funcSort = DefaultGreedySort)
	{
		std::ranges::sort(listRepeatFragment, funcSort);
	}

	static SuffixArray::RepeatFragmentList GreedyNonOverlapAcrossFragments(const SuffixArray::RepeatFragmentList &listRepeatFragment, size_t szMinRepeatCount)
	{
		if (szMinRepeatCount < 2)//小于2个重复没有计算必要
		{
			return {};
		}

		GreedySectionOccupiedArray arrayGreedySectionOccupied{};//区间抢占排序数组
		GreedySectionOccupiedArray::SectionList listTempSection;//临时区间列表
		
		//在这里按照贪心筛选规则进行排序，按序遍历并依次贪心即可获取目标结果
		SuffixArray::RepeatFragmentList listGreedyRepeatFragment;//贪心结果
		for (const auto &it : listRepeatFragment)
		{
			listTempSection.clear();//清空

			//遍历并进行碰撞判断
			for (const auto &it2 : it.vStartIndices)
			{
				GreedySectionOccupiedArray::Section newSec{ .szBeg = it2, .szEnd = it2 + it.szPrefixLength };

				//碰撞，跳过
				if (arrayGreedySectionOccupied.IsSectionCollision(newSec))
				{
					continue;
				}

				listTempSection.push_back(std::move(newSec));//仅保留未碰撞
			}

			//现在已经筛选出未碰撞的所有成员，检测剩余成员数量是否符合要求
			if (listTempSection.size() < szMinRepeatCount)
			{
				continue;//跳过（删除整个家族）
			}

			//保留家族
			//先插入长度
			auto &vNewStartIndices = listGreedyRepeatFragment.emplace_back(it.szPrefixLength, SuffixArray::ValueList<size_t>{}).vStartIndices;
			vNewStartIndices.reserve(listTempSection.size());//提前扩容

			//依序插入返回列表和碰撞判断列表
			for (auto &it2 : listTempSection)
			{
				vNewStartIndices.emplace_back(it2.szBeg);//这里的big就是刚才初始化的it.vStartIndices元素
				arrayGreedySectionOccupied.InsertSection(std::move(it2));//移动转移所有权
			}
		}

		return listGreedyRepeatFragment;
	}
};


//自循环判断
class PeriodicityDetector
{
	PeriodicityDetector(void) = delete;
	~PeriodicityDetector(void) = delete;

public:
	/*
		输入字符串: "ababac"
		计算得到的 PM表 (π数组):
		索引:	0  1  2  3  4  5
		字符:	a  b  a  b  a  c
		PM表:	0  0  1  2  3  0
	*/
	template<typename T>
	static std::vector<size_t> ComputePartialMatch(const std::span<const T> &vInputArr)
	{
		size_t szInputSize = vInputArr.size();
		std::vector<size_t> vPartialMatch;
		vPartialMatch.reserve(szInputSize);

		vPartialMatch.push_back(0);//vPartialMatch[0] = 0;
		for (size_t szIndex = 1; szIndex < szInputSize; ++szIndex)
		{
			//获取当前的前缀长度
			size_t szPrefixLength = vPartialMatch[szIndex - 1];

			//只要还有候选前缀，且当前字符比对失败，就不断缩短前缀重试
			while (szPrefixLength > 0 && vInputArr[szIndex] != vInputArr[szPrefixLength])
			{
				//这里事实上相当于把当前匹配的长度-1作为前半段（前缀）的下标，
				//获取上一个匹配的序列中的前半部分，因为vPartialMatch有
				//递归自相似性，所以这个操作相当于把最后一个不匹配的字符
				//移动到前面匹配的位置的后一位进行匹配，并查找是否可以在
				//更短的前半模式串中得到当前字符可以组成的前后缀。
				szPrefixLength = vPartialMatch[szPrefixLength - 1];
			}

			//新的字符相等，那么增加前缀
			if (vInputArr[szIndex] == vInputArr[szPrefixLength])
			{
				++szPrefixLength;
			}

			//加入新的前缀长度
			vPartialMatch.push_back(szPrefixLength);//vPartialMatch[szIndex] = szPrefixLength;
		}

		return vPartialMatch;
	}

	//返回循环周期长度，如果为0，则串不循环
	template<typename T>
	static size_t CheckPeriodicity(const std::span<const T> &vInputArr, const std::vector<size_t> &vPartialMatch)
	{
		size_t szInputLength = vInputArr.size();
		if (szInputLength < 2)//单字符或空不算周期
		{
			return 0;
		} 

		//最长循环真前后缀子串长度
		size_t szOverlapLength = vPartialMatch.back();

		//周期长度
		size_t szPeriodLength = szInputLength - szOverlapLength;

		//如果周期至少有1，且剩余长度（周期偏移对齐举例）刚好能整除，则代表内部串为循环模式
		if (szOverlapLength > 0 && szInputLength % szPeriodLength == 0)
		{
			return szPeriodLength;//返回循环周期长度
		}

		//否则szPeriodLength不代表循环周期，返回0，串没有周期性
		return 0;
	}
};

