#include "Data.h"

//const float Data::BASELINE_INCREMENT = 0.5f;
//const float Data::VARIABILITY_INCREMENT = 0.03125f;

Data::Data()
{
	/*
	if (b_write_to_file)
	{
		OpenFile();
	}
	*/
	next_sample_ptr = new std::vector<short>(ELECTRODE_ARRAY_SIZE, 0);
	// spikes will be 512 length vector of vectors which hold times for all found spikes
}

Data::~Data()
{
	/*
	if (b_file_is_open)
		fclose(file);
	*/
	for (size_t i = 0; i < data_ol.size(); i++)
	{
		PopData();
	}
}

void Data::LoadStimFile(std::string filename)
{
	stim = std::make_unique<Stim>(filename);
	stim->OpenAndReadFile();
	std::vector<int> channels_stimulated = GetStimChannelsVec();
	for (auto it = channels_stimulated.begin(); it != channels_stimulated.end(); it++)
	{
		psth_data.insert(std::make_pair(*it,std::vector<int>()));
	}
	next_stim_event = stim->GetNextStimTime();
}

void Data::set(int ch, unsigned int sample, short value)
{
	// If start of new sample
	if (sample > samples_parsed) // add previous completed sample and create an empty next sample
	{
		// lock thread
		std::lock_guard<std::mutex> guard(data_mutex);
		data_ol.push_back(next_sample_ptr);
		// unlock thread
		next_sample_ptr = new std::vector<short>(ELECTRODE_ARRAY_SIZE, 0);
		samples_parsed++;
	}
	// add current value to next sample
	next_sample_ptr->at(ch) = value;
}

void Data::PopData()
{
	std::lock_guard<std::mutex> guard(data_mutex);
	delete data_ol.front();
	data_ol.pop_front();
}

void Data::AnalyzeData()
{
	//LOG("Analysis Begin");
	// For every sample we haven't analyzed yet
	while (to_analyze < data_ol.size())
	{
		/*  BEGIN ANALYSIS  */
		// get pointer to next sample to analyze and analyze it
		std::vector<short>* ptr_to_analyze = data_ol.at(to_analyze);
		//LOG("Updating Noise");
		UpdateRunningNoise(ptr_to_analyze);
		//LOG("Spike Detection");
		SpikeDetectOL(ptr_to_analyze);
		//LOG("Clearing Old Spikes");
		ClearOldSpikes();
		/*  END ANALYSIS  */
		//LOG("Increasing counters");
		// If not full, increase counter. If full, pop first sample
		if (to_analyze < MAX_QUEUE_LENGTH)
		{
			to_analyze++;
		}
		else
		{
			PopData();
		}
		samples_analyzed++;
		stim_tracker_updated_this_frame = false;
	}
	//LOG("Analysis End");
	//LOG(spikes.size() << " spikes found");
}

std::vector<int> Data::GetRawData(int ch) const
{
	std::vector<int> raw_data;
	std::lock_guard<std::mutex> guard(data_mutex);
	int n = data_ol.size();
	for (auto it = data_ol.begin(); it != data_ol.end(); it++)
	{
		raw_data.push_back((*it)->at(ch));
	}
	return raw_data;
}

std::pair<float, float> Data::GetMeanAndThresh(int ch) const
{
	return std::pair<float, float>(mean[ch], threshold_amp[ch]);
}

std::deque<Spike> Data::GetSpikes() const
{
	//static std::vector<Spike> spikes_to_send;
	std::lock_guard<std::mutex> guard(spike_mutex);
	return spikes;
}

std::vector<int>& Data::GetStimChannelsVec() const
{
	return stim->GetStimChannelsVec();
}

std::vector<int> Data::GetPSTHData(int ch) const
{
	std::lock_guard<std::mutex> guard(psth_mutex);
	return psth_data.at(ch);
}

short Data::median(std::vector<short> v)
{
	std::size_t n = v.size();
	if (n == 0) //empty
		return 0;
	else if (n % 2 == 0) //even
	{
		std::nth_element(v.begin(), v.begin() + n / 2,	v.end());
		std::nth_element(v.begin(), v.begin() + (n - 1) / 2, v.end());
		return (short)(v[n / 2] + v[(n - 1) / 2]) / 2;
	}
	else //odd
	{
		std::nth_element(v.begin(), v.begin() + n / 2, v.end());
		return v[n / 2];
	}
}

