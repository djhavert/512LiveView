#pragma once
#include <string>
#include <functional>
#include <vector>
#include <mutex>

#include "imgui.h"

#include "Logger.h"
#include "Event.h"

class ImGuiWindowBase
{
public:
  virtual void Init();
  virtual void Create();

  bool ShouldDisplay() { return b_should_display; } const

  inline void SetShouldDisplay(bool b) { b_should_display = b; };
  void TextPushBack(const std::string text);
  void TextPopBack();

  void DisplaySavedText();

  inline virtual void SetCallback(const std::function<void(Event& e)> callback) {};
protected:
  bool b_should_display = false;
  std::vector<std::string> text_to_insert;
  std::mutex text_mutex;

  std::function<void(Event& e)> OnEvent;
};

