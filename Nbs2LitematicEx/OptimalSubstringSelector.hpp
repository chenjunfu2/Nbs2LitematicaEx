#pragma once

#include "MyAlgorithm.hpp"

#include <vector>
#include <string>
#include <functional>
#include <optional>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <unordered_map>
#include <unordered_set>

/**
 * @brief 最优子串选择器：基于动态规划解决带约束的非重叠子串选择问题
 *
 * 核心流程：
 * 1. 候选挖掘：利用 SA+Height 找出所有 长度>k 且 频次>=n 的模式
 * 2. 规则应用：对每个模式的出现位置进行"非数字结尾"截断，生成合法区间
 * 3. 区间调度：使用 DP 求解加权非重叠区间集合的最大权重和
 */
template<typename T> // T: 字符类型，需满足 unsigned integral
requires(std::is_integral_v<T> &&std::is_unsigned_v<T> && sizeof(T) <= sizeof(size_t))
class OptimalSubstringSelector
{
public:
	// ================= 配置与数据结构 =================

	/// @brief 选择器配置参数
	struct SelectorConfig
	{
		size_t szMinLength = 0;          // k: 子串最小长度约束 (> k)
		size_t szMinFrequency = 0;       // n: 子串最小出现频次约束 (>= n)

		/// @brief 权重计算函数：输入(长度, 频次)，输出该子串的得分
		/// 示例：[](size_t len, size_t freq) { return static_cast<double>(len) * std::log1p(freq); }
		std::function<double(size_t, size_t)> fnWeightCalculator =
		[](size_t len, size_t freq) -> double
		{
			return static_cast<double>(len);
		}; // 默认仅按长度加权

		/// @brief 判断字符是否为"非法结尾"的谓词 (默认: 数字字符不能结尾)
		std::function<bool(T)> fnIsInvalidEndingChar =
		[](T ch) -> bool
		{
			return ch < 10;
		};

		/// @brief 判断字符是否为"非法开头" (默认同结尾规则)
		std::function<bool(T)> fnIsInvalidStartingChar =
		[](T ch) -> bool
		{
			return ch < 10;
		};
	};

	/// @brief 合法区间结构体：代表一个经过截断处理、可被选中的子串
	struct ValidInterval
	{
		size_t szStart;          // 区间起始位置 [0, N-1]
		size_t szEnd;            // 区间结束位置 [0, N-1] (包含)
		size_t szOriginalLength; // 截断前的原始模式长度
		size_t szFrequency;      // 该模式的全局出现频次
		double dWeight;          // 计算后的权重得分

		// 辅助比较：用于调试或排序
		bool operator<(const ValidInterval &other) const
		{
			return szEnd < other.szEnd || (szEnd == other.szEnd && szStart < other.szStart);
		}
	};

	/// @brief DP 求解结果
	struct SelectionResult
	{
		double dTotalWeight;                 // 选中集合的总权重
		std::vector<ValidInterval> vIntervals; // 选中的区间列表 (按起始位置排序)
	};

private:
	// ================= 内部辅助结构 =================

	/// @brief 原始候选模式信息 (截断前)
	struct RawPattern
	{
		size_t szLength;          // 模式长度
		size_t szFrequency;       // 全局频次
		std::vector<size_t> vEndPositions; // 所有出现的**结束位置** (inclusive)
	};

public:
	// ================= 核心接口 =================

	/**
	 * @brief 执行最优子串选择
	 * @param vInputArr 输入字符串数组 (数字/字母)
	 * @param config 配置参数 (k, n, 权重函数等)
	 * @return 最优选择结果
	 */
	static SelectionResult Select(const std::vector<T> &vInputArr, const SelectorConfig &config)
	{
		SelectionResult result;
		if (vInputArr.empty() || config.szMinLength >= vInputArr.size())
		{
			return result;
		}

		const size_t N = vInputArr.size();

		// Step 1: 候选模式挖掘 (利用 SA + Height)
		std::vector<RawPattern> vRawPatterns = MineCandidatePatterns(vInputArr, config);

		// Step 2: 应用截断规则，生成合法区间并按结束位置分组
		// events[endPos] 存储所有以 endPos 结尾的合法区间
		std::vector<std::vector<ValidInterval>> events(N);
		GenerateValidIntervals(vInputArr, vRawPatterns, config, events);

		// Step 3: 区间动态规划求解
		return SolveIntervalDP(N, events);
	}

private:
	// ================= 算法实现细节 =================

