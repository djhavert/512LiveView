#pragma once
#include "imgui_impl_glfw.h"
#define _WINSOCKAPI_
#include "Window.h"

#include "ImGuiManager.h"
#include "Event.h"
#include "Client.h"
#include "Server.h"
#include "Data.h"

#include <thread>
#include <chrono>

//class Client;
//class ImGuiManager;

// Singleton Class
class Application
{
public:
  static Application& Get();
private:
  Application();
  ~Application();
public:
  Application(Application const&) = delete;
  void operator=(Application const&) = delete;
public:
  void Run();

  inline const Data* GetDataPtr() const { return data.get(); };
  inline bool StimEnabled() const { return stim_enabled; };

  void SetRawData(int ch, int sample, short value);
  void AnalyzeData();
private:
  Window window;
  ImGuiManager imgui_manager;
  std::unique_ptr<Client> client;
  std::unique_ptr<Server> server;
  //std::unique_ptr<Stim> stim;
  std::unique_ptr<Data> data;
  //Data data;
  std::atomic<bool> stop_receiving_on_client = false;
  bool stim_enabled = false;

  std::string ipAddress = "192.168.1.1";			// IP Address of the server
  int data_port = 7887;						// Listening port # on the server
  int cmd_port = 9876;

  bool Init();
  void Render();
  void Draw();
  void DataLoop();

  void OnEvent(Event& e);
  bool SetupNetworkConnections();
  void Shutdown();
};

