#include "Stim.h"

Stim::Stim(std::string file)
  : filename(file)
{
  LoadClk();
}

Stim::~Stim()
{
}

bool Stim::OpenAndReadFile()
{
  std::ifstream stimfile(filename, std::ios::in | std::ios::binary);
  if (stimfile)
  {
    ReadWholeFile(stimfile);
    return true;
  }
  else
  {
    // Failed to open file
    std::ostringstream error;
    error << "Failed to Open Stim File " << filename;
    throw std::runtime_error(error.str());
    return false;
  }
}

std::vector<int>& Stim::GetStimChannelsVec()
{
  return stim_channels;
}

StimEvent* Stim::GetNextStimTime()
{
  return &event_sequence[next_stim_event];
  next_stim_event++;
}

bool Stim::ReadWholeFile(std::ifstream& stimfile)
{
  while (stimfile.peek() != EOF)
  {
    // Read Header
    StimHeader header = ReadAndParseHeader(stimfile);
    
    // Read Pulses
    ReadPulses(stimfile, header);

    // Read Commands
    ReadCommands(stimfile, header);
  }
  return true;
}

StimHeader Stim::ReadAndParseHeader(std::ifstream& stimfile)
{
  StimHeader header{ 0, 0, 0 };
  char buffer[HEADER_LENGTH_IN_BYTES];
  if (stimfile.read(&buffer[0], HEADER_LENGTH_IN_BYTES * sizeof(char)))
  {
    // Read Header
    header.window_num = static_cast<int>(CastChar2Uint32(&buffer[0]));
    header.pulse_count = static_cast<int>(CastChar2Uint32(&buffer[4]));
    header.command_count = static_cast<int>(CastChar2Uint32(&buffer[8]));
  }
  else
  {
    // Failed to read from filestream
    std::ostringstream error;
    error << "Failed to Read from Stim File " << filename;
    throw std::runtime_error(error.str());
  }
  return header;
}

void Stim::ReadPulses(std::ifstream& stimfile, const StimHeader& header)
{
  /* Read entire pulse buffer from current window into vector */
  std::vector<std::vector<uint32_t>> pulses;
  for (int frame = 0; frame < header.pulse_count; frame++) // For each pulse frame
  {
    char buffer[PULSE_LENGTH_IN_BYTES];
    std::vector<uint32_t> single_frame_pulse;
    if (stimfile.read(&buffer[0], PULSE_LENGTH_IN_BYTES * sizeof(char)))
    {
      for (int i = 0; i < 7; i++)
      {
        single_frame_pulse.push_back(CastChar2Uint32(&buffer[i*4]));
      }
      if (single_frame_pulse != std::vector<uint32_t>(7, 0))
      {
        pulses.push_back(single_frame_pulse);
      }
    }
    else
    {
      // Failed to read from filestream
      std::ostringstream error;
      error << "Failed to Read from Stim File " << filename;
      throw std::runtime_error(error.str());
    }
  } // End of reading from file

  // Parse Pulse Data
  ParsePulses(pulses, header);
}

