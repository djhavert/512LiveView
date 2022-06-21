#include "ImGuiWindowConnect.h"

void ImGuiWindowConnect::Create()
{
  ImGui::Begin("Connect");
  //ImGui::Text("Setting up LabView Connection");
  DisplaySavedText();
  ImGui::End();
}
