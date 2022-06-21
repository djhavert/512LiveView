#include "ImGuiWindowBase.h"

void ImGuiWindowBase::Init()
{
}

void ImGuiWindowBase::Create()
{
  ImGui::Begin("ParentWindow");
  ImGui::Text("Hello!");
  ImGui::End();
}

void ImGuiWindowBase::TextPushBack(const std::string text)
{
  std::lock_guard<std::mutex> guard(text_mutex);
  text_to_insert.push_back(text);
}

void ImGuiWindowBase::TextPopBack()
{
  std::lock_guard<std::mutex> guard(text_mutex);
  text_to_insert.pop_back();
}

void ImGuiWindowBase::DisplaySavedText()
{
  std::lock_guard<std::mutex> guard(text_mutex);
  for (auto it = text_to_insert.begin(); it != text_to_insert.end(); it++)
  {
    ImGui::Text(it->c_str());
  }
}
