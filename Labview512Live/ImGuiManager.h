#pragma once
#define _WINSOCKAPI_
#include <map>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"

#include "Event.h"
#include "ImGuiWindowBase.h"
#include "ImGuiWindowStartup.h"
#include "ImGuiWindowConnect.h"
#include "ImguiWindowLive.h"

class ImGuiManager
{
public:
  ImGuiManager();
  ~ImGuiManager();

  bool Init();
  bool InitForOpenGL();

  void StartFrame();
  void CreateWindows();
  void Render();
  void Draw();

  void DisplayStartup(bool b);
  void DisplayConnecting(bool b);
  void DisplayLive(bool b);
  
  void CloseAllWindows();
  void Cleanup();

  void AddTextToWindow(const std::string window_name, const std::string text);
  void ConnectingComplete();

  inline void SetCallback(const std::function<void(Event& e)> callback) { OnEvent = callback; };

private:
  ImGuiIO* imgui_io;
  std::map<std::string,std::unique_ptr<ImGuiWindowBase>> imgui_windows;
  //ImGuiWindowStartup startup;
  //ImGuiWindowConnect connecting;
  //ImGuiWindowLive receiving;

  void StartupRunPressed(Event& e);
  
  std::function<void(Event& e)> OnEvent;
};

