#include "ImGuiManager.h"

ImGuiManager::ImGuiManager()
	: imgui_io(nullptr)
{
	// Initialize ImGui
	bool success = Init();

	// Initialize imgui_windows
	imgui_windows.insert(std::make_pair("startup", std::make_unique<ImGuiWindowStartup>()));
	imgui_windows.insert(std::make_pair("connecting", std::make_unique<ImGuiWindowConnect>()));
	imgui_windows.insert(std::make_pair("live", std::make_unique<ImGuiWindowLive>()));

	// Startup window should display
	imgui_windows.at("startup")->SetShouldDisplay(true);
	imgui_windows.at("connecting")->SetShouldDisplay(false);
	imgui_windows.at("live")->SetShouldDisplay(false);

	// Bind functions to Event Callbacks
	imgui_windows.at("startup")->SetCallback(std::bind(&ImGuiManager::StartupRunPressed, this, std::placeholders::_1));
	
}

ImGuiManager::~ImGuiManager()
{
	Cleanup();
}

bool ImGuiManager::Init()
{
	// Initialize ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();
	imgui_io = &(ImGui::GetIO()); //(void)io;
	ImGui::StyleColorsDark();
	//ImGui_ImplGlfw_InitForOpenGL(window, true);

  return true;
}

bool ImGuiManager::InitForOpenGL()
{
	ImGui_ImplOpenGL3_Init("#version 330");
	return true;
}

void ImGuiManager::StartFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void ImGuiManager::CreateWindows()
{
	// for each window
	for (auto it = imgui_windows.begin(); it != imgui_windows.end(); it++)
	{
		if (it->second->ShouldDisplay())
		{
			it->second->Create(); // Create window
		}
	}
}

void ImGuiManager::Render()
{
	ImGui::Render();
}

void ImGuiManager::Draw()
{
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiManager::DisplayStartup(bool b)
{
	imgui_windows.at("startup")->SetShouldDisplay(b);
}

void ImGuiManager::DisplayConnecting(bool b)
{
	imgui_windows.at("connecting")->SetShouldDisplay(b);
}

void ImGuiManager::DisplayLive(bool b)
{
	imgui_windows.at("live")->SetShouldDisplay(b);
	if (b)
	{
		imgui_windows.at("live")->Init();
	}
	
}

void ImGuiManager::CloseAllWindows()
{
	DisplayStartup(false);
	DisplayConnecting(false);
	DisplayLive(false);
}

void ImGuiManager::Cleanup()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImPlot::DestroyContext();
	ImGui::DestroyContext();
}

void ImGuiManager::AddTextToWindow(const std::string window_name, const std::string text)
{
	try
	{
		imgui_windows.at(window_name)->TextPushBack(text);
	}
	catch (const std::exception e)
	{
		std::cerr << "Failed to access window in function 'TextToWindow' in 'ImGuiManager.h" << std::endl;
		std::cerr << e.what() << std::endl;
	}
	
}

void ImGuiManager::StartupRunPressed(Event& e)
{
	imgui_windows.at("startup")->SetShouldDisplay(false);
	//imgui_windows.insert(std::make_pair("connecting", std::make_unique<ImGuiWindowConnect>()));
	imgui_windows.at("connecting")->SetShouldDisplay(true);
	OnEvent(e);
}

void ImGuiManager::ConnectingComplete()
{
	//imgui_windows.erase("connecting");
	imgui_windows.at("connecting")->SetShouldDisplay(false);

	//imgui_windows.insert(std::make_pair("Live",std::make_unique<ImGuiWindowLive>()));
	imgui_windows.at("live")->SetShouldDisplay(true);
	imgui_windows.at("live")->Init();
	//ConnectingCompleteEvent e("StartupRun");
	//OnEvent(e);


}