void Data::UpdateRunningNoise(std::vector<short>* sample)
{
	unsigned int n = to_analyze;
	
	if (n < MAX_QUEUE_LENGTH) // if not full yet
	{
		std::lock_guard<std::mutex> guard(data_mutex);
		for (int ch = 0; ch < ELECTRODE_ARRAY_SIZE; ch++)
		{
			long long next = (long long)sample->at(ch);
			sum[ch] += next;
			sumsq[ch] += next * next;

			float mean_temp = (float)sum[ch] / n;
			mean[ch] = mean_temp;
			float var = (float)sumsq[ch] / n - (mean_temp * mean_temp);
			threshold_amp[ch] = THRESHOLD_ALPHA * sqrt(var);
		}
	}
	else // is full
	{
		std::lock_guard<std::mutex> guard(data_mutex);
		for (int ch = 0; ch < ELECTRODE_ARRAY_SIZE; ch++)
		{
			long long next = (long long)sample->at(ch);
			long long last = (long long)data_ol.front()->at(ch);
			sum[ch] += next - last;
			sumsq[ch] += (next * next) - (last * last);

			float mean_temp = (float)sum[ch] / n;
			mean[ch] = mean_temp;
			float var = (float)sumsq[ch] / n - (mean_temp * mean_temp);
			threshold_amp[ch] = THRESHOLD_ALPHA * sqrt(var);
		}
	}
	
}

void Data::SpikeDetectOL(std::vector<short>* sample)
{
	for (int ch = 0; ch < ELECTRODE_ARRAY_SIZE; ch++)
	{
		float datum = (float)sample->at(ch);
		float amplitude = mean[ch] - datum; // positive for downward spikes
		if (amplitude > threshold_amp[ch])
		{
			// either start of new sub_threshold event or continuation of one
			sub_thresh_dur[ch]++;
			if (datum < max_amp[ch])
			{
				max_amp[ch] = amplitude;
				max_amp_time[ch] = samples_analyzed;
			}
		}
		else
		{
			unsigned int dur = sub_thresh_dur[ch];
			// if end of sub threshold event
			if (dur > 0)
			{
				// if it's a valid spike
				if (dur >= 6 && dur <= 60)
				{
					AddSpike(ch, max_amp_time[ch], max_amp[ch]);
				}
				// reset parameters
				sub_thresh_dur[ch] = 0;
				max_amp[ch] = 0.0f;
			}
		}
	}
}

void Data::AddSpike(int ch, unsigned int time, float amplitude)
{
	// add new spike data
	{
		std::lock_guard<std::mutex> guard(spike_mutex);
		spikes.push_back({ ch, time, amplitude });
	}
	if (stim)
	{
		AddSpikeToPSTH(ch, time);
	}
}

void Data::ClearOldSpikes()
{
	// Get rid of old spike data to conserve space.
	
	while (!spikes.empty() && spikes.front().time < samples_analyzed - SPIKES_MAX_TIME_STORED)
	{
		spikes.pop_front();
	}
}

void Data::AddSpikeToPSTH(int ch, unsigned int time)
{
	UpdateStimTracker();
	LOG("Add Spike to PSTH");
	for (auto it = stim_event_tracker.begin(); it != stim_event_tracker.end(); it++)
	{
		int time_diff = time - (*it)->GetTime();
		int stim_ch = (*it)->channel;
		if (time_diff > PSTH_DEADZONE && time_diff < PSTH_MAX_RANGE)
		{
			// lock thread and push value into psth_data
			std::lock_guard<std::mutex> guard(psth_mutex);
			psth_data.at(stim_ch).push_back(time_diff);
		}
	}
	LOG("Spike Added to PSTH");
}

void Data::UpdateStimTracker()
{
	LOG("Update Stim Tracker");
	if (!stim_tracker_updated_this_frame)
	{
		// Add any stim events triggered in the current analysis frame
		while (next_stim_event->GetTime() <= samples_analyzed)
		{
			stim_event_tracker.push_back(next_stim_event);
			next_stim_event = stim->GetNextStimTime();
		}
		// Get rid of any stim events that are more than PSTH_MAX_RANGE samples behind
		while (stim_event_tracker.front()->GetTime() < samples_analyzed - PSTH_MAX_RANGE)
		{
			stim_event_tracker.pop_front();
		}
		stim_tracker_updated_this_frame = true;
	}	
	LOG("Stim Tracker Updated");
}

