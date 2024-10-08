#pragma once

#include <cinttypes>
#include <ctime>
#include <sstream>
#include <vector>
#include <unistd.h>

#include "json11.hpp"

#include "utils.hh"
#include "aligned_maccess.hh"
#include "mapping.hh"

using json11::Json;
using std::vector;

class StrideExperiment {
public:
	// stride width in bytes
	ssize_t const stride;
	// number of accesses to perform
	size_t const step;
	// offset of the first access from mapping.base_addr in bytes
	size_t const first_access_offset;
	// wait before probing or not
	bool const use_nanosleep;
	// Flush+Reload threshold
	size_t const fr_thresh;
	// Flush+Reload noise threshold
	size_t const noise_thresh;
	// structs for nanosleep
	struct timespec const t_req;
	struct timespec t_rem;

	StrideExperiment(ssize_t stride, size_t step, size_t first_access_offset, bool use_nanosleep, size_t fr_thresh, size_t noise_thresh)
	: stride {stride}
	, step {step}
	, first_access_offset {first_access_offset}
	, use_nanosleep {use_nanosleep}
	, fr_thresh {fr_thresh}
	, noise_thresh {noise_thresh}
	, t_req { .tv_sec = 0, .tv_nsec = 1500 /* 1µs */ }
	{}

	bool offset_accessed(size_t offset) const;
	bool offset_accessed(size_t offset, ssize_t delta, size_t gap1, size_t gap2) const;
	bool offset_potential_prefetch(size_t offset) const;
	bool offset_potential_prefetch(size_t offset, ssize_t delta, size_t gap1, size_t gap2) const;
	bool cl_accessed(size_t cl_idx) const;
	bool cl_accessed(size_t cl_idx, ssize_t delta, size_t gap1, size_t gap2) const;
	bool cl_potential_prefetch(size_t cl_idx) const;
	bool cl_potential_prefetch(size_t cl_idx, ssize_t delta, size_t gap1, size_t gap2) const;

	// Helper functions to get first and (last+1*stride) address to access.

	inline uint8_t* get_ptr_begin(Mapping const& mapping) const {
		return mapping.base_addr + first_access_offset;
	}
	inline uint8_t* get_ptr_end(Mapping const& mapping) const {
		return get_ptr_begin(mapping) + stride * step;
	}
	inline uint8_t* get_ptr_end(Mapping const& mapping, ssize_t delta, size_t gap1, size_t gap2) const {
		return get_ptr_begin(mapping) + (int)(step * (stride * gap1 + (stride + delta) * gap2));
	}
	inline size_t offset_last_access() const {
		size_t offset = first_access_offset + stride * (step - 1);
		assert(offset >= 0);
		return offset;
	}

private:
	inline void probe_single(vector<size_t>& cache_histogram, size_t idx, uint8_t* ptr) const __attribute__((always_inline)) {
		assert(idx < cache_histogram.size());
		size_t time = flush_reload_t(ptr);
		cache_histogram[idx] += (time < fr_thresh) ? 1 : 0;
	}

public:
	vector<size_t> collect_cache_histogram(Mapping const& mapping, size_t no_repetitions, void (*workload)(StrideExperiment const&, Mapping const&, void*), void* additional_info);
	vector<size_t> collect_cache_histogram(Mapping const& mapping, size_t no_repetitions, ssize_t delta, size_t gap1,  size_t gap2, 
														void (*workload)(StrideExperiment const&, Mapping const&, ssize_t, size_t, size_t, void*),
														void* additional_info);
	vector<size_t> collect_cache_histogram(Mapping const& mapping, size_t no_repetitions, ssize_t delta, size_t gap1,  size_t gap2, size_t plus,
														void (*workload)(StrideExperiment const&, Mapping const&, ssize_t, size_t, size_t, size_t, void*),
														void* additional_info);
	vector<size_t> collect_cache_histogram(Mapping const& mapping, size_t no_repetitions, ssize_t delta, size_t gap1,  size_t gap2, 
														void (*workload)(StrideExperiment const&, Mapping const&, ssize_t, size_t, size_t, ssize_t, size_t, void*),
														void* additional_info);
	vector<size_t> collect_cache_histogram(Mapping const& mapping1, Mapping const& mapping2, size_t no_repetitions, void (*workload)(StrideExperiment const&, Mapping const&, Mapping const&, void*), void* additional_info);
	vector<size_t> collect_cache_histogram_lazy(Mapping const& mapping, size_t no_repetitions, void (*workload)(StrideExperiment const&, Mapping const&, void*), void* additional_info);

	vector<bool> evaluate_cache_histogram(vector<size_t> const& cache_histogram, size_t no_repetitions, double threshold_multiplier) const;
	vector<bool> evaluate_cache_histogram(vector<size_t> const& cache_histogram, size_t no_repetitions, double threshold_multiplier, 
														ssize_t delta, size_t gap1, size_t gap2) const;
	vector<bool> evaluate_cache_histogram(vector<size_t> const& cache_histogram, size_t no_repetitions) const;
	vector<bool> evaluate_cache_histogram(vector<size_t> const& cache_histogram, size_t no_reprtitions, ssize_t delta, size_t gap1, size_t gap2) const;

	void dump(vector<size_t> const& cache_histogram, vector<bool> prefetch_vector, string const& filepath) const;
	static pair<StrideExperiment, vector<size_t>> restore(string const& filepath);
};

// ===== WORKLOADS =====
__attribute__((always_inline)) static inline void access_fence(void *p) {
    asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax");
    asm volatile("mfence");
}
__attribute__((always_inline)) static inline void access_fence_2(uint8_t* ptr) {
    access_fence(ptr);
    access_fence(ptr + 64);
}
__attribute__((always_inline)) static inline void access_fence_4(uint8_t* ptr) {
    access_fence_2(ptr);
    access_fence_2(ptr + 2 * 64);
}
__attribute__((always_inline)) static inline void access_fence_8(uint8_t* ptr) {
    access_fence_4(ptr);
    access_fence_4(ptr + 4 * 64);
}
__attribute__((always_inline)) static inline void access_fence_16(uint8_t* ptr) {
    access_fence_8(ptr);
    access_fence_8(ptr + 8 * 64);
}
__attribute__((always_inline)) static inline void access_fence_32(uint8_t* ptr) {
    access_fence_16(ptr);
    access_fence_16(ptr + 16 * 64);
}
__attribute__((always_inline)) static inline void access_fence_64(uint8_t* ptr) {
    access_fence_32(ptr);
    access_fence_32(ptr + 32 * 64);
}
__attribute__((always_inline)) static inline void access_fence_128(uint8_t* ptr) {
    access_fence_64(ptr);
    access_fence_64(ptr + 64 * 64);
}
__attribute__((always_inline)) static inline void access_fence_256(uint8_t* ptr) {
    access_fence_128(ptr);
    access_fence_128(ptr + 128 * 64);
}


/**
 * Standard workload for most tests. A loop with a single load in the loop
 * body. Supports positive and negative strides.
 *
 * @param      experiment       The experiment
 * @param      mapping          The mapping
 * @param      additional_info  The additional information (must be nullptr)
 */
__attribute__((always_inline)) inline void workload_stride_loop(StrideExperiment const& experiment, Mapping const& mapping, void* additional_info) {
	assert(additional_info == nullptr);

	uint8_t* ptr_begin = experiment.get_ptr_begin(mapping);
	uint8_t* ptr_end = experiment.get_ptr_end(mapping);
	
	for (
		uint8_t* ptr = ptr_begin;
		(ptr_end > ptr_begin) ? (ptr < ptr_end) : (ptr > ptr_end);
		ptr += experiment.stride
	) {
		//asm volatile("movq (%0), %%rax\n" : : "c"(ptr) : "rax");
		maccess(ptr);
        mfence();
        //printf("%p\n", ptr);
	}
}

/**
 * 2D stride workload 
 */