	/**
	 * @brief [Step 1] 利用 SuffixArray + Height 挖掘候选模式
	 *
	 * 原理：
	 * 在 SA 中，若一段连续区间 [L, R] 的 height 值均 >= len，
	 * 则该区间内所有后缀共享一个长度为 len 的前缀。
	 * 若区间长度 (R-L+1) >= minFreq，则该前缀是一个候选模式。
	 */
	static std::vector<RawPattern> MineCandidatePatterns(
		const std::vector<T> &vInputArr,
		const SelectorConfig &config)
	{
		std::vector<RawPattern> patterns;
		const size_t N = vInputArr.size();
		const size_t k = config.szMinLength;
		const size_t n = config.szMinFrequency;

		// 1. 构建 SA 和 Height
		// 注意：需根据实际字符集大小调整 valueRange，数字+字母通常 < 128
		size_t valueRange = 256;
		auto saPair = SuffixArray::DoublingCountingRadixSortSuffixArray(valueRange, vInputArr);
		auto heightArr = SuffixArray::LcpHeightArray(vInputArr, saPair);

		const auto &sa = saPair.vSuffixArray; // sa[rank] = startPos
		const auto &rankArr = saPair.vRank;   // rank[startPos] = rank

		// 2. 枚举所有可能的模式长度 (从 k+1 到 N)
		// 优化：实际工程中可只枚举在 height 数组中出现过的长度值
		for (size_t len = k + 1; len <= N; ++len)
		{
			// 在 SA 上滑动窗口，寻找 height >= len 的连续段
			size_t L = 0;
			while (L < N)
			{
				// 寻找满足条件的右边界 R
				size_t R = L;
				while (R + 1 < N && heightArr[R + 1] >= len)
				{
					++R;
				}

				// 如果段长度 [L, R] 满足频次要求
				if ((R - L + 1) >= n)
				{
					// 提取该模式的所有出现位置
					// 注意：长度为 len 的模式可能包含在更长的模式中，这里需要去重或特殊处理
					// 为简化，我们记录该 (len, [L,R]) 组合
					RawPattern pat;
					pat.szLength = len;
					pat.szFrequency = R - L + 1;
					pat.vEndPositions.reserve(pat.szFrequency);

					for (size_t i = L; i <= R; ++i)
					{
						size_t startPos = sa[i];
						size_t endPos = startPos + len - 1;
						if (endPos < N) pat.vEndPositions.push_back(endPos);
					}

					// 简单去重：如果该模式完全被之前找到的更长模式覆盖且位置相同，可跳过
					// (此处省略复杂去重逻辑，保留所有候选，由后续截断和DP处理)
					if (!pat.vEndPositions.empty())
					{
						patterns.push_back(std::move(pat));
					}
				}
				L = R + 1;
			}
		}
		return patterns;
	}

	/**
	 * @brief [Step 2] 应用截断规则，生成合法区间
	 *
	 * 规则：
	 * 1. 若子串以非法字符(如数字)结尾，则向左截断至最近的合法字符。
	 * 2. 若子串以非法字符(如数字)开头，则向右截断至最近的合法字符。
	 * 3. 截断后长度 < k，则丢弃该区间。
	 */
	static void GenerateValidIntervals(
		const std::vector<T> &vInputArr,
		const std::vector<RawPattern> &vRawPatterns,
		const SelectorConfig &config,
		std::vector<std::vector<ValidInterval>> &events) // output: events[endPos] = list of intervals
	{
		const size_t N = vInputArr.size();

		for (const auto &pat : vRawPatterns)
		{
			for (size_t originalEnd : pat.vEndPositions)
			{
				// 0. 计算原始起始位置
				size_t originalStart = originalEnd - pat.szLength + 1;

				// 1. 应用结尾截断规则：向左寻找最近的合法结尾字符
				size_t validEnd = originalEnd;
				while (validEnd >= originalStart && // 确保不越过原始起始位置
					config.fnIsInvalidEndingChar(vInputArr[validEnd]))
				{
					--validEnd;
				}

				// 2. 应用开头截断规则：向右寻找最近的合法开头字符
				// 注意：必须在已截断的 validEnd 范围内搜索，避免区间交叉
				size_t validStart = originalStart;
				while (validStart <= validEnd && // 确保不越过已截断的结尾
					config.fnIsInvalidStartingChar(vInputArr[validStart]))
				{
					++validStart;
				}

				// 3. 校验截断后的合法性
				// 情况1: 双向截断后区间为空 (起始位置越过结束位置)
				// 情况2: 截断后实际长度不足最小要求
				size_t actualLength = (validStart <= validEnd) ? (validEnd - validStart + 1) : 0;
				if (validStart > validEnd || actualLength < config.szMinLength)
				{
					continue;
				}

				// 4. 计算权重并生成区间
				ValidInterval interval;
				interval.szStart = validStart;          // 使用截断后的起始位置
				interval.szEnd = validEnd;              // 使用截断后的结束位置
				interval.szOriginalLength = pat.szLength;
				interval.szFrequency = pat.szFrequency;
				// 权重基于截断后的实际长度计算，反映真实价值
				interval.dWeight = config.fnWeightCalculator(actualLength, pat.szFrequency);

				// 5. 按结束位置分组存入 events
				if (interval.szEnd < N)
				{
					events[interval.szEnd].push_back(interval);
				}
			}
		}

		// 优化：对每个位置的区间按权重降序排序，便于后续剪枝（可选）
		// 当同一结束位置有多个候选区间时，优先尝试权重高的，可提前剪枝
		for (auto &list : events)
		{
			if (list.size() > 1)
			{
				std::sort(list.begin(), list.end(),
					[](const ValidInterval &a, const ValidInterval &b)
					{
						return a.dWeight > b.dWeight;
					});
			}
		}
	}

