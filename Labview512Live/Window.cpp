#include "Window.h"

Window::Window(const WindowParams& params)
{
  bool success = Init(params);
}

Window::~Window()
{
  glfwDestroyWindow(window);
  Terminate();
}

bool Window::ShouldClose()
{
  return glfwWindowShouldClose(window);
}

void Window::SwapBuffers()
{
  glfwSwapBuffers(window);
}

void Window::PollEvents()
{
  glfwPollEvents();
}

void Window::OnTick()
{
}

void Window::OnUpdate()
{
}

bool Window::Init(const WindowParams& params)
{
  glfwSetErrorCallback(error_callback);

  if (!b_is_initialized)
  {
    bool success = glfwInit();
    if (!success)
    {
      LOG("GLFW Failed to Initialize");
      return false;
    }
    b_is_initialized = true;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

  window = glfwCreateWindow(params.width, params.height, params.name.c_str(), NULL, NULL);
  if (!window)
  {
    LOG("Window or OpenGL context creation failed");
    return false;
  }

  glfwMakeContextCurrent(window);

  gladLoadGL();
  //gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

  glfwSwapInterval(1);

  window_params = params;
  glfwSetWindowUserPointer(window, (void*)&window_params);

  return true;
}

void Window::Terminate()
{
  glfwTerminate();
}

void Window::error_callback(int error, const char* description)
{
  LOG("Error: " << description);
}