__attribute__((always_inline)) inline void workload_2Dstride_loop(StrideExperiment const& experiment, Mapping const& mapping, 
																ssize_t	delta, size_t gap1, size_t gap2, void* additional_info)
{
	uint8_t* ptr_begin = experiment.get_ptr_begin(mapping);
	uint8_t* ptr_end = experiment.get_ptr_end(mapping, delta, gap1, gap2);
	//printf("begin=%p, end=%p\n", ptr_begin, ptr_end);
	uint8_t* ptr = ptr_begin;
	maccess_noinline(ptr);
	while((ptr_end > ptr_begin) ? (ptr < ptr_end) : (ptr > ptr_end)) {
		for (size_t i = 0; i < (gap1 + gap2); ++i) {			
			if (i < gap1) {
				ptr += experiment.stride;
			} else {
				ptr += (experiment.stride + delta);
			}
			maccess_noinline(ptr);
        	mfence();
			//printf("i=%ld, %p\n", i, ptr);
		}	
	}
}

__attribute__((always_inline)) inline void workload_2Dstride_loop_plus(StrideExperiment const& experiment, Mapping const& mapping, 
																ssize_t	delta, size_t gap1, size_t gap2, size_t plus, void* additional_info)
{
	uint8_t* ptr_begin = experiment.get_ptr_begin(mapping);
	uint8_t* ptr_end = experiment.get_ptr_end(mapping, delta, gap1, gap2);

	uint8_t* ptr = ptr_begin;
	maccess_noinline(ptr);
	while((ptr_end > ptr_begin) ? (ptr < ptr_end) : (ptr > ptr_end)) {
		for (size_t i = 0; i < (gap1 + gap2); ++i) {			
			if (i < gap1) {
				ptr += experiment.stride;
			} else {
				ptr += (experiment.stride + delta);
			}
			maccess_noinline(ptr);
        	mfence();
			//printf("i=%ld, %p\n", i, ptr);
		}	
	}
	for (size_t j = 0; j < plus; ++j) {
		ptr += experiment.stride;
		maccess_noinline(ptr);
		mfence();
	}
}


__attribute__((always_inline)) inline void workload_3Dstride_loop(StrideExperiment const& experiment, Mapping const& mapping, 
																ssize_t	delta, size_t gap1, size_t gap2, ssize_t stride3, size_t gap3, void* additional_info)
{
	uint8_t* ptr =  experiment.get_ptr_begin(mapping);
	maccess_noinline(ptr);
	mfence();
	for (size_t i = 0; i < experiment.step; ++i) {
		for (size_t j = 0; j < (gap1 + gap2 + gap3); ++j) {
			if (j < gap1) {
				ptr += experiment.stride;
			} else if (j < (gap1 + gap2)) {
				ptr += (experiment.stride + delta);
			} else {
				ptr += stride3;
			}
			maccess_noinline(ptr);
        	mfence();
		}
	}
}
			

/**
 * Standard workload for most tests. A loop with a single load in the loop
 * body. Supports positive and negative strides.(noinline)
 *
 * @param      experiment       The experiment
 * @param      mapping          The mapping
 * @param      additional_info  The additional information (must be nullptr)
 */
__attribute__((always_inline)) inline void workload_stride_loop_n_pc(StrideExperiment const& experiment, Mapping const& mapping, void* additional_info) {
	
    uint8_t* ptr_begin = experiment.get_ptr_begin(mapping);
    uint8_t step = experiment.step;
    int64_t stride = experiment.stride;
    for (uint64_t i = 0; i < step; i++) {
        access_fence_128(ptr_begin + i * stride);
		access_fence(ptr_begin + i * stride + 128 * 64);
        //access_fence_32(ptr_begin + i * stride + 64 * 64);
        //access_fence_4(ptr_begin + i * stride + 96 * 64);
    }
}

/**
 * Performs (step) accesses from (step) different PCs in the same memory
 * area.
 *
 * @param      experiment       The experiment
 * @param      mapping          The mapping
 * @param      additional_info  The additional information (must be nullptr)
 */
