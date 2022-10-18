#pragma once
#define _CRT_SECURE_NO_DEPRECATE
#include <iostream>
#include <iomanip>
#include <vector>
#include <queue>
#include <algorithm>
#include <math.h>
#include <numeric>
#include <mutex>

#include "Stim.h"

struct Spike
{
  int channel;
  unsigned int time;
  float amplitude;
};

class Data
{
public:
  Data();
  ~Data();

  void LoadParams(); // Load paramaters from file for spike sorting and analysis (NOT IMPLEMENTED)
  void LoadStimFile(std::string filename);

  void set(int ch, unsigned int sample, short value);
  void PopData();
  void AnalyzeData();

  inline unsigned int GetSamplesParsed() const { return samples_parsed; };
  inline unsigned int GetSamplesAnalyzed() const { return samples_analyzed; };

  std::vector<int> GetRawData(int ch) const;
  std::pair<float, float> GetMeanAndThresh(int ch) const;
  std::deque<Spike> GetSpikes() const;
  std::vector<int>& GetStimChannelsVec() const;
  std::vector<int> GetPSTHData(int ch) const;

  //inline short get(int ch, int sample) { return data[ch][sample]; }
  //int WriteToFile();
private:
   // Constants
  static const int ELECTRODE_ARRAY_SIZE = 512;
  static const int SAMPLE_RATE = 20000; // In Hz
  static const int THRESHOLD_ALPHA = 4; // Number of standard deviations below mean 
  static const int SPIKES_MAX_TIME_STORED = 20000;
  static const int PSTH_MAX_RANGE = 20000; //  Max range of psth calculations
  static const int PSTH_DEADZONE = 20; // time after a stimulus in which we don't include spikes to account for stimulation artifacts

  // Mutexes
  mutable std::mutex data_mutex;
  mutable std::mutex spike_mutex;
  mutable std::mutex psth_mutex;

  // Analysis
  bool online = true;
  bool psth = false;
  bool spike_sorting = false;
  bool find_avalanches = false; // TODO: Implement live avalanche analysis
  std::atomic<unsigned int> samples_parsed = 0;
  std::atomic<unsigned int> samples_analyzed = 0;
  std::deque<Spike> spikes;

  // Raw Data buffer
  std::deque<std::vector<short>*> data_ol; // Sample Number by Channel Number. 
    // Will expand and contract automatically as needed. Data will only be 
    // popped after it is analyzed.
  // Pointer to next raw data sample
  std::vector<short>* next_sample_ptr;
  unsigned int to_analyze = 0; // which element in data_ol is next to be analyzed.
    // Analysis will keep running, front popping data every time until to_analyze
    // is the same size as data_ol
  
  // Running Noise Method (online)
  static const int MAX_QUEUE_LENGTH = 20000;
  std::vector<long long> sum = std::vector<long long>(ELECTRODE_ARRAY_SIZE, 0);
  std::vector<long long> sumsq = std::vector<long long>(ELECTRODE_ARRAY_SIZE, 0);
  std::vector<float> mean = std::vector<float>(ELECTRODE_ARRAY_SIZE, 0.0f);
  //std::vector<float> noise = std::vector<float>(ELECTRODE_ARRAY_SIZE, 0.0f);
  std::vector<float> threshold_amp = std::vector<float>(ELECTRODE_ARRAY_SIZE, 0.0f);
  short median(std::vector<short> sample);
  void UpdateRunningNoise(std::vector<short>* sample);

  // Spike Finding (online)
  std::vector<unsigned int> sub_thresh_dur = std::vector<unsigned int>(ELECTRODE_ARRAY_SIZE, 0);
  std::vector<float> max_amp = std::vector<float>(ELECTRODE_ARRAY_SIZE, 0.0f);
  std::vector<unsigned int> max_amp_time = std::vector<unsigned int>(ELECTRODE_ARRAY_SIZE, 0);
  void SpikeDetectOL(std::vector<short>* sample);
  void AddSpike(int ch, unsigned int time, float amplitude);
  void ClearOldSpikes();

  // Stimulation Params and PSTH Analysis
  std::unique_ptr<Stim> stim;
  StimEvent* next_stim_event;
  bool stim_tracker_updated_this_frame = false;
  std::deque<StimEvent*> stim_event_tracker;
  std::map<int, std::vector<int>> psth_data; // maps stim ch to spike time
  // TODO: Map to spike time and spike channel to allow ability to see response on individual channels
  void UpdateStimTracker();
  void AddSpikeToPSTH(int ch, unsigned int time);

    /* POTENTIALLY FASTER/BETTER SPIKE FINDING METHOD (NOT IMPLEMENTED)
  * BASED ON WORK BY MATTHIAS HENNING IN SOFTWARE 'HERDING SPIKES 2'
  * SEE GITHUB: https://github.com/mhhennig/HS2
  // Baseline and Noise for finding Threshold (online)
  static const float BASELINE_INCREMENT;
  static const float VARIABILITY_INCREMENT;
  std::vector<float> baseline = std::vector<float>(ELECTRODE_ARRAY_SIZE, 0.0f);
  std::vector<float> variability = std::vector<float>(ELECTRODE_ARRAY_SIZE, 0.0f);
  void UpdateBaselineAndVariability();
  void IncreaseBaseline(int ch);
  void DecreaseBaseline(int ch);
  void IncreaseVariability(int ch);
  void DecreaseVariability(int ch);
  */


    /* OLD SPIKE FINDING METHOD. 
  * Only updated spikes once per second rather than live.
  std::vector<std::vector<short>> data; // Channel Number by Sample Number
  bool b_avg_calculated;
  std::vector<double> avg;
  std::vector<double> stdev;
  // Spike Finding Methods (updated once a second)
  int CalculateAverage();
  int CalculateStandardDeviation();
  int SpikeFinding();
  int ClearSpikeTimes();
  */

   /* DEBUG: Writing to file
  bool b_write_to_file = false;
  bool b_file_is_open = false;
  FILE* file;
  bool OpenFile();
  */
};