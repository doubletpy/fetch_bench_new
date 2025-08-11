#include "testcase_temporal_temporalexperiment.hh"

using json11::Json;
using std::vector;

TemporalExperiment::TemporalExperiment(vector<size_t> training_offsets, vector<size_t> trigger_offsets, bool use_nanosleep, size_t fr_thresh, size_t noise_thresh)
: training_offsets {training_offsets}
, trigger_offsets {trigger_offsets}
, use_nanosleep {use_nanosleep}
, fr_thresh {fr_thresh}
, noise_thresh {noise_thresh}
, t_req { .tv_sec = 0, .tv_nsec = 1000 /* 1µs */ }
{

}

bool
TemporalExperiment::cl_accessed(size_t cl_idx) const
{
    for (size_t const& offset : trigger_offsets) {
		if (offset / CACHE_LINE_SIZE == cl_idx) {
			return true;
		}
	}
	return false;
}


bool
TemporalExperiment::cl_accessed(size_t cl_idx, vector<size_t> current_trigger) const
{
	for (size_t const& offset: current_trigger) {
		if (offset / CACHE_LINE_SIZE == cl_idx) {
			return true;
		}
	}
	return false;
}


bool
TemporalExperiment::cl_potential_prefetch(size_t cl_idx) const
{
    for (size_t const& offset : training_offsets) {
		// if cache line contains at least one offset that's not a trigger
		// location
		if (offset / CACHE_LINE_SIZE == cl_idx) {
			return true;
		}
	}
    return false;

}

vector<size_t>
TemporalExperiment::collect_cache_histogram(Mapping const& mapping, size_t no_repetitions, void (*workload)(TemporalExperiment const&, Mapping const&, void*), void* additional_info)
{
	L::info("%s\n", __FUNCTION__);
    assert(training_offsets.size() > 0);

    vector<size_t> cache_histogram(mapping.size / CACHE_LINE_SIZE, 0);
    for (size_t repetition = 0; repetition <  no_repetitions; ++repetition) {
		//L::info("repeat: %zu\n", repetition);
        flush_mapping(mapping);

        if (workload != nullptr) {
            workload(*this, mapping, additional_info);
        }
        mfence();

        // sleep a while to give the prefetcher some time to work
		if (use_nanosleep) {
		    //nanosleep(&t_req, &t_rem);
            asm volatile (
                ".rept 1000000;"
                "nop;"
                ".endr;"
                :
                :
                :
            );
		}
		mfence();

		// probe probe array
        size_t probe_idx = repetition % (cache_histogram.size());
        probe_single(cache_histogram, probe_idx, mapping.base_addr + (probe_idx * CACHE_LINE_SIZE));

    }
    // normalize cache histogram
	for (size_t& hist_value : cache_histogram) {
		hist_value = hist_value * 1000 / (no_repetitions / cache_histogram.size());
	}
	return cache_histogram;
}


vector<vector<size_t>>
TemporalExperiment::collect_cache_histogram_vec(Mapping const& mapping, size_t no_repetitions, void (*workload)(TemporalExperiment const&, Mapping const&, vector<size_t>&, void*), void* additional_info)
{
	L::info("%s\n", __FUNCTION__);
    assert(training_offsets.size() > 0);

    vector<vector<size_t>> cache_histogram_vec(training_offsets.size(), vector<size_t>(mapping.size / CACHE_LINE_SIZE, 0));
	for (size_t i = 0; i < training_offsets.size(); i ++) {
		vector<size_t> current_trigger { training_offsets[i] };
		L::info("num_addr %ld, Test trigger[%ld]\n", training_offsets.size(), i);
		for (size_t repetition = 0; repetition <  no_repetitions; ++repetition) {
			//L::info("repeat: %zu\n", repetition);
			flush_mapping(mapping);

			if (workload != nullptr) {
				workload(*this, mapping, current_trigger, additional_info);
			}
			mfence();

			// sleep a while to give the prefetcher some time to work
			if (use_nanosleep) {
				//nanosleep(&t_req, &t_rem);
				asm volatile (
					".rept 1000000;"
					"nop;"
					".endr;"
					:
					:
					:
				);
			}
			mfence();

			// probe probe array
			size_t probe_idx = repetition % (cache_histogram_vec[i].size());
			probe_single(cache_histogram_vec[i], probe_idx, mapping.base_addr + (probe_idx * CACHE_LINE_SIZE));

		}
	}
	// normalize cache histogram
    for (auto& cache_histogram : cache_histogram_vec) {
	    for (size_t& hist_value : cache_histogram) {
		    hist_value = hist_value * 1000 / (no_repetitions / cache_histogram.size());
	    }
    }
	return cache_histogram_vec;
}




