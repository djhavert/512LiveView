#pragma once
#include <string>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <map>
#include <fstream>

#include <bitset>
#include "Logger.h"

struct StimHeader
{
  int window_num;
  int pulse_count;
  int command_count;
};

struct StimPulseFrame
{
  bool artifact_reduction;
  int frame;
  int channel;
  uint16_t pulse_code;
};

struct StimEvent
{
  int window_num;
  int frame;
  int channel;
  int pulseID;
  unsigned int GetTime()
  {
    return window_num * 5000 + frame;
  }
};

struct StimCommand
{
  int frame;
};

class Stim
{
public:
  Stim(std::string file);
  ~Stim();

  bool OpenAndReadFile();
  std::vector<int>& GetStimChannelsVec();
  StimEvent* GetNextStimTime();

private:
  /*   VARIABLES   */
  // Constants
  static const int HEADER_LENGTH_IN_BYTES = 3 * sizeof(uint32_t);
  static const int PULSE_LENGTH_IN_BYTES = 7 * sizeof(uint32_t);
  static const int COMMAND_LENGTH_IN_BYTES = 1001 * 4;
  
  // stim data
  std::vector<std::vector<int>> pulse_library;
  std::vector<StimEvent> event_sequence;
  std::vector<int> stim_channels;
  std::map<int, std::vector<int>> stim_times_for_channel;
  unsigned int next_stim_event;
  

  /*   VARIABLES/METHODS FOR READING STIM BINARY FILE   */
  // Variables
  std::vector<uint32_t> clk;
  std::string filename;
  // Methods
  bool ReadWholeFile(std::ifstream& stimfile);
  StimHeader ReadAndParseHeader(std::ifstream& stimfile);
  void ReadPulses(std::ifstream& stimfile, const StimHeader& header);
  void ParsePulses(const std::vector<std::vector<uint32_t>>& pulses, const StimHeader& header);
  uint16_t ParsePulseCode(const std::vector<uint32_t>& pulse_code_segments);
  void ReadCommands(std::ifstream& stimfile, const StimHeader& header);
  std::vector<int> CreatePulse(const std::vector<StimPulseFrame>& pulse_frames, const int start_index, const int length);
  int GetPulseLibraryIndex(const std::vector<int>& pulse);
  bool LoadClk();
  uint32_t CastChar2Uint32(char* char_ptr);
  int Indicator2Channel(int indicator);
};
