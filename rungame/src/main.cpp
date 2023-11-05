#include <string>
#include <iomanip>
#include <cmath>
#include <cassert>
#include <array>

#if defined(_MSC_VER)
	#include <Windows.h>
#endif

#include "EngineCore.hpp"

Engine::Engine* engine;
const char* windowTitle = "";
const unsigned int windowSizeX = 1200, windowSizeY = 720;

int main(int argc, char** argv)
{
#if defined(_MSC_VER)
	HANDLE myConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwMode = 0;
	GetConsoleMode(myConsole, &dwMode);
	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(myConsole, dwMode);
#endif
	engine = Engine::initializeEngine(windowTitle, windowSizeX, windowSizeY);
	
	try
	{
		engine->ConfigFile("game/config.json");
	}
	catch(std::exception e)
	{
		printf("COULD NOT LOAD CONFIG.JSON! QUITTING");
		exit(-1);
	}
	
	engine->Run();

	return 0;
}