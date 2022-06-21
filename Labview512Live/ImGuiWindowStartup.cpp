#include "ImGuiWindowStartup.h"

void ImGuiWindowStartup::Create()
{
	ImGui::Begin("LiveView512Startup");

	// Stim checkbox
	ImGui::Checkbox("Stimulation?", &b_stim);
	if (b_stim)
	{
		ImGui::Text("Input Path to Stimulation Binary File:");
		ImGui::InputText(" ", &stim_filename);
		ImGui::SameLine();
		if (ImGui::Button("Browse"))
		{
			OpenFileDialog();
		}
	}

	// Run Button
	if (ImGui::Button("Run"))
	{
		b_run = true;
		StartupRunEvent e(b_stim, stim_filename);
		OnEvent(e);
	}
	ImGui::End();
}

bool ImGuiWindowStartup::OpenFileDialog()
{
	OPENFILENAMEA ofn = { 0 };
	CHAR szFile[260] = { 0 };
	// Initialize remaining fields of OPENFILENAME structure
	ofn.lStructSize = sizeof(ofn);
	//ofn.hwndOwner = hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
	if (GetOpenFileNameA(&ofn) == TRUE)
	{
		stim_filename = ofn.lpstrFile;
		return true;
	}
	else
	{
		return false;
	}
}