	/**
	 * @brief [Step 3] 区间动态规划求解 (加权区间调度)
	 *
	 * 状态定义：dp[i] = 考虑前缀 [0, i-1] 能获得的最大权重
	 * 转移方程：dp[i] = max( dp[i-1], max_{interval in events[i-1]} { dp[interval.start] + interval.weight } )
	 */
	static SelectionResult SolveIntervalDP(
		size_t N,
		const std::vector<std::vector<ValidInterval>> &events)
	{
		SelectionResult result;

		// DP 数组
		std::vector<double> dp(N + 1, 0.0);
		// 回溯辅助：prevIdx[i] 记录 dp[i] 是从哪个下标转移来的
		std::vector<size_t> prevIdx(N + 1, 0);
		// 回溯辅助：chosen[i] 记录在位置 i-1 结尾是否选中了区间，以及是哪个
		std::vector<std::optional<size_t>> chosenEventIdx(N + 1, std::nullopt);

		for (size_t i = 1; i <= N; ++i)
		{
			size_t currentPos = i - 1; // 当前处理的原字符串下标

			// 决策 1: 不选以 currentPos 结尾的任何区间
			dp[i] = dp[i - 1];
			prevIdx[i] = i - 1;
			// chosenEventIdx[i] 保持 nullopt

			// 决策 2: 枚举所有以 currentPos 结尾的合法区间
			for (size_t evIdx = 0; evIdx < events[currentPos].size(); ++evIdx)
			{
				const auto &interval = events[currentPos][evIdx];

				// 状态转移：接在 interval.start 之后
				// 注意：区间是 [start, end]，所以接在 start 位置的最优解之后
				// dp 定义是前缀 [0, start-1]，所以用 dp[interval.szStart]
				// 修正：如果区间是 [start, end]，那么它占用了 start...end。
				// 前面的子问题应该是 [0, start-1]，即 dp[start]。

				double candidateWeight = dp[interval.szStart] + interval.dWeight;

				if (candidateWeight > dp[i])
				{
					dp[i] = candidateWeight;
					prevIdx[i] = interval.szStart; // 记录前驱状态
					chosenEventIdx[i] = evIdx;     // 记录选中的区间索引
				}
			}
		}

		// 填充结果总权重
		result.dTotalWeight = dp[N];

		// 回溯还原选中的区间
		size_t cursor = N;
		while (cursor > 0)
		{
			if (chosenEventIdx[cursor].has_value())
			{
				size_t evIdx = chosenEventIdx[cursor].value();
				const auto &interval = events[cursor - 1][evIdx];
				result.vIntervals.push_back(interval);

				// 跳转到该区间开始之前的位置
				cursor = interval.szStart;
			}
			else
			{
				// 该位置未选中区间，向前移动
				--cursor;
			}
		}

		// 回溯得到的是逆序，反转回来
		std::reverse(result.vIntervals.begin(), result.vIntervals.end());

		return result;
	}
};