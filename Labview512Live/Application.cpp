#include "Application.h"

Application& Application::Get()
{
  static Application instance;
  return instance;
}

Application::Application()
  : window({ 854, 580 }), imgui_manager()
{
  bool success = Init();
  imgui_manager.SetCallback(std::bind(&Application::OnEvent, this, std::placeholders::_1));
}

Application::~Application()
{
}

bool Application::Init()
{
  ImGui_ImplGlfw_InitForOpenGL(window.GetWindowPtr(), true);
  imgui_manager.InitForOpenGL();

  data = std::make_unique<Data>();
  return true;
}

void Application::Run()
{
  /*  MAIN APPLICATION LOOP  */
  while (!window.ShouldClose())
  {
    window.PollEvents();
    imgui_manager.StartFrame();
    imgui_manager.CreateWindows();
    Render();
    Draw();
    window.SwapBuffers();
  }
  LOG("Shutdown Commenced");
  Shutdown();
}

void Application::SetRawData(int ch, int sample, short value)
{
  data->set(ch, sample, value);
}

void Application::AnalyzeData()
{
  data->AnalyzeData();
}

void Application::Render()
{
  imgui_manager.Render();
  glViewport(0, 0, window.GetWidth(), window.GetHeigth());
  glClear(GL_COLOR_BUFFER_BIT);
}

void Application::Draw()
{
  imgui_manager.Draw();
}

void Application::DataLoop()
{
  // Setup Network Connections to Labview. If they fail, don't start receiving data
  if (!SetupNetworkConnections())
  {
    return;
  }
  unsigned int receive_count = 0;
  while (!stop_receiving_on_client)
  {
    // RECEIVE DATA
    // Wait for Server Message
    int bytes_received = client->Receive();
    if (bytes_received > 0)
    {
      //LOG(bytes_received << " bytes received");
      receive_count++;
      // Parse Data
      client->ParseData();
    }
    else if (bytes_received == 0)
    {
      LOG("No bytes received.");
      continue;
    }
    else // Connection Broken. Abort
    {
      LOG("BREAK");
      break;
    }
    // ANALYZE DATA
    data->AnalyzeData();
  }
}

void Application::OnEvent(Event& e)
{
  if (e.GetEventType() == EventType::StartupRun)
  {
    StartupRunEvent& ev = dynamic_cast<StartupRunEvent&>(e);
    // Load stim file and all of it's events
    if (ev.StimEnabled())
    {
      stim_enabled = true;
      data->LoadStimFile(ev.GetStimFilename());
    }
    // Detach a thread to complete network setup, wait for Labview to start, 
    // then proceed with data collection and analysis.
    LOG("Starting Data Loop Thread");
    std::thread data_loop_thread(std::bind(&Application::DataLoop, this));
    data_loop_thread.detach();
  }
}

bool Application::SetupNetworkConnections()
{
  LOG("Network Setup Begin");
  // Initialize Winsock. Create Client Socket
  std::string display_message = "Initializing Winsock and Creating Client Socket...";
  imgui_manager.AddTextToWindow("connecting", display_message);
  client = std::make_unique<Client>();
  LOG("Create and Bind Server Socket");
  // Create and Bind Server Socket
  display_message = "Creating and Binding Server Socket...";
  imgui_manager.AddTextToWindow("connecting", display_message);
  server = std::make_unique<Server>(cmd_port);
  LOG("Wait for Connection");
  // Wait for LabviewClient To Connect to this program's Server
  display_message = "Network Initialization Complete. Waiting to Receive Connect Signal from LabView Client...";
  imgui_manager.AddTextToWindow("connecting", display_message);
  if (server->WaitForClientConnection() == -1)
  {
    return false;
  }
  display_message = "Connection Established with LabView Client.";
  imgui_manager.AddTextToWindow("connecting", display_message);

  // Connect this program's Client to Labview Server
  display_message = "Connecting to LabView Server, " + ipAddress + ":" + std::to_string(cmd_port) + " ...";
  imgui_manager.AddTextToWindow("connecting", display_message);
  client->Connect(ipAddress, data_port);
  display_message = "Connection Established with LabView Server.";
  imgui_manager.AddTextToWindow("connecting", display_message);

  // Send Confirm Message to Labview Client that Setup has Completed and we are ready for data
  char confirm_msg = '1';
  client->Send(&confirm_msg);
  LOG("confirmation sent");

  // Close out the Connection Window and Open Data Window
  imgui_manager.ConnectingComplete();

  return true;
}

void Application::Shutdown()
{
  imgui_manager.CloseAllWindows();
  if (server->IsWaiting())
  {
    server->SetStopWaiting(true);
  }
  stop_receiving_on_client = true;
  // Wait 2 seconds to ensure Client has time to disconnect from Labiew
  LOG("Waiting for Connections to be deestablished");
  std::this_thread::sleep_for(std::chrono::seconds(2));
  LOG("Finished Waiting");
}


