#pragma once
#include "ImGuiWindowBase.h"
class ImGuiWindowConnect :
    public ImGuiWindowBase
{
public:
  virtual void Create() override;

  inline void SetCallback(const std::function<void(Event& e)> callback) override { OnEvent = callback; };
protected:

};

