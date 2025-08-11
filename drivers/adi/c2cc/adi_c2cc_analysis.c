/*
 * Copyright (c) 2025, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <drivers/adi/adi_c2cc.h>

/**
 * searching through the combined data only, find the largest available
 * window -- as long as its center is less than or equal to the provided
 * max_trim_center.
 * @param[in] stats - the array of training statistics.
 * @param[in] max_trim_center - reject windows if their center is above
 *                              this value.
 * @param[out] offset - the trim position of the (start of the) window.
 * @return the size of the window, or 0 if no window was found.
 */
static size_t adi_c2cc_find_largest_combined_window(uint32_t *stats, size_t max_trim_center, size_t *offset)
{
	size_t s = 0;
	size_t o = 0;
	int pos = -1;
	size_t i = 0;

	/* find largest contiguous window of trim values with no errors */
	for (i = 0; i < max_trim_center; i++) {
		switch (pos) {
		case -1:
			if (!stats[i])
				pos = i;
			break;
		default:
			if (stats[i]) {
				size_t size = i - pos;
				VERBOSE("%s: C2CC found combined candidate (offset=%d,size=%lu)\n", __func__, pos, size);
				if (size > s) {
					o = pos;
					s = size;
				}
				pos = -1;
			}
			break;
		}
	}
	if (!stats[max_trim_center - 1] && pos >= 0) {
		i = max_trim_center;
		while (!stats[i] && i < ADI_C2C_TRIM_MAX)
			i++;
		size_t size = i - pos;
		if (pos + size / 2 <= max_trim_center) {
			VERBOSE("%s: C2CC found combined candidate (offset=%d,size=%lu)\n", __func__, pos, size);
			if (size > s) {
				o = pos;
				s = size;
			}
		}
	}

	if (offset)
		*offset = o;
	return s;
}

/**
 * given a lane and a position, where that position falls somewhere
 * within a lane window, find the start of that window by searching
 * backwards.
 * @param[in] stats - the array of training statistics.
 * @param[in] middle - the trim position which falls somewhere within a
 *                     lane window.
 * @param[in] lane - the lane number (0-indexed).
 * @return the start of the lane window.
 */
static size_t adi_c2cc_find_lane_window_start(uint32_t *stats, size_t middle, unsigned int lane)
{
	size_t i = 0;

	for (i = middle + 1; i > 0 && ADI_C2C_GET_LANE_STAT(stats, i - 1, lane) == 0; i--) {
	}
	return i;
}

/**
 * given a lane and a position, where that position falls somewhere
 * within a lane window, find the end of that window by searching
 * forward.
 * @param[in] stats - the array of training statistics.
 * @param[in] middle - the position, somewhere within the lane window.
 * @param[in] lane - the lane number (0-indexed).
 * @return the end of the lane window.
 */
static size_t adi_c2cc_find_lane_window_end(uint32_t *stats, size_t middle, unsigned int lane)
{
	size_t i = 0;

	for (i = middle; i < ADI_C2C_TRIM_MAX && ADI_C2C_GET_LANE_STAT(stats, i, lane) == 0; i++) {
	}
	return i;
}

/**
 * starting from the provided position and searching forward across all
 * lanes at once, return the first lane window that meets the provided
 * criteria.
 * @param[in] stats - the array of training statistics.
 * @param[in] start - the trim position to start searching from.
 * @param[in] max_dist - reject windows if their start is more than this
 *                       many trim values away from the start position.
 * @param[in] min_size - reject windows that are smaller than this size.
 * @param[out] lane - the lane in which the window was found.
 * @param[out] offset - the trim position of the (start of the) window.
 * @return the size of the window, or 0 if no window was found.
 */
static size_t adi_c2cc_find_next_window(uint32_t *stats, size_t start, size_t max_dist, size_t min_size, unsigned int *lane, size_t *offset)
{
	bool skip_lanes[ADI_C2C_LANE_COUNT] = { 0 };
	size_t end = 0;
	size_t i = 0;
	size_t j = 0;

	if (start != 0) {
		uint8_t lane_stats[ADI_C2C_LANE_COUNT] = ADI_C2C_GET_LANE_STATS(stats, start - 1);
		/* skip over any windows that started before 'start' */
		for (j = 0; j < ADI_C2C_LANE_COUNT; j++)
			if (!lane_stats[j])
				skip_lanes[j] = true;
	}

	for (i = start; i < start + max_dist; i++) {
		uint8_t lane_stats[ADI_C2C_LANE_COUNT] = ADI_C2C_GET_LANE_STATS(stats, i);
		for (j = 0; j < ADI_C2C_LANE_COUNT; j++) {
			if (skip_lanes[j]) {
				/* stop skipping if the window closed */
				if (lane_stats[j])
					skip_lanes[j] = false;
				continue;
			}
			if (lane_stats[j])
				continue;
			end = adi_c2cc_find_lane_window_end(stats, i, j);
			VERBOSE("%s: C2CC found candidate (offset=%lu,size=%lu)\n", __func__, i, end - i);
			if (end - i < min_size) {
				skip_lanes[j] = true;
				continue;
			}
			if (offset)
				*offset = i;
			if (lane)
				*lane = j;
			return end - i;
		}
	}

	return 0;
}

