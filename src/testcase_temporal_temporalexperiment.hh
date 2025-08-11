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



class TemporalExperiment {
  public:
    // offsets to access to train a pattern, from the beginning of the
	// mapping, in bytes
	vector<size_t> const training_offsets;
	// offsets to access to trigger the pattern, from the beginning of the
	// mapping, in bytes
	vector<size_t> const trigger_offsets;
	// wait before probing or not
	bool const use_nanosleep;
	// Flush+Reload threshold
	size_t const fr_thresh;
	// Flush+Reload noise threshold
	size_t const noise_thresh;

    //struct  for nanosleep
    struct timespec const t_req;
    struct timespec t_rem;
//  private:
//    vector<ssize_t> distances;
  public:
    TemporalExperiment(vector<size_t> training_offsets, vector<size_t> trigger_offsets, bool use_nanosleep, size_t fr_thresh, size_t noise_thresh);
    bool cl_accessed(size_t cl_idx) const;
    bool cl_accessed(size_t cl_idx, vector<size_t> current_trigger) const;
    bool cl_potential_prefetch(size_t cl_idx) const;
    vector<size_t> collect_cache_histogram(Mapping const& mapping, size_t no_repretitions, void (*workload)(TemporalExperiment const&, Mapping const&, void*), void* additioinal_info);
    vector<vector<size_t>> collect_cache_histogram_vec(Mapping const& mapping, size_t no_repetitions, void (*workload)(TemporalExperiment const&, Mapping const&, vector<size_t>&, void*), void* additional_info);
    vector<bool> evaluate_cache_histogram(vector<size_t> const& cache_histogram, size_t no_repetitions, double threshold_multiplier) const;
    vector<bool> evaluate_cache_histogram(vector<size_t> const& cache_histogram, size_t no_repetitions) const;
    vector<vector<bool>> evaluate_cache_histogram_vec(vector<vector<size_t>> const& cache_histogram_vec, size_t no_repetitions) const;
    void dump(vector<size_t> const& cache_histogram, vector<bool> prefetch_vector, string const& filepath) const;

  private:
    inline void probe_single(vector<size_t>& cache_histogram, size_t idx, uint8_t* ptr) const __attribute__((always_inline)) {
		assert(idx < cache_histogram.size());
		size_t time = flush_reload_t(ptr);
		cache_histogram[idx] += (time < fr_thresh) ? 1 : 0;
	}

};

__attribute__((always_inline)) inline void workload_temporal_same_pc_same_memory(TemporalExperiment const& experiment, Mapping const& mapping, void* additional_info) {
	// Training in mapping
    for (size_t loop = 0; loop < 10; loop++) {
        for (size_t offset : experiment.training_offsets) {
            maccess_noinline(mapping.base_addr + offset);
            mfence();
        }

        flush_mapping(mapping);
    }

	// Trigger in mapping (same PC)
	for (size_t offset : experiment.trigger_offsets) {
		maccess_noinline(mapping.base_addr + offset);
        mfence();
	}
}

__attribute__((always_inline)) inline void workload_temporal_entry(TemporalExperiment const& experiment, Mapping const& mapping, vector<size_t>& current_trigger, void* additional_info) {
	// Training in mapping
    for (size_t loop = 0; loop < 10; loop++) {
        for (size_t offset : experiment.training_offsets) {
            maccess_noinline(mapping.base_addr + offset);
            mfence();
        }

        flush_mapping(mapping);
    }

	// Trigger in mapping (same PC)
	for (size_t offset : current_trigger) {
		maccess_noinline(mapping.base_addr + offset);
        mfence();
	}
}