__attribute__((always_inline)) inline void workload_stride_different_pc_same_memory(StrideExperiment const& experiment, Mapping const& mapping, void* additional_info) {
	assert(additional_info == nullptr);
	assert(experiment.stride > 0);
	assert(experiment.step == 12);
	
	uint8_t* ptr_begin = experiment.get_ptr_begin(mapping);
	maccess(ptr_begin +  0 * experiment.stride);
	maccess(ptr_begin +  1 * experiment.stride);
	maccess(ptr_begin +  2 * experiment.stride);
	maccess(ptr_begin +  3 * experiment.stride);
	maccess(ptr_begin +  4 * experiment.stride);
	maccess(ptr_begin +  5 * experiment.stride);
	maccess(ptr_begin +  6 * experiment.stride);
	maccess(ptr_begin +  7 * experiment.stride);
	maccess(ptr_begin +  8 * experiment.stride);
	maccess(ptr_begin +  9 * experiment.stride);
	maccess(ptr_begin + 10 * experiment.stride);
	maccess(ptr_begin + 11 * experiment.stride);
}

/**
 * Performs (step-*additional_info) accesses in memory area mapping1 and
 * (*additional_info) access in memory area mapping2. All accesses are
 * performed from the same PC.
 *
 * @param      experiment       The experiment
 * @param      mapping1         The mapping 1
 * @param      mapping2         The mapping 2
 * @param      additional_info  The additional information, here: pointer
 *                              to size_t: how many of the (step) accesses
 *                              shall be performed in mapping2?
 */
__attribute__((always_inline)) inline void workload_stride_same_pc_different_memory(StrideExperiment const& experiment, Mapping const& mapping1, Mapping const& mapping2, void* additional_info)  {
	assert(additional_info != nullptr);
	assert(experiment.stride > 0);

	size_t no_accesses_on_mapping2 = *((size_t*)additional_info);

	// perform (step-1) accesses in mapping1 and one final access in mapping2.
	uint8_t* ptr_begin_1 = experiment.get_ptr_begin(mapping1);
	uint8_t* ptr_end_1 = experiment.get_ptr_end(mapping1) - no_accesses_on_mapping2 * experiment.stride;
	uint8_t* ptr_begin_2 = experiment.get_ptr_end(mapping2) - no_accesses_on_mapping2 * experiment.stride;
	uint8_t* ptr_end_2 = experiment.get_ptr_end(mapping2);

	for (uint8_t* ptr = ptr_begin_1; ptr < ptr_end_1; ptr += experiment.stride) {
		maccess_noinline(ptr);
	}
	for (uint8_t* ptr = ptr_begin_2; ptr < ptr_end_2; ptr += experiment.stride) {
		maccess_noinline(ptr);
	}
}

/**
 * Performs (step-*additional_info) accesses in memory area mapping1 with
 * PC1 and (*additional_info) access in memory area mapping2 with PC2.
 *
 * @param      experiment       The experiment
 * @param      mapping1         The mapping 1
 * @param      mapping2         The mapping 2
 * @param      additional_info  The additional information, here: pointer
 *                              to size_t: how many of the (step) accesses
 *                              shall be performed in mapping2?
 */
__attribute__((always_inline)) inline void workload_stride_different_pc_different_memory(StrideExperiment const& experiment, Mapping const& mapping1, Mapping const& mapping2, void* additional_info)  {
	assert(additional_info != nullptr);
	assert(experiment.stride > 0);

	size_t no_accesses_on_mapping2 = *((size_t*)additional_info);

	// perform (step-1) accesses in mapping1 and one final access in mapping2.
	uint8_t* ptr_begin_1 = experiment.get_ptr_begin(mapping1);
	uint8_t* ptr_end_1 = experiment.get_ptr_end(mapping1) - no_accesses_on_mapping2 * experiment.stride;
	uint8_t* ptr_begin_2 = experiment.get_ptr_end(mapping2) - no_accesses_on_mapping2 * experiment.stride;
	uint8_t* ptr_end_2 = experiment.get_ptr_end(mapping2);

	for (uint8_t* ptr = ptr_begin_1; ptr < ptr_end_1; ptr += experiment.stride) {
		maccess(ptr);
	}
	for (uint8_t* ptr = ptr_begin_2; ptr < ptr_end_2; ptr += experiment.stride) {
		maccess(ptr);
	}
}