/*
void Data::UpdateBaselineAndVariability()
{
	for (int ch = 0; ch < ELECTRODE_ARRAY_SIZE; ch++)
	{
		float datum = (float)data_ol[ch].front();
		float base = baseline[ch];
		float var = variability[ch];
		if (datum <= base)
		{
			if (datum > base - var)
			{
				DecreaseVariability(ch);
			}
			else
			{
				DecreaseBaseline(ch);
				if (datum <= base - 6 * var)
				{
					DecreaseVariability(ch);
				}
				else if (datum > base - 5 * var)
				{
					IncreaseVariability(ch);
				}
			}
		}
		else if (datum > base + var)
		{
			IncreaseBaseline(ch);
		}
	}
}

void Data::IncreaseBaseline(int ch)
{
	baseline[ch] += 0.5 * BASELINE_INCREMENT * variability[ch];
}

void Data::DecreaseBaseline(int ch)
{
	baseline[ch] -= BASELINE_INCREMENT * variability[ch];
}

void Data::IncreaseVariability(int ch)
{
	variability[ch] += VARIABILITY_INCREMENT;
}

void Data::DecreaseVariability(int ch)
{
	variability[ch] -= VARIABILITY_INCREMENT;
}
*/

/*
int Data::CalculateAverage()
{
	for (int ch = 0; ch < ELECTRODE_ARRAY_SIZE; ch++)
	{
		int sum = std::accumulate(data[ch].begin(), data[ch].end(), 0);
		avg[ch] = static_cast<double>(sum) / data[ch].size();
	}
	b_avg_calculated = true;
	return 0;
}

int Data::CalculateStandardDeviation()
{
	if (!b_avg_calculated)
	{
		CalculateAverage();
	}
	for (int ch = 0; ch < ELECTRODE_ARRAY_SIZE; ch++)
	{
		double sqsum = 0.0;
		for (int n = 0; n < SAMPLE_RATE; n++)
		{
			sqsum += (data[ch][n] - avg[ch]) * (data[ch][n] - avg[ch]);
		}
		stdev[ch] = sqsum / SAMPLE_RATE;
	}
	return 0;
}

int Data::SpikeFinding()
{
	// Initialize
	bool below_threshold = false;
	int below_threshold_duration = 0;
	short below_threshold_min_val = 2048;
	int below_threshold_min_sample = 0;
	ClearSpikeTimes();
	// For each channel
	for (int ch = 0; ch < ELECTRODE_ARRAY_SIZE; ch++)
	{
		// Calculate Threshold
		short threshold = (short)(avg[ch] - (THRESHOLD_ALPHA * stdev[ch]));
		// Loop through every voltage value
		for (int sample = 0; sample < SAMPLE_RATE; sample++)
		{
			// If voltage is below threshold
			short datum = data[ch][sample];
			if (datum < threshold)
			{
				below_threshold = true;
				below_threshold_duration++;
				if (datum < below_threshold_min_val)
				{
					below_threshold_min_val = datum;
					below_threshold_min_sample = sample;
				}

			}
			else
			{
				// If we reached the end of a below threshold segment and the duration was between 0.3 and 3 ms
				if (below_threshold && below_threshold_duration >= 6 && below_threshold_duration <= 60)
				{
					// There was a spike on the channel. Add it to the list of spike times
					spikes[ch].push_back(below_threshold_min_sample);
				}
				// reset duration tracker
				below_threshold = false;
				below_threshold_duration = 0;
				below_threshold_min_val = 2048;
			}

		}
	}
	return 0;
}

int Data::ClearSpikeTimes()
{
	spikes.resize(ELECTRODE_ARRAY_SIZE, std::vector<unsigned int>());
	return 0;
}
*/

/*
bool Data::OpenFile()
{
	file = fopen("test_data.bin", "wb");
	if (file)
	{
		b_file_is_open = true;
		return true;
	}
	else
	{
		return false;
	}
}
*/

/*
int Data::WriteToFile()
{
	if (!b_file_is_open && b_write_to_file)
	{
		//OpenFile();
	}
	for (int n = 0; n < ELECTRODE_ARRAY_SIZE; n++)
	{
		short *arr = new short[SAMPLE_RATE];
		std::copy(data[n].begin(), data[n].end(), arr);
		fwrite(&arr[0], sizeof(short), SAMPLE_RATE , file);
		delete [] arr;
	}
	return 0;
}
*/