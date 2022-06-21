#pragma once
#include <Windows.h>
#include <string>

#include "Logger.h"

#include "glad\glad.h"
#define GLFW_INCLUDE_NONE
#include "GLFW\glfw3.h"



struct WindowParams
{
  unsigned int width;
  unsigned int height;
  std::string name;

  WindowParams(const unsigned int kWidth = 640,
               const unsigned int kHeight = 480,
               const std::string& kName = "LiveView512")
    : width(kWidth), height(kHeight), name(kName)
  {
  }
};

class Window
{
public:
  Window(const WindowParams& params = WindowParams::WindowParams());
  ~Window();

  inline unsigned int GetWidth() const { return window_params.width; }
  inline unsigned int GetHeigth() const { return window_params.height; }
  inline GLFWwindow* GetWindowPtr() const { return window; }

  bool ShouldClose();
  void SwapBuffers();
  void PollEvents();

  void OnTick();
  void OnUpdate();

private:
  GLFWwindow* window;
  bool b_is_initialized;

  WindowParams window_params;

  bool Init(const WindowParams& params);

  void Terminate();

  static void error_callback(int error, const char* description);
};