/**
 * Performs (step-1) accesses with PC1 in memory area mapping1 and 1 access
 * with PC2 in memory area mapping2. PC1 and PC2 have colliding LSBs. The
 * number of colliding bits is specified in additional_info.
 *
 * @param      experiment       The experiment
 * @param      mapping1         The mapping 1
 * @param      mapping2         The mapping 2
 * @param      additional_info  The additional information (expects pointer
 *                              to pair<size_t,size_t>, containing the
 *                              number of colliding bits (first) and the
 *                              number of accesses to perform in mapping2
 *                              (second).)
 */
__attribute__((always_inline)) inline void workload_stride_pc_collision(StrideExperiment const& experiment, Mapping const& mapping1, Mapping const& mapping2, void* additional_info)  {
	assert(additional_info != nullptr);
	assert(experiment.stride > 0);

	pair<size_t, size_t> ai_collidingbits_noaccesses = *((pair<size_t,size_t>*)additional_info);
	size_t const& colliding_bits = ai_collidingbits_noaccesses.first;
	size_t const& no_accesses_on_mapping2 = ai_collidingbits_noaccesses.second;
	assert(no_accesses_on_mapping2 >= 1 && no_accesses_on_mapping2 <= 2);

	// get pointers to co-aligned maccess functions
	pair<maccess_func_t, maccess_func_t> maccess_funcs = get_maccess_functions(colliding_bits);
	maccess_func_t const& maccess_train = maccess_funcs.first;
	maccess_func_t const& maccess_probe = maccess_funcs.second;

	// perform (step-1) accesses in mapping1 and one final access in mapping2.
	uint8_t* ptr_begin_1 = experiment.get_ptr_begin(mapping1);
	uint8_t* ptr_end_1 = experiment.get_ptr_end(mapping1) - no_accesses_on_mapping2 * experiment.stride;
	uint8_t* ptr_begin_2 = experiment.get_ptr_end(mapping2) - no_accesses_on_mapping2 * experiment.stride;
	
	for (uint8_t* ptr = ptr_begin_1; ptr < ptr_end_1; ptr += experiment.stride) {
		maccess_train(ptr);
	}
	if (no_accesses_on_mapping2 == 1) {
		maccess_probe(ptr_begin_2);
	} else if (no_accesses_on_mapping2 == 2) {
		maccess_probe(ptr_begin_2);
		maccess_probe(ptr_begin_2 + experiment.stride);
	} else {
		assert(false);
	}
}

/**
 * Performs (step) accesses with the given stride, but adds a random offset
 * of [0..(CACHE_LINE_SIZE-1)] to each access.
 *
 * @param      experiment       The experiment
 * @param      mapping          The mapping
 * @param      additional_info  The additional information (must be
 *                              nullptr)
 */
__attribute__((always_inline)) inline void workload_stride_random_offset_within_cl(StrideExperiment const& experiment, Mapping const& mapping, void* additional_info) {
	assert(additional_info == nullptr);

	uint8_t* ptr_begin = experiment.get_ptr_begin(mapping);
	uint8_t* ptr_end = experiment.get_ptr_end(mapping);
	
	assert(experiment.step < 64);
	size_t random_offsets[64];
	for (size_t i = 0; i < 64; i++) {
		random_offsets[i] = random_uint32(0, CACHE_LINE_SIZE - 1);
	}

	size_t random_idx = 0;
	for (
		uint8_t* ptr = ptr_begin;
		(ptr_end > ptr_begin) ? (ptr < ptr_end) : (ptr > ptr_end);
		ptr += experiment.stride
	) {
		maccess(ptr + random_offsets[random_idx++]);
	}
}
