#pragma once

#include <cassert>
#include <limits>
#include <vector>
#include <unistd.h>
#include <utility>
#include <algorithm>
#include <map>
#include <sys/mman.h>
#include "json11.hpp"
#include <cstdlib>
#include <ctime>
#include <unordered_set>

#include "testcase.hh"
#include "cacheutils.hh"
#include "logger.hh"
#include "utils.hh"
#include "mapping.hh"


#include "testcase_temporal_temporalexperiment.hh"

using json11::Json;

class TestCaseTemporal: public TestCaseBase
{
  private:
	size_t const fr_thresh;
	size_t const noise_thresh;
	bool use_nanosleep = false;


  public:
    TestCaseTemporal(size_t fr_thresh, size_t noise_thresh, bool use_nanosleep)
    : fr_thresh {fr_thresh}
	, noise_thresh {noise_thresh}
	, use_nanosleep {use_nanosleep}
    {}
    
    virtual string id() override{
        return "temporal";
    }

  protected:
    virtual Json pre_test() override {
        architecture_t arch = get_arch();
        if (arch == ARCH_INTEL) {
			set_intel_prefetcher(-1, INTEL_L2_HW_PREFETCHER, false);
			set_intel_prefetcher(-1, INTEL_L2_ADJACENT_CL_PREFETCHER, false);
			set_intel_prefetcher(-1, INTEL_DCU_PREFETCHER, false);
			set_intel_prefetcher(-1, INTEL_DCU_IP_PREFETCHER, false);
		} else if (arch == ARCH_AMD) {
            set_amd_prefetcher(-1, AMD_L1_STRIDE, false);
            set_amd_prefetcher(-1, AMD_L1_STREAM, false);
            set_amd_prefetcher(-1, AMD_L1_REGION, false);
            set_amd_prefetcher(-1, AMD_L2_STREAM, false);
            set_amd_prefetcher(-1, AMD_L2_UPDOWN, false);
        }     
		return Json::object {
			{"architecture", arch},
		};
    }

    virtual Json post_test() override {
        if (get_arch() == ARCH_INTEL) {
			set_intel_prefetcher(-1, INTEL_L2_HW_PREFETCHER, true);
			set_intel_prefetcher(-1, INTEL_L2_ADJACENT_CL_PREFETCHER, true);
			set_intel_prefetcher(-1, INTEL_DCU_PREFETCHER, true);
			set_intel_prefetcher(-1, INTEL_DCU_IP_PREFETCHER, true);
		}  else if (get_arch() == ARCH_AMD) {
            set_amd_prefetcher(-1, AMD_L1_STRIDE, true);
            set_amd_prefetcher(-1, AMD_L1_STREAM, true);
            set_amd_prefetcher(-1, AMD_L1_REGION, true);
            set_amd_prefetcher(-1, AMD_L2_STREAM, true);
            set_amd_prefetcher(-1, AMD_L2_UPDOWN, true);
        }

		return Json::object {};
    }

    Json test_trigger_same_pc_same_memory(size_t no_repetitions, size_t num_addr) {
        L::info("Test: %s\n", __FUNCTION__);
        Mapping mapping = allocate_mapping(64 * PAGE_SIZE);
        random_activity(mapping);
		flush_mapping(mapping);
        vector<size_t> training_offsets;
        std::srand(std::time(nullptr));
        for (size_t i = 0; i < num_addr; ++i) {
            size_t offset = rand() % (64 * 64);
            training_offsets.push_back(offset * CACHE_LINE_SIZE);
            L::info("addr[%lu]: %lu/%lu ", i, offset, offset * CACHE_LINE_SIZE);
        }
        L::info("\n");
        vector<size_t> trigger_offsets { training_offsets[0] };
        TemporalExperiment experiment { training_offsets, trigger_offsets, use_nanosleep, fr_thresh, noise_thresh };
        //training and trigger
        vector<size_t> cache_histogram = experiment.collect_cache_histogram(mapping, no_repetitions, workload_temporal_same_pc_same_memory, nullptr);

        //evaluate
        vector<bool> prefetch_vector = experiment.evaluate_cache_histogram(cache_histogram, no_repetitions);
        experiment.dump(cache_histogram, prefetch_vector, "trace-temporal-test_trigger_same_pc_same_memory.json");
        size_t prefetch_count = std::count(prefetch_vector.begin(), prefetch_vector.end(), true);

        unmap_mapping(mapping);

        return Json::object {
            {"status", "completed"},
			{"triggers_prefetch", (prefetch_count > 0)},
        };

    }


