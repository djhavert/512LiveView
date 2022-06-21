#include <iostream>
#include <stdexcept>
#include <limits>

#define _WINSOCKAPI_
#include "Application.h"
#include "Logger.h"

static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

int main()
{
	try {
		Application& app = Application::Get();
		app.Run();
	}
	catch (const std::runtime_error &e) 
	{
		std::cerr << e.what() << std::endl;
		std::cout << "Program has finished. Enter any key to exit." << std::endl;
		std::cin.get();
		return 0;
	}
	catch (...)
	{
		std::cout << "Unknown Error" << std::endl;
		return -999;
	}

	// Pause until user termination
	std::cout << "Program has finished. Press enter to exit." << '\n';
	std::cin.get();
	return 0;
}