void Stim::ParsePulses(const std::vector<std::vector<uint32_t>>& pulses, const StimHeader& header)
{
  std::vector<StimPulseFrame> pulse_frames_in_window;
  // For each frame of all the pulses in the current window,
  // Parse the length 7 pulse code
  for (auto it = pulses.begin(); it < pulses.end(); it++) 
  {
    // Get info from first element of this pulse
    uint32_t pulse0 = it->at(0);
    bool artifact_reduction = pulse0 / 10000000;
    int frame = pulse0 % 10000000 / 1000;
    bool is_even_chip = false;
    int indicator = pulse0 % 10000000 % 1000 + 1;
    if (indicator > 426)
      is_even_chip = true;
    int channel_rel = Indicator2Channel(indicator);
    int channel = 0;
    
    // Read through other 6 elements to get pulse code and channel
    std::vector<int> chip_vec;
    std::vector<uint32_t> pulse_code_segments;
    for (int i = 1; i < 7; i++)
    {
      // get pulse code and remove clock signal from it.
      uint32_t pulsei = it->at(i);
      uint32_t A = pulsei - clk[indicator + i - 2];
      // get rid of hold signal on first part of pulse code
      if (i == 1 || i == 2) 
      {
        A = A - 2236928;
      }
      // Determine which chip the pulse is on and adjust the pulse code as needed
      int chip;
      if (A > (1048576 - 1)) // chips 1 and 2 are larger than 2^20
      {
        A = A / 1048576;
        if (is_even_chip)
          chip = 2;
        else
          chip = 1;
      }
      else if (A > (65536 - 1)) // chips 7 and 8 are larger than 2^16
      {
        A = A / 65536;
        if (is_even_chip)
          chip = 8;
        else
          chip = 7;
      }
      else if (A > (4096 - 1)) // chips 5 and 6 are larger than 2^12
      {
        A = A / 4096;
        if (is_even_chip)
          chip = 6;
        else
          chip = 5;
      }
      else if (A > (256 - 1)) // chips 3 and 4 are larger than 2^8
      {
        A = A / 256;
        if (is_even_chip)
          chip = 4;
        else
          chip = 3;
      }
      else if (A == 0)
      {
        chip = chip_vec.back();
      }
      else
      {
        // DEBUG error
        chip = 0;
        LOG("ERROR: Chip value calculation doesn't match predifined structure.");
        LOG("pulseID = " << pulsei);
        LOG("Converted = " << A);
      }
      // DEBUG make sure chip is same for each of 6 pulse codes
      chip_vec.push_back(chip);
      if (chip_vec.back() != chip_vec.front())
      {
        LOG("ERROR: Chip value changed in the middle of a pulse.");
      }
      // DEBUG make sure ending pulse code is only 4 bits
      if (A > 16)
      {
        LOG("ERROR: Pulse code segment for frame " << frame << " is not valid.");
        LOG("Pulse Code: " << A);
      }
      pulse_code_segments.push_back(A);
      // calculate full channel number from relative channel and chip
      channel = (chip - 1) * 64 + channel_rel;
    } // end of single frame loop reading;

    // Calculate the pulse code from the 6 pulse code segments
    uint16_t pulsecode = ParsePulseCode(pulse_code_segments);
    // Add pulse frame to this pulse sequence.
    pulse_frames_in_window.push_back({ artifact_reduction, frame, channel, pulsecode });
  } // end of loop through all pulses

  // Now loop through the pulse frames again to determine where each pulse starts and ends
  std::vector<int> pulse_start_indices;
  int pulse_start_prev = 0;
  for (std::size_t i = 0; i < pulse_frames_in_window.size(); i++)
  {
    bool end_of_previous_pulse = false;
    // Three ways to tell if it's a new pulse
    // 1) It's the first pulse (trivial)
    if (i == 0)
    {
      end_of_previous_pulse = false;
    }
    // 2) the channel number changes
    else if (pulse_frames_in_window[i].channel != pulse_frames_in_window[i - 1].channel)
    {
      end_of_previous_pulse = true;
    }
    // 3) the frame incremented by a value different than 1
    else if (pulse_frames_in_window[i].frame != pulse_frames_in_window[i - 1].frame + 1)
    {
      end_of_previous_pulse = true;
    }
    
    // If previous pulse has ended, add the pulse and create an event.
    if (end_of_previous_pulse)
    {
      int pulse_length = i - pulse_start_prev;
      // Create vector of pulse from pulse codes
      std::vector<int> prev_pulse = CreatePulse(pulse_frames_in_window, pulse_start_prev, pulse_length);
      // Find pulse index. If pulse doesn't already exist, this function will add it to the pulse library
      int pulse_index = GetPulseLibraryIndex(prev_pulse);
      // Add event to event vector. window number (the 0 below) is a placeholder.
      int frame_temp = pulse_frames_in_window[pulse_start_prev].frame;
      int channel_temp = pulse_frames_in_window[pulse_start_prev].channel;
      event_sequence.push_back({ header.window_num, frame_temp, channel_temp, pulse_index });
      //LOG(header.window_num << " " << pulse_frames_in_window[pulse_start_prev].frame << " " << pulse_frames_in_window[pulse_start_prev].channel << " " << pulse_index);
      // If channel doesn't already exist in map, add it
      if (stim_times_for_channel.find(channel_temp) == stim_times_for_channel.end())
      {
        stim_channels.push_back(channel_temp);
        stim_times_for_channel.insert(std::make_pair(channel_temp, std::vector<int>()));
      }
      stim_times_for_channel[channel_temp].push_back(frame_temp * header.window_num * 5000);
      // reset for next loop
      pulse_start_prev = i;
    }

    // if we reach the end of pulse vector, add last pulse and create and event.
    if (i == pulse_frames_in_window.size() - 1)
    {
      int pulse_length = i + 1 - pulse_start_prev;
      // Create vector of pulse from pulse codes
      std::vector<int> last_pulse = CreatePulse(pulse_frames_in_window, pulse_start_prev, pulse_length);
      // Find pulse index. If pulse doesn't already exist, this function will add it to the pulse library
      int pulse_index = GetPulseLibraryIndex(last_pulse);
      // Add event to event vector. window number (the 0 below) is a placeholder.
      event_sequence.push_back({ header.window_num, pulse_frames_in_window[pulse_start_prev].frame, pulse_frames_in_window[pulse_start_prev].channel, pulse_index });
      //LOG(header.window_num << " " << pulse_frames_in_window[pulse_start_prev].frame << " " << pulse_frames_in_window[pulse_start_prev].channel << " " << pulse_index);
    }
  }
}

