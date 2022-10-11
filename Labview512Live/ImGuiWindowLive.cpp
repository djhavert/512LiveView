#include "ImGuiWindowLive.h"

#include "Application.h"

void ImGuiWindowLive::Init()
{
  LOG("Loading Electrode Map");
  LoadElMap();
  if (Application::Get().StimEnabled())
  {
    LOG("PSTH will display");
    psth_enabled = true;
    UpdatePsthChannnelList();
  }
  LOG("Spike Summary Initialized");
}

void ImGuiWindowLive::Create()
{
  ImGui::Begin("LiveView");
  //OscilliscopeDraw();
  //SpikeSummaryDraw();
  PsthDraw();
  ImGui::End();
}

void ImGuiWindowLive::LoadElMap()
{
  std::ifstream mapfile(el_map_filename, std::ios::in);
  if (mapfile)
  {
    el_map_x.resize(512, 0.0f);
    el_map_y.resize(512, 0.0f);
    std::string line;
    char temp[20];
    while (std::getline(mapfile, line))
    {
      std::istringstream iline(line);
      iline.getline(temp, 20, ':');
      int ch = std::atoi(temp);
      iline.getline(temp, 20, '[');
      iline.getline(temp, 20, ',');
      el_map_x[ch] = (float)std::atoi(temp);
      iline.getline(temp, 20, ']');
      el_map_y[ch] = (float)std::atoi(temp);
      //LOG(el_map_x[ch] << ' ' << el_map_y[ch]);
    }
  }
  else
  {
  // Failed to open file
  std::ostringstream error;
  error << "Failed to Open Clk.bin File " << "data\\clk.bin";
  throw std::runtime_error(error.str());
  }
}

void ImGuiWindowLive::OscilliscopeDraw()
{
  //ImGui::Begin("Oscilliscope");
  if (ImGui::BeginListBox("Channel"))
  {
    for (int n = 0; n < 512; n++)
    {
      const bool is_selected = (oscill_channel_idx == n);
      if (ImGui::Selectable(std::to_string(n+1).c_str(), is_selected))
        oscill_channel_idx = n;

      // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
      if (is_selected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndListBox();
  }

  if (ImPlot::BeginPlot("Oscilliscope"))
  {
    ImPlot::SetupAxesLimits(-20000, 0, -500, -100);
    //LOG("GET DATA");
    std::vector<int> data = Application::Get().GetDataPtr()->GetRawData(oscill_channel_idx);
    int n = data.size();
    std::vector<int> x(n);
    std::iota(std::begin(x), std::end(x), -n);
    //LOG("PLOT DATA");
    ImPlot::PlotLine("data", x.data(), data.data(), n);

    //LOG("GET NOISE");
    std::pair<float, float> noise_data = Application::Get().GetDataPtr()->GetMeanAndThresh(oscill_channel_idx);
    float mean = noise_data.first;
    float std = noise_data.second;
    float noise[] = { mean, mean - std};
    ImVec4 col = { 0.11f, .86f, .90f, .7f };
    //LOG("PLOT NOISE");
    ImPlot::SetNextLineStyle(col);
    ImPlot::PlotHLines("mean and threshold", noise, 2);
    //LOG("END PLOT");
    ImPlot::EndPlot();
  }
}

void ImGuiWindowLive::SpikeSummaryDraw()
{
  //ImGui::Begin("Spike Summary");

  /* Plot the Array with Electrode Numbers */
  if (ImPlot::BeginPlot("Spikes",ImVec2(-1, -1),ImPlotFlags_Equal))
  {
    ImPlot::SetupAxesLimits(-1000, 1000, -500, 500);
    /* Plot the array with Electrode Numbers */
    ImPlot::PushStyleVar(ImPlotStyleVar_PlotDefaultSize, ImVec2(1600,800));
    ImPlot::PushStyleVar(ImPlotStyleVar_FitPadding, ImVec2(0.05f, 0.10f));
    ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
    ImPlot::SetNextMarkerStyle(ImPlotMarker_Square, 6, ImPlot::GetColormapColor(1), IMPLOT_AUTO, ImPlot::GetColormapColor(1));
    ImPlot::PlotScatter("Data 1", el_map_x.data(), el_map_y.data(), el_map_x.size());
    ImPlot::PopStyleVar();
    ImPlot::PopStyleVar();
    ImPlot::PopStyleVar();
    for (size_t i = 0; i < el_map_x.size(); i++)
    {
      ImPlot::Annotation(el_map_x[i], el_map_y[i], ImVec4(0,0,0,0), ImVec2(5, 5), true, std::to_string(i+1).c_str());
    }

    /* Plot the spikes*/
    // Plot any spike within last __ milliseconds. 
    // Amplitude given by size of circle up to max.
    // Circle fades with time
    std::deque<Spike> spikes = Application::Get().GetDataPtr()->GetSpikes();
    //LOG(spikes.size() << "spikes in current frame");
    unsigned int current_time = Application::Get().GetDataPtr()->GetSamplesAnalyzed();
    for (auto it = spikes.begin(); it != spikes.end(); it++)
    {
      unsigned int deltaT = current_time - it->time;
      float alpha = 1 - (float)deltaT/20000;
      ImVec4 col = { 0, 1, 0, 1 };
      ImVec4 bordercol = { 0, 0, 0, 0 };
      float rel_ampl = it->amplitude / 60;
      if (rel_ampl > 1) rel_ampl = 1;
      ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, rel_ampl * 10.0f, col, 1, bordercol);
      ImPlot::SetNextFillStyle(col, alpha);
      int ch = it->channel;
      ImPlot::PlotScatter("Spikes", &el_map_x[ch], &el_map_y[ch], 1);
    }

    ImPlot::EndPlot();
  }
  

  //ImGui::End();
}

void ImGuiWindowLive::UpdatePsthChannnelList()
{
  LOG("Update psth channel list");
  std::vector<int> stim_channels = Application::Get().GetDataPtr()->GetStimChannelsVec();
  for (auto it = stim_channels.begin(); it != stim_channels.end(); it++)
  {
    psth_channel_list_int.push_back(*it);
    psth_channel_list_string.push_back(std::to_string(*it));
  }
  LOG("Psth channel list updated");
}

void ImGuiWindowLive::PsthDraw()
{
  if (!psth_enabled)
  {
    return;
  }
  /* Box to Select which Stimulated Channel's PSTH to show */
  //ImGui::Begin("PSTH");
  //LOG("Begin PSTH Stim Channels ListBox");
  if (ImGui::BeginListBox("Stimulated Channel"))
  {
    for (size_t n = 0; n < psth_channel_list_string.size(); n++)
    {
      const bool is_selected = (psth_channel_idx == n);
      if (ImGui::Selectable(psth_channel_list_string[n].c_str(), is_selected))
        psth_channel_idx = n;

      // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
      if (is_selected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndListBox();
  }

  /* Plot the PSTH */
  //LOG("PSTH PLOT");
  if (ImPlot::BeginPlot("PSTH")) {
    ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
    std::vector<int> psth_data = Application::Get().GetDataPtr()->GetPSTHData(psth_channel_list_int[psth_channel_idx]);
    if (psth_data.begin() != psth_data.end())
    {
      //LOG("Plot");
      ImPlot::PlotHistogram("Empirical", &psth_data[0], psth_data.size(), bins, cumulative, density, ImPlotRange(), outliers);
    }
    ImPlot::EndPlot();
  }

  //
  //LOG("PSTH plotted");
}