vector<bool>
TemporalExperiment::evaluate_cache_histogram(vector<size_t> const& cache_histogram, size_t no_repetitions, double threshold_multiplier) const
{
    // compute averages for (a) all locations where we expect hits,
	// (b) all locations where we expect misses
	L::info("%s\n", __FUNCTION__);
	size_t hit_avg = 0, hit_n = 0;
	size_t miss_avg = 0, miss_n = 0;
    for (size_t cl_idx = 0; cl_idx < cache_histogram.size(); cl_idx++) {
		if (cl_accessed(cl_idx)) {
			// architectural hit
			L::debug("expecting hit at %2zu:               %6zu\n", cl_idx, cache_histogram[cl_idx]);
			hit_avg += cache_histogram[cl_idx];
			hit_n++;
		} else if (cl_potential_prefetch(cl_idx)) {
			// prefetch (ignore for now)
			L::debug("potential prefetch location at %2zu: %6zu\n", cl_idx, cache_histogram[cl_idx]);
		} else {
			// miss location
			L::debug("miss location at %2zu:          %6zu\n", cl_idx, cache_histogram[cl_idx]);
			miss_avg += cache_histogram[cl_idx];
			miss_n++;
		}
	}
    if (hit_n > 0) {
		hit_avg /= hit_n;
	}
	if (miss_n > 0) {
		miss_avg /= miss_n;
	}
    L::debug("average hit:       %3zu\n", hit_avg);
	L::debug("average miss:      %3zu\n", miss_avg);
    size_t prefetch_thresh = miss_avg + (size_t)(threshold_multiplier * (double)(hit_avg - miss_avg));
	if (prefetch_thresh <= 0) { prefetch_thresh = 1; }
	L::debug("prefetch thresh:   %3zu\n", prefetch_thresh);

    // iterate over the possible prefetch locations and use the prefetch_threshold
	// to decide whether this is a prefetch or not.
	vector<bool> prefetch_vector (cache_histogram.size(), false);
	for (size_t cl_idx = 0; cl_idx < cache_histogram.size(); cl_idx++) {
		// check whether this is a prefetch location or not
		if (cl_potential_prefetch(cl_idx) && !cl_accessed(cl_idx)) {
			L::debug("potential prefetch location at %2zu: %6zu\n", cl_idx, cache_histogram[cl_idx]);
			// check whether the value exceeds the noise threshold
			if (cache_histogram[cl_idx] > noise_thresh) {
				L::debug(" *** Exceeds noise threshold (%zu > %zu)\n", cache_histogram[cl_idx], noise_thresh);
				// check whether the value exceeds the prefetch threshold
				if (cache_histogram[cl_idx] >= prefetch_thresh) {
					L::debug(" *** I think this is a prefetch (%zu >= %zu). ***\n", cache_histogram[cl_idx], prefetch_thresh);
					prefetch_vector[cl_idx] = true;
				}
			}
		}
	}
	return prefetch_vector;
}


vector<bool>
TemporalExperiment::evaluate_cache_histogram(vector<size_t> const& cache_histogram, size_t no_repetitions) const
{
	return evaluate_cache_histogram(cache_histogram, no_repetitions, 1.0/64);
}