uint16_t Stim::ParsePulseCode(const std::vector<uint32_t>& pulse_code_segments)
{
  // Full pulse code is 12 bits imprinted on 16 bit int
  // bits 13-16 are 0
  // bits 1-8 determine amplitude
  // bits 9-12 determine hardware switches
  uint16_t pulse_code = 0;
  for (std::size_t i = 0; i < pulse_code_segments.size(); i+=2)
  {
    uint16_t temp;
    switch (i)
    {
    case 0: // bits 9-12
      temp = pulse_code_segments[i] << 8;
      break;
    case 2: // bits 5-8
      temp = pulse_code_segments[i] << 4;
      break;
    case 4: // bits 1-4
      temp = pulse_code_segments[i];
      break;
    }
    pulse_code = pulse_code + temp;
  }
  return pulse_code;
}

void Stim::ReadCommands(std::ifstream& stimfile, const StimHeader& header)
{
  for (int i = 0; i < header.command_count; i++)
  {
    stimfile.ignore(COMMAND_LENGTH_IN_BYTES);
  }
}

std::vector<int> Stim::CreatePulse(const std::vector<StimPulseFrame>& pulse_frames, int start_index, int length)
{
  std::vector<int> pulse;
  for (int i = start_index; i < start_index + length; i++)
  {
    pulse.push_back(pulse_frames[i].pulse_code);
  }
  return pulse;
}

int Stim::GetPulseLibraryIndex(const std::vector<int>& pulse)
{
  int pulse_index = -1;
  for (std::size_t i = 0; i < pulse_library.size(); i++)
  {
    if (pulse == pulse_library[i])
    {
      pulse_index = i;
    }
  }
  // Pulse doesn't already exist, so add it to the pulse_library;
  if (pulse_index == -1)
  {
    pulse_library.push_back(pulse);
    pulse_index = pulse_library.size() - 1;
    /*
    for (int i = 0; i < pulse.size(); i++)
    {
      LOG(std::bitset<16>(pulse[i]));
    }
    */
  }
  return pulse_index;
}

bool Stim::LoadClk()
{
  std::ifstream clkfile("data\\clk.bin", std::ios::in | std::ios::binary);
  char buffer[sizeof(uint32_t)];
  LOG("uint32_t size: " << sizeof(uint32_t));
  if (clkfile)
  {
    for (int i = 0; i < 1000; i++)
    {
      clkfile.read(&buffer[0], sizeof(uint32_t));
      clk.push_back(CastChar2Uint32(&buffer[0]));
    }
    return true;
  }
  else
  {
    // Failed to open file
    std::ostringstream error;
    error << "Failed to Open Clk.bin File " << "data\\clk.bin";
    throw std::runtime_error(error.str());
    return false;
  }
}

uint32_t Stim::CastChar2Uint32(char* char_ptr)
{
  uint32_t val = (uint8_t)char_ptr[0] + ((uint8_t)char_ptr[1] << 8) + ((uint8_t)char_ptr[2] << 16) + ((uint8_t)char_ptr[3] << 24);
  //uint32_t val = (char_ptr[0] << 24) + (char_ptr[1] << 16) + (char_ptr[2] << 8) + char_ptr[3];
  return val;
}

int Stim::Indicator2Channel(int indicator)
{
  if (indicator > 426)
    indicator = indicator - 384;
  indicator = (indicator - 37) / 6;
  return indicator;
}
