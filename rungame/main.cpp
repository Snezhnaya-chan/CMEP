#include <string>
#include <iomanip>
#include <cmath>
#include <cassert>
#include <array>

#include <Windows.h>

#include "EngineCore.hpp"

#pragma comment(lib, "EngineCore.lib")

Engine::Engine* engine;
const char* windowTitle = "Test Game";
const unsigned int windowSizeX = 1200, windowSizeY = 720;

int main(int argc, char** argv)
{
	HANDLE myConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwMode = 0;
	GetConsoleMode(myConsole, &dwMode);
	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(myConsole, dwMode);
	//Engine::Scripting::LuaScript eventHandlers = ::Engine::Scripting::LuaScript("data/scripts/eventHandlers.lua");

	engine = Engine::initializeEngine(windowTitle, windowSizeX, windowSizeY);
	engine->ConfigFile("data/config.json");
	//engine->RegisterEventHandler(Engine::EventHandling::EventType::ON_INIT, onInit);
	//engine->RegisterLuaEventHandler(Engine::EventHandling::EventType::ON_INIT, &eventHandlers, "onInit");
	//engine->RegisterLuaEventHandler(Engine::EventHandling::EventType::ON_UPDATE, &eventHandlers, "onUpdate");
	//engine->RegisterLuaEventHandler(Engine::EventHandling::EventType::ON_KEYDOWN, &eventHandlers, "onKeyDown");
	//engine->RegisterLuaEventHandler(Engine::EventHandling::EventType::ON_MOUSEMOVED, &eventHandlers, "onMouseMoved");
	engine->Run();

	return 0;
}