/**
 * starting from the provided position and searching forward within a
 * specific lane, return the first lane window that meets the provided
 * criteria.
 * @param[in] stats - the array of training statistics.
 * @param[in] start - the trim position to start searching from.
 * @param[in] max_dist - reject windows if their start is more than this
 *                       many trim values away from the start position.
 * @param[in] min_size - reject windows that are smaller than this size.
 * @param[out] offset - the trim position of the (start of the) window.
 * @return the size of the window, or 0 if no window was found.
 */
static size_t adi_c2cc_find_next_lane_window(uint32_t *stats, size_t start, unsigned int lane, size_t max_dist, size_t min_size, size_t *offset)
{
	size_t end = 0;
	size_t i = start;

	if (start != 0) {
		uint8_t stat = ADI_C2C_GET_LANE_STAT(stats, start - 1, lane);
		if (!stat) {
			end = adi_c2cc_find_lane_window_end(stats, start, lane);
			if (end - start > max_dist)
				return 0;
			max_dist -= (end - start);
			start = end;
			i = end;
		}
	}

	while (i < start + max_dist && i < ADI_C2C_TRIM_MAX) {
		uint8_t stat = ADI_C2C_GET_LANE_STAT(stats, i, lane);
		if (stat) {
			i += 1;
			continue;
		}
		end = adi_c2cc_find_lane_window_end(stats, i, lane);
		VERBOSE("%s: C2CC found window candidate (offset=%lu,size=%lu)\n", __func__, i, end - i);
		if (end - i < min_size) {
			i = end;
			continue;
		}
		if (offset)
			*offset = i;
		return end - i;
	}

	return 0;
}

/**
 * compute the score for a group of lane windows.
 * @param[in] effective_spread - the distance between the earliest and
 *                               latest lane windows.
 * @param[in] effective_size - the size of the smallest lane window.
 * @return the score for the group.
 */
static int adi_c2cc_score_window_group(size_t effective_spread, size_t effective_size)
{
	const unsigned int size_multiplier = 5;
	const unsigned int spread_multiplier = 1;
	size_t size_score = (effective_size - ADI_C2C_MIN_WINDOW_SIZE) * size_multiplier;
	size_t spread_score = effective_spread * spread_multiplier;

	return (int)size_score - (int)spread_score;
}

/**
 * starting from the provided position and searching forward across all
 * lanes at once, return the first available group of lane windows (one
 * in each lane) that meets the provided criteria.
 * @note - see adi_c2cc_score_window_group for details about how each
 *         group is scored.
 * @param[in] stats - the array of training statistics.
 * @param[in] start - the trim position to start searching from.
 * @param[in] max_spread - reject window groups if the distance between
 *                         the earliest and latest lane windows is
 *                         larger than this value.
 * @param[in] min_size - reject window groups if any individual lane
 *                       window is smaller than this size.
 * @param[in] max_center - reject window groups if the effective center
 *                         of the group (after alignment) would be
 *                         larger than this value.
 * @param[in] target_score - reject window groups unless their score
 *                           exceeds this target.
 * @param[out] offsets - the trim positions of (the start of) each lane
 *                       window in the group.
 * @param[out] sizes - the size of each lane window in the group.
 * @param[out] smallest_offset - the minimum of "offsets".
 * @param[out] largest_offset - the maximum of "offsets".
 * @param[out] smallest_size - the minimum of "sizes".
 * @param[out] group_score - the computed score for this group.
 * @return true if a suitable group was found, false otherwise.
 */
