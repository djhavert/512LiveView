#pragma once
#include <Windows.h>
#include <string>
#include <tchar.h>
#include "commdlg.h"

#include "ImGuiWindowBase.h"

#include "imgui_stdlib.h"


class ImGuiWindowStartup :
    public ImGuiWindowBase
{
public:
  virtual void Create() override;
  inline void SetCallback(const std::function<void(Event& e)> callback) override { OnEvent = callback; };
  inline std::string GetStimFilename() { return stim_filename; };
protected:
  bool b_run = false;
  bool b_stim = false;

  std::string stim_filename;
  bool OpenFileDialog();

};