vector<vector<bool>>
TemporalExperiment::evaluate_cache_histogram_vec(vector<vector<size_t>> const& cache_histogram_vec, size_t no_repetitions) const
{
	size_t hit_avg = 0, hit_n = 0;
	size_t miss_avg = 0, miss_n = 0;
	vector<size_t> prefetch_thresh_vec;
	for (size_t i = 0; i < cache_histogram_vec.size(); ++i) {
		L::info("num_addr %ld, evaluate trigger[%ld]\n", cache_histogram_vec.size(), i);
		for (size_t cl_idx = 0; cl_idx < cache_histogram_vec[i].size(); cl_idx++) {
			if (cl_accessed(cl_idx, { training_offsets[i] })) {
				// architectural hit
				L::debug("expecting hit at %2zu:               %6zu\n", cl_idx, cache_histogram_vec[i][cl_idx]);
				hit_avg += cache_histogram_vec[i][cl_idx];
				hit_n++;
			} else if (cl_potential_prefetch(cl_idx)) {
				// prefetch (ignore for now)
				L::debug("potential prefetch location at %2zu: %6zu\n", cl_idx, cache_histogram_vec[i][cl_idx]);
			} else {
				// miss location
				L::debug("miss location at %2zu:          %6zu\n", cl_idx, cache_histogram_vec[i][cl_idx]);
				miss_avg += cache_histogram_vec[i][cl_idx];
				miss_n++;
			}
		}
		if (hit_n > 0) {
			hit_avg /= hit_n;
		}
		if (miss_n > 0) {
			miss_avg /= miss_n;
		}
		L::debug("average hit:       %3zu\n", hit_avg);
		L::debug("average miss:      %3zu\n", miss_avg);
		size_t prefetch_thresh = miss_avg + (size_t)(1.0/64 * (double)(hit_avg - miss_avg));
		if (prefetch_thresh <= 0) { prefetch_thresh = 1; }
		L::debug("prefetch thresh:   %3zu\n", prefetch_thresh);
		prefetch_thresh_vec.push_back(prefetch_thresh);
	}

	vector<vector<bool>> prefetch_vector_vec (cache_histogram_vec.size(), vector<bool>(cache_histogram_vec[0].size(), false));
	for (size_t i = 0; i < cache_histogram_vec.size(); i++) {
		for (size_t cl_idx = 0; cl_idx < cache_histogram_vec[i].size(); cl_idx++) {
			// check whether this is a prefetch location or not
			if (cl_potential_prefetch(cl_idx) && !cl_accessed(cl_idx, { training_offsets[i]})) {
				L::debug("potential prefetch location at %2zu: %6zu\n", cl_idx, cache_histogram_vec[i][cl_idx]);
				// check whether the value exceeds the noise threshold
				if (cache_histogram_vec[i][cl_idx] > noise_thresh) {
					L::debug(" *** Exceeds noise threshold (%zu > %zu)\n", cache_histogram_vec[i][cl_idx], noise_thresh);
					// check whether the value exceeds the prefetch threshold
					if (cache_histogram_vec[i][cl_idx] >= prefetch_thresh_vec[i]) {
						L::debug(" *** I think this is a prefetch (%zu >= %zu). ***\n", cache_histogram_vec[i][cl_idx], prefetch_thresh_vec[i]);
						prefetch_vector_vec[i][cl_idx] = true;
					}
				}
			}
		}
	}
	return prefetch_vector_vec;
}



void
TemporalExperiment::dump(vector<size_t> const& cache_histogram, vector<bool> prefetch_vector, string const& filepath) const
{
    // Build a JSON array from the cache histogram numbers
	Json::array cache_histogram_values {};
	for (size_t i = 0; i < cache_histogram.size(); i++) {
		cache_histogram_values.push_back((int)cache_histogram[i]);
	}
	Json::array prefetch_vector_values {};
	for (size_t i = 0; i < prefetch_vector.size(); i++) {
		prefetch_vector_values.push_back((bool)prefetch_vector[i]);
	}
	Json::array training_offsets_values {};
	for (size_t i = 0; i < training_offsets.size(); i++) {
		training_offsets_values.push_back((int)training_offsets[i]);
	}
	Json::array trigger_offsets_values {};
	for (size_t i = 0; i < trigger_offsets.size(); i++) {
		trigger_offsets_values.push_back((int)trigger_offsets[i]);
	}

    Json j = Json::object {
		{ "training_offsets", training_offsets_values },
		{ "trigger_offsets", trigger_offsets_values },
		{ "use_nanosleep", use_nanosleep },
		{ "fr_thresh", (int)fr_thresh },
		{ "noise_thresh", (int)noise_thresh },
		{ "cache_histogram", cache_histogram_values },
		{ "prefetch_vector", prefetch_vector },
		{ "cache_line_size", CACHE_LINE_SIZE },
	};
	
	json_dump_to_file(j, filepath);
}