/*
 * Copyright (c) 2025, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <drivers/adi/adi_c2cc.h>

static size_t adi_c2cc_find_largest_combined_window(uint32_t *stats, size_t *offset)
{
	size_t s = 0;
	size_t o = 0;
	int pos = -1;
	size_t i = 0;

	/* find largest contiguous window of trim values with no errors */
	for (i = 0; i < ADI_C2C_TRIM_MAX; i++) {
		switch (pos) {
		case -1:
			if (!stats[i])
				pos = i;
			break;
		default:
			if (stats[i]) {
				size_t size = i - pos;
				VERBOSE("%s: C2CC found candidate (offset=%d,size=%lu)\n", __func__, pos, size);
				if (size > s) {
					o = pos;
					s = size;
				}
				pos = -1;
			}
			break;
		}
	}
	if (!stats[ADI_C2C_TRIM_MAX - 1] && pos >= 0) {
		size_t size = ADI_C2C_TRIM_MAX - pos;
		VERBOSE("%s: C2CC found candidate (offset=%d,size=%lu)\n", __func__, pos, size);
		if (size > s) {
			o = pos;
			s = size;
		}
	}

	if (offset)
		*offset = o;
	return s;
}

static size_t adi_c2cc_find_lane_window_end(uint32_t *stats, size_t start, unsigned int lane)
{
	size_t i = 0;

	for (i = start; i < ADI_C2C_TRIM_MAX && ADI_C2C_GET_LANE_STAT(stats, i, lane) == 0; i++) {
	}
	return i;
}

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

	while (i < start + max_dist) {
		uint8_t stat = ADI_C2C_GET_LANE_STAT(stats, i, lane);
		if (stat) {
			i += 1;
			continue;
		}
		end = adi_c2cc_find_lane_window_end(stats, i, lane);
		VERBOSE("%s: C2CC found candidate (offset=%lu,size=%lu)\n", __func__, i, end - i);
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

static bool adi_c2cc_find_window_group(uint32_t *stats, size_t max_spread, size_t min_size, size_t *offsets, size_t *sizes, size_t *largest_offset, size_t *smallest_size)
{
	size_t start = 0;
	size_t o = 0;
	size_t s = 0;
	unsigned int l = 0;
	unsigned int i = 0;

	while (start < ADI_C2C_TRIM_DELAY_MAX) {
		bool valid_candidate = true;

		if (!offsets || !sizes)
			return false;
		s = adi_c2cc_find_next_window(stats, start, ADI_C2C_TRIM_MAX - start, min_size, &l, &o);
		if (!s)
			return false;
		VERBOSE("%s: C2CC found candidate (offset=%lu,size=%lu,lane=%u)\n", __func__, o, s, l);
		start = o;
		offsets[l] = o;
		sizes[l] = s;
		if (largest_offset)
			*largest_offset = o;
		if (smallest_size)
			*smallest_size = s;

		for (i = 0; i < ADI_C2C_LANE_COUNT; i++) {
			if (i == l)
				continue;
			s = adi_c2cc_find_next_lane_window(stats, start, i, max_spread, min_size, &o);
			if (!s) {
				valid_candidate = false;
				break;
			}
			offsets[i] = o;
			sizes[i] = s;
			if (largest_offset && o > *largest_offset)
				*largest_offset = o;
			if (smallest_size && s < *smallest_size)
				*smallest_size = s;
		}

		if (!valid_candidate) {
			start += 1;
			continue;
		}

		return true;
	}

	return false;
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
 * @param[in] stats - the array of training statistics.
 * @param[in] min_size - minimum valid window size
 * @param[out] trim_delays - the chosen trim delay values (this is an array of
 *                           size ADI_C2C_LANE_COUNT).
 * @param[out] eye_width - the window size for the chosen trim value
 * @return the chosen trim value, or ADI_C2C_TRIM_MAX if no window was found.
 */
uint8_t adi_c2cc_find_optimal_trim(uint32_t *stats, size_t min_size, uint8_t *trim_delays, size_t *eye_width)
{
	size_t target_offset = 0;
	size_t target_size = 0;
	size_t offsets[ADI_C2C_LANE_COUNT] = { 0 };
	size_t sizes[ADI_C2C_LANE_COUNT] = { 0 };
	size_t i = 0;

	target_size = adi_c2cc_find_largest_combined_window(stats, &target_offset);
	if (target_size >= min_size) {
		if (trim_delays)
			for (i = 0; i < ADI_C2C_LANE_COUNT; i++)
				trim_delays[i] = 0;
		if (eye_width)
			*eye_width = target_size;
		return target_offset + (target_size / 2);
	}
	WARN("%s: C2CC failed to find suitable trim window. Re-attempting with delay to account for skew.\n", __func__);

	if (!adi_c2cc_find_window_group(stats, ADI_C2C_TRIM_DELAY_MAX, min_size, offsets, sizes, &target_offset, &target_size)) {
		WARN("%s: C2CC failed to find suitable trim window (with delay).\n", __func__);
		return ADI_C2C_TRIM_MAX;
	}

	if (trim_delays)
		for (i = 0; i < ADI_C2C_LANE_COUNT; i++)
			trim_delays[i] = target_offset - offsets[i];
	if (eye_width)
		*eye_width = target_size;

	return target_offset + (target_size / 2);
}
