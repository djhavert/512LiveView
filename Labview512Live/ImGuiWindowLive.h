#pragma once
#include "ImGuiWindowBase.h"

#include "implot.h"
#include <fstream>
#include <sstream>

//#include "Application.h"

class ImGuiWindowLive :
    public ImGuiWindowBase
{
public:
  virtual void Init() override;
  virtual void Create() override;
  inline void SetCallback(const std::function<void(Event& e)> callback) override { OnEvent = callback; };

protected:
  /* Spike ARRAY VIEWER - VIEW OF ENTIRE ARRAY */
  void LoadElMap();
  std::string el_map_filename = "data\\512map.txt";
  std::vector<float> el_map_x;
  std::vector<float> el_map_y;
  void SpikeSummaryDraw();

  /* OSCILLISCOPE */
  int oscill_channel_idx = 1;
  void OscilliscopeDraw();

  /* PSTH */
  void UpdatePsthChannnelList();
  bool psth_enabled = false;
  std::vector<std::string> psth_channel_list_string;
  std::vector<int> psth_channel_list_int;
  int psth_channel_idx;
  int  bins = 50;
  bool cumulative = false;
  bool density = true;
  bool outliers = true;
  void PsthDraw();

};