static bool adi_c2cc_find_window_group(uint32_t *stats, size_t start, size_t max_spread, size_t min_size, size_t max_center, int target_score, size_t *offsets, size_t *sizes, size_t *smallest_offset, size_t *largest_offset, size_t *smallest_size, int *group_score)
{
	size_t o = 0;
	size_t s = 0;
	unsigned int l = 0;
	unsigned int i = 0;
	size_t tmp_offsets[ADI_C2C_LANE_COUNT] = { 0 };
	size_t tmp_sizes[ADI_C2C_LANE_COUNT] = { 0 };

	while (start < ADI_C2C_TRIM_MAX) {
		int score = 0;
		size_t smallest = 0;
		size_t lowest = 0;
		size_t highest = 0;
		bool valid_candidate = true;

		s = adi_c2cc_find_next_window(stats, start, ADI_C2C_TRIM_MAX - start, min_size, &l, &o);
		if (!s) {
			VERBOSE("%s: C2CC no more windows in any lane (pos=%lu).\n", __func__, start);
			return false;
		}
		VERBOSE("%s: C2CC found leader window candidate (offset=%lu,size=%lu,lane=%u)\n", __func__, o, s, l);
		start = o;
		tmp_offsets[l] = o;
		tmp_sizes[l] = s;
		highest = o;
		lowest = o;
		smallest = s;

		for (i = 0; i < ADI_C2C_LANE_COUNT; i++) {
			if (i == l)
				continue;
			s = adi_c2cc_find_next_lane_window(stats, start, i, max_spread, min_size, &o);
			if (!s) {
				valid_candidate = false;
				break;
			}
			tmp_offsets[i] = o;
			tmp_sizes[i] = s;
			if (o > highest)
				highest = o;
			if (s < smallest)
				smallest = s;
		}

		if (!valid_candidate) {
			VERBOSE("%s: C2CC discarding window group\n", __func__);
			start += 1;
			continue;
		}

		if (highest + (smallest / 2) > max_center) {
			VERBOSE("%s: C2CC discarding window group (center=%lu,max=%lu)\n", __func__, highest + (smallest / 2), max_center);
			start += 1;
			continue;
		}

		score = adi_c2cc_score_window_group(highest - lowest, smallest);
		if (score <= target_score) {
			VERBOSE("%s: C2CC discarding window group (score=%d,best=%d)\n", __func__, score, target_score);
			start += 1;
			continue;
		}

		if (group_score)
			*group_score = score;
		if (largest_offset)
			*largest_offset = highest;
		if (smallest_offset)
			*smallest_offset = lowest;
		if (smallest_size)
			*smallest_size = smallest;
		if (offsets)
			for (i = 0; i < ADI_C2C_LANE_COUNT; i++)
				offsets[i] = tmp_offsets[i];
		if (sizes)
			for (i = 0; i < ADI_C2C_LANE_COUNT; i++)
				sizes[i] = tmp_sizes[i];

		return true;
	}

	return false;
}

/**
 * iterate through all possible groupings of lane windows that are
 * within range of each other, large enough, and centered below the
 * threshold. then select the "best" group based on its score.
 * @note - see adi_c2cc_score_window_group for details about how each
 *         group is scored.
 * @param[in] stats - the array of training statistics.
 * @param[in] max_spread - reject window groups if the distance between
 *                         the earliest and latest lane windows is
 *                         larger than this value.
 * @param[in] min_size - reject window groups if any individual lane
 *                       window is smaller than this size.
 * @param[in] max_center - reject window groups if the effective center
 *                         of the group (after alignment) would be
 *                         larger than this value.
 * @param[out] offsets - the trim positions of (the start of) each lane
 *                       window in the group.
 * @param[out] sizes - the size of each lane window in the group.
 * @param[out] largest_offset - the maximum of "offsets".
 * @param[out] smallest_size - the minimum of "sizes".
 * @return true if a suitable group was found, false otherwise.
 */
static bool adi_c2cc_find_best_window_group(uint32_t *stats, size_t max_spread, size_t min_size, size_t max_center, size_t *offsets, size_t *sizes, size_t *largest_offset, size_t *smallest_size)
{
	/* lowest possible score */
	int best_score = adi_c2cc_score_window_group(max_spread, 0);
	size_t current_offset = 0;

	if (!adi_c2cc_find_window_group(stats, 0, max_spread, min_size, max_center, best_score, offsets, sizes, &current_offset, largest_offset, smallest_size, &best_score))
		return false;

	VERBOSE("%s: C2CC found window group candidate (offsets=%lu,%lu,%lu,%lu,sizes=%lu,%lu,%lu,%lu,score=%d)\n",
		__func__, offsets[0], offsets[1], offsets[2], offsets[3], sizes[0], sizes[1], sizes[2], sizes[3], best_score);

	while (adi_c2cc_find_window_group(stats, current_offset + 1, max_spread, min_size, max_center, best_score, offsets, sizes, &current_offset, largest_offset, smallest_size, &best_score))
		VERBOSE("%s: C2CC found window group candidate (offsets=%lu,%lu,%lu,%lu,sizes=%lu,%lu,%lu,%lu,score=%d)\n",
			__func__, offsets[0], offsets[1], offsets[2], offsets[3], sizes[0], sizes[1], sizes[2], sizes[3], best_score);

	return true;
}