    Json test_delta_range(size_t no_repetitions)
    {
        L::info("Test: %s\n", __FUNCTION__);
        Mapping mapping = allocate_mapping(4096 * PAGE_SIZE);
        flush_mapping(mapping);
        map<ssize_t, size_t> delta_hist;

        for (ssize_t sign : {-1, 1}) {           
            for (ssize_t offset = sign * 1 * CACHE_LINE_SIZE; std::abs(offset) <= 4096 * PAGE_SIZE;  offset *= 2) {
                Mapping sub_mapping {.base_addr = mapping.base_addr, .size = (std::abs(offset) + CACHE_LINE_SIZE )};
                //temporal pattern: {first_access, first_acccess + delta} 
                size_t first_access_offset = (sign == 1) ? 0 : (sub_mapping.size - CACHE_LINE_SIZE);                
                vector<size_t> training_offsets;
                training_offsets.push_back(first_access_offset);
                L::info("trigger_addr: %ld/%ld\n", first_access_offset/CACHE_LINE_SIZE, first_access_offset);
                training_offsets.push_back(first_access_offset + offset);
                L::info("training_addr: %ld/%ld\n", (first_access_offset + offset)/CACHE_LINE_SIZE, first_access_offset + offset);
                //trigger offsets
                vector<size_t> trigger_offsets { training_offsets[0] };

                //experiment for each delta
                TemporalExperiment experiment { training_offsets, trigger_offsets, use_nanosleep, fr_thresh, noise_thresh };
                //training and trigger
                vector<size_t> cache_histogram = experiment.collect_cache_histogram(sub_mapping, no_repetitions, workload_temporal_same_pc_same_memory, nullptr);
                //evaluate
                vector<bool> prefetch_vector = experiment.evaluate_cache_histogram(cache_histogram, no_repetitions);
                size_t prefetch_count = std::count(prefetch_vector.begin(), prefetch_vector.end(), true);
                L::info("delta: %ld/%ld, trigger_prefetch: %lu\n", offset/CACHE_LINE_SIZE, offset, prefetch_count);
                L::info("\n");
                delta_hist[offset] = prefetch_count;
                flush_mapping(mapping);
            }
        }

        ssize_t min_delta_neg = std::numeric_limits<ssize_t>::min();
		ssize_t min_delta_pos = std::numeric_limits<ssize_t>::max();
		ssize_t max_delta_neg = std::numeric_limits<ssize_t>::max();
		ssize_t max_delta_pos = std::numeric_limits<ssize_t>::min();

        for (pair<ssize_t const, size_t> const& delta_hist_pair : delta_hist) {
            ssize_t const& delta = delta_hist_pair.first;
            size_t const& max_count = delta_hist_pair.second;
            if (delta > 0 && delta < min_delta_pos && max_count > 0) { min_delta_pos = delta; }
			if (delta < 0 && delta > min_delta_neg && max_count > 0) { min_delta_neg = delta; }
			if (max_delta_pos < delta && max_count > 0) { max_delta_pos = delta; }
			if (max_delta_neg > delta && max_count > 0) { max_delta_neg = delta; }
			L::debug("delta: %zd, max. count: %zu\n", delta, max_count);
        }

        unmap_mapping(mapping);

		return Json::object {
			{"status", "completed"},
			{"min_delta_negative", (min_delta_neg != std::numeric_limits<ssize_t>::min()) ? (int)min_delta_neg : 0},
			{"min_delta_positive", (min_delta_pos != std::numeric_limits<ssize_t>::max()) ? (int)min_delta_pos : 0},
			{"max_delta_negative", (max_delta_neg < 0) ? (int)max_delta_neg : 0},
			{"max_delta_positive", (max_delta_pos > 0) ? (int)max_delta_pos : 0},
		};
    }


    Json
    test_metadata_entry(size_t no_repetitions)
    {
        L::info("Test: %s\n", __FUNCTION__);
        for (size_t num_addr = 1024; num_addr <= 262144; num_addr *= 2) {
            assert(num_addr % 64 == 0);
            Mapping mapping = allocate_mapping(num_addr * CACHE_LINE_SIZE * 4);
            flush_mapping(mapping);
            
            vector<size_t> training_offsets;
            std::unordered_set<size_t> existing;
            std::srand(std::time(nullptr));
            size_t i = 0;
            while(training_offsets.size() < num_addr) {
                size_t offset = rand() % (num_addr * 4);
                if (existing.find(offset * CACHE_LINE_SIZE) == existing.end()){
                    existing.insert(offset * CACHE_LINE_SIZE);
                    training_offsets.push_back(offset * CACHE_LINE_SIZE);
                    L::info("addr[%lu]: %lu/%lu\n ", i, offset, offset * CACHE_LINE_SIZE);
                    ++i;
                }
            }
            TemporalExperiment experiment {training_offsets, training_offsets, use_nanosleep, fr_thresh, noise_thresh};
            vector<vector<size_t>> cache_histogram_vec = experiment.collect_cache_histogram_vec(mapping, no_repetitions, workload_temporal_entry, nullptr);
            vector<vector<bool>> prefetch_vector_vec = experiment.evaluate_cache_histogram_vec(cache_histogram_vec, no_repetitions);

            for (size_t i = 0; i < prefetch_vector_vec.size(); i++) {
                size_t prefetch_count = 0;
                for (bool const is_pf : prefetch_vector_vec[i]) {
                    if (is_pf) {
                        prefetch_count++;
                    }
                }
                printf("num_addr %ld: trigger[%lu] %lu/%lu, number of prefetches %zu\n", num_addr, i, training_offsets[i]/CACHE_LINE_SIZE, training_offsets[i], prefetch_count);
            }
            unmap_mapping(mapping);
            training_offsets.clear();
        }

        // stream detected when both positive and negative streams have more than 50% two consecutive prefetches
		return Json::object {
			{"status", "completed"},
		};

    }


    virtual Json identify() override {
        size_t no_repetitions = 400000 * (PAGE_SIZE / 4096);
        return Json::object {
            { "test_same_pc_same_memory_temporal", test_trigger_same_pc_same_memory(no_repetitions, 15) },
            //{ "test_delta_range", test_delta_range(no_repetitions) },
            //{ "test_entry", test_metadata_entry(no_repetitions) },

		};
    }

    virtual Json characterize() override {
		size_t no_repetitions = 400000 * (PAGE_SIZE / 4096);

		return Json::object {
			{ "test_same_pc_same_memory_temporal", test_trigger_same_pc_same_memory(no_repetitions, 15) },
		};
	}

};

