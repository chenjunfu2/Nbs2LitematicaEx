#pragma once

#include "MyAlgorithm.hpp"

#include <set>
#include <stack>
#include <vector>
#include <cmath>
#include <string>
#include <algorithm>
#include <functional>

#include <stddef.h>
#include <stdint.h>

class RepetitionPatternFinder
{
	RepetitionPatternFinder(void) = delete;
	~RepetitionPatternFinder(void) = delete;

private:
	template<typename FilterFunc_T>
	static std::vector<size_t> buildFilterPrefix(const std::vector<size_t> &listInput, FilterFunc_T &funcFilter)
	{
		std::vector<size_t> listPref(listInput.size() + 1, 0);
		for (size_t i = 0; i < listInput.size(); ++i)
		{
			listPref[i + 1] = listPref[i] + (funcFilter(listInput, i) ? (size_t)0 : (size_t)1);
		}
		
		return listPref;
	}

	struct Candidate
	{
		size_t len;
		size_t valid_len;
		size_t occur_cnt;
		size_t rank_left;
		size_t rank_right;
		size_t start_pos;
		double score;
	};

	static std::vector<Candidate> enumerateCandidates(
		const std::vector<size_t> &input,
		const std::vector<size_t> &sa,
		const std::vector<size_t> &lcp,
		const std::vector<size_t> &filter_pref,
		size_t min_occurrences,
		size_t min_valid_len)
	{

		size_t m = lcp.size();
		std::vector<size_t> left(m), right(m);
		std::stack<size_t> st;

		// 计算左边界
		for (size_t i = 0; i < m; ++i)
		{
			while (!st.empty() && lcp[st.top()] >= lcp[i]) st.pop();
			left[i] = st.empty() ? 0 : st.top() + 1;
			st.push(i);
		}

		// 清空栈
		while (!st.empty()) st.pop();

		// 计算右边界
		for (size_t i = m; i-- > 0; )
		{
			while (!st.empty() && lcp[st.top()] > lcp[i]) st.pop();
			right[i] = st.empty() ? m - 1 : st.top() - 1;
			st.push(i);
		}

		std::vector<Candidate> candidates;
		for (size_t i = 0; i < m; ++i)
		{
			size_t L = left[i], R = right[i];
			size_t cnt = R - L + 2;
			if (cnt < min_occurrences) continue;

			size_t len = lcp[i];
			size_t start = sa[L];
			size_t valid_len = filter_pref[start + len] - filter_pref[start];
			if (valid_len < min_valid_len) continue;

			candidates.push_back({ len, valid_len, cnt, L, R, start, 0.0 });
		}

		return candidates;
	}

	static std::vector<size_t> getOccurrencePositions(
		const std::vector<size_t> &sa,
		size_t rank_left,
		size_t rank_right)
	{

		std::vector<size_t> poses;
		for (size_t rk = rank_left; rk <= rank_right; ++rk)
		{
			poses.push_back(sa[rk]);
		}
		std::sort(poses.begin(), poses.end());
		return poses;
	}

	static bool isOverlap(
		size_t l, size_t r,
		const std::set<std::pair<size_t, size_t>> &selected)
	{

		auto it = selected.lower_bound({ l, 0 });
		if (it != selected.end() && it->first <= r) return true;
		if (it != selected.begin())
		{
			--it;
			if (it->second >= l) return true;
		}
		return false;
	}

public:
	static void computeScores(std::vector<Candidate> &candidates, double alpha)
	{
		for (auto &c : candidates)
		{
			c.score = c.occur_cnt * std::pow(c.valid_len, alpha);
		}
	}

	struct PatternResult
	{
		std::vector<size_t> content;
		std::vector<size_t> occurrences;
		size_t start_pos;
		size_t end_pos;
		size_t length;
		size_t valid_length;
		double score;
	};

	struct FinderConfig
	{
		size_t min_occurrences = 2;      // 至少重复次数
		size_t min_valid_length = 2;     // 最小有效长度（不含数字）
		double length_importance = 1.5;  // 长度重要性指数（alpha）
	};

	/**
	 * 从字符串中查找所有不重叠的重复模式
	 * @param str 输入字符串
	 * @param config 查找配置
	 * @return 所有找到的模式（已按优先级排序，且不重叠）
	 */
	template<bool bFindAll = false, typename T, typename ScoreFunc_T, typename FilterFunc_T>
	requires(std::is_integral_v<T> &&std::is_unsigned_v<T> && sizeof(T) <= sizeof(size_t))
	static std::vector<PatternResult> findRepeatingPatterns(
		const SuffixArray::ValueList<T> &input,
		const std::vector<size_t> sa,
		const std::vector<size_t> lcp,
		ScoreFunc_T funcScore,
		FilterFunc_T funcFilter,
		const FinderConfig &config = FinderConfig())
	{
		std::vector<PatternResult> results;
		if (input.empty())
		{
			return results;
		}

		// 构建相关数据结构
		std::vector<size_t> filter_pref = buildFilterPrefix(input, funcFilter);

		// 枚举候选模式
		auto candidates = enumerateCandidates(
			input, sa, lcp, filter_pref,
			config.min_occurrences, config.min_valid_length);

		if (candidates.empty())
		{
			return results;
		}

		// 计算评分
		funcScore(candidates, config.length_importance);

		// 按评分降序排序
		std::sort(candidates.begin(), candidates.end(),
			[](const Candidate &a, const Candidate &b)
			{
				return a.score > b.score;
			}
		);

		// 贪心选取不重叠区间
		std::set<std::pair<size_t, size_t>> selected;

		for (const auto &cand : candidates)
		{
			auto poses = getOccurrencePositions(sa, cand.rank_left, cand.rank_right);
			if constexpr (bFindAll == false)
			{
				for (size_t pos : poses)
				{
					size_t l = pos, r = pos + cand.len - 1;
					if (!isOverlap(l, r, selected))
					{
						selected.insert({ l, r });
						results.emplace_back
						(
							PatternResult
							{
								.content = std::vector<size_t>(input.begin() + cand.start_pos, input.begin() + cand.start_pos + cand.len),
								.occurrences = poses,
								.start_pos = l,
								.end_pos = r,
								.length = cand.len,
								.valid_length = cand.valid_len,
								.score = cand.score,
							}
						);

						break; // 每个模式只选取第一个不重叠的出现
					}
				}
			}
			else//bFindAll == true
			{
				results.emplace_back
				(
					PatternResult
					{
						.content = std::vector<size_t>(input.begin() + cand.start_pos, input.begin() + cand.start_pos + cand.len),
						.occurrences = poses,
						.start_pos = cand.start_pos,
						.end_pos = cand.start_pos + cand.len - 1,
						.length = cand.len,
						.valid_length = cand.valid_len,
						.score = cand.score,
					}
				);
			}
		}

		return results;
	}
};