/**
 * for a given position within a combined window, find the four lane
 * windows which overlap at that position.
 * @param[in] stats - the array of training statistics.
 * @param[in] middle - the trim position which falls somewhere within a
 *                     combined window.
 * @param[out] offsets - the trim positions of (the start of) each lane
 *                       window in the group.
 */
static void adi_c2cc_overlapping_window_group(uint32_t *stats, size_t middle, size_t *offsets)
{
	size_t i = 0;

	for (i = 0; i < ADI_C2C_LANE_COUNT; i++)
		offsets[i] = adi_c2cc_find_lane_window_start(stats, middle, i);
}

/**
 * attempt to find the optimal trim value based on reviewing the training
 * statistics. ideally, this is done by finding the largest contiguous window
 * of trim values that reported no error, and selecting the middle of that
 * range.
 * however, in some cases, a certain lane may be consistently too slow or fast
 * which causes that lane's "no-error" window to sit at slightly higher or
 * lower trim values than the other lanes. if the windows for all four lanes
 * do not overlap enough, then we need to apply a delay to some lanes in order
 * to bring everything back into alignment.
 * If a suitable overlapping (combined) window is found, this function will
 * return the center of that window. If not, it will return ADI_C2C_TRIM_MAX.
 * In both cases, however, it will also set the trim delays. If a suitable
 * combined window was found, this represents the trim delays one could apply in
 * order to optimize the window further. Otherwise, if a suitable combined
 * window was not found, the delay values represent what must be applied in
 * order to achieve a suitable combined window in the first place.
 * If this function returns ADI_C2C_TRIM_MAX and also the trim_delays are all
 * zero, then it means there is no possible way to align the lanes to create a
 * suitable combined window (fatal error).
 * @note - trim delays should only be applied once.
 * @param[in] stats - the array of training statistics.
 * @param[in] min_size - minimum valid window size.
 * @param[in] max_spread - maximum allowed distance between the earliest
 *                         and latest lane windows.
 * @param[out] trim_delays - the chosen trim delay values for each lane.
 * @param[out] eye_width - the window size for the chosen trim value.
 * @return the chosen trim value, or ADI_C2C_TRIM_MAX if no suitable combined
 * window was found.
 */
uint8_t adi_c2cc_find_optimal_trim(uint32_t *stats, size_t min_size, size_t max_spread, size_t max_center, uint8_t *trim_delays, size_t *eye_width)
{
	size_t target_offset = 0;
	size_t target_size = 0;
	size_t offsets[ADI_C2C_LANE_COUNT] = { 0 };
	size_t sizes[ADI_C2C_LANE_COUNT] = { 0 };
	size_t i = 0;

	target_size = adi_c2cc_find_largest_combined_window(stats, max_center, &target_offset);
	if (target_size >= min_size) {
		adi_c2cc_overlapping_window_group(stats, target_offset, offsets);
		if (trim_delays)
			for (i = 0; i < ADI_C2C_LANE_COUNT; i++)
				trim_delays[i] = target_offset - offsets[i];
		if (eye_width)
			*eye_width = target_size;
		return target_offset + (target_size / 2);
	}
	WARN("%s: C2CC failed to find suitable trim window. Attempting exhaustive search to account for skew.\n", __func__);

	if (!adi_c2cc_find_best_window_group(stats, max_spread, min_size, max_center, offsets, sizes, &target_offset, &target_size)) {
		WARN("%s: C2CC failed to find any suitable window group during exhaustive search.\n", __func__);
		return ADI_C2C_TRIM_MAX;
	}

	VERBOSE("%s: C2CC selected window group (offsets=%lu,%lu,%lu,%lu,size=%lu)\n",
		__func__, offsets[0], offsets[1], offsets[2], offsets[3], target_size);

	if (trim_delays)
		for (i = 0; i < ADI_C2C_LANE_COUNT; i++)
			trim_delays[i] = target_offset - offsets[i];
	if (eye_width)
		*eye_width = target_size;

	return ADI_C2C_TRIM_MAX;
}
