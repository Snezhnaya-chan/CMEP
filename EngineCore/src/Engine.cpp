#include <assert.h>
#include <fstream>
#include <memory>
#include <thread>
#include <chrono>

#include "nlohmann-json/single_include/nlohmann/json.hpp"

#include "glm/glm.hpp"

#include "Rendering/AxisRenderer.hpp"

#include "Scripting/LuaScriptExecutor.hpp"
#include "Scripting/LuaScript.hpp"

#include "ObjectFactory.hpp"
#include "AssetManager.hpp"
#include "Engine.hpp"
#include "Object.hpp"

#if defined(_MSC_VER)
#include <Windows.h>
#endif

#if defined(_MSC_VER)
	#ifndef _DEBUG
		#define _DEBUG 0
	#endif
#endif

namespace Engine
{
	bool EngineIsWindowInFocus = false;
	bool EngineIsWindowInContent = false;
	double EngineMouseXPos = 0.0;
	double EngineMouseYPos = 0.0;

	// Utility sleep function
	void Engine::spinSleep(double seconds)
	{
		static double estimate = 5e-3;
		static double mean = 5e-3;
		static double m2 = 0;
		static int64_t count = 1;

		while (seconds > estimate) {
			const auto start = std::chrono::steady_clock::now();
			std::this_thread::sleep_for(std::chrono::milliseconds(2));
			const auto end = std::chrono::steady_clock::now();

			const double observed = (end - start).count() / 1e9;
			seconds -= observed;

			count++;
			const double delta = observed - mean;
			mean += delta / count;
			m2 += delta * (observed - mean);
			const double stddev = sqrt(m2 / (count - 1));
			estimate = mean + stddev;
		}

		// spin lock
		const auto start = std::chrono::steady_clock::now();
		while ((std::chrono::steady_clock::now() - start).count() / 1e9 < seconds) {};
	}

	void Engine::handleInput(const double deltaTime) noexcept
	{
		Rendering::GLFWwindowData windowdata = this->rendering_engine->GetWindow();

		double xpos = 0.0, ypos = 0.0;
		static double lastx = (windowdata.windowX / 2.0), lasty = (windowdata.windowY / 2.0);

		if (EngineIsWindowInFocus && EngineIsWindowInContent)
		{
			if ((EngineMouseXPos - lastx) != 0.0 || (EngineMouseYPos - lasty) != 0.0)
			{
				EventHandling::Event event = EventHandling::Event(EventHandling::EventType::ON_MOUSEMOVED);
				event.mouse.x = EngineMouseXPos - lastx;
				event.mouse.y = EngineMouseYPos - lasty;
				event.deltaTime = deltaTime;
				event.raisedFrom = this;
				this->FireEvent(event);

				lastx = EngineMouseXPos;
				lasty = EngineMouseYPos;
			}
		}
	}

	void Engine::HandleConfig()
	{
		std::ifstream file(this->config_path);
		
		nlohmann::json data;
		try
		{
			data = nlohmann::json::parse(file);
		}
		catch(std::exception& e)
		{
			this->logger->SimpleLog(Logging::LogLevel::Exception, "Error parsing config json '%s', what: %s", this->config_path.c_str(), e.what());
			exit(1);
		}

		EventHandling::EventType eventType = EventHandling::EventType::EVENT_UNDEFINED;

		this->scene_manager->LoadScene(this->config.defaultScene);
		this->scene_manager->SetScene(this->config.defaultScene);

		std::string setting;
		setting = "window.title";
		this->windowTitle = data[setting];

		setting = "window.size";
		this->windowX = data[setting]["x"]; this->windowY = data[setting]["y"];
	}

	void Engine::ErrorCallback(int code, const char* message)
	{
		printf("GLFW ERROR: %u, %s\n", code, message);
	}
	
	void Engine::OnWindowFocusCallback(GLFWwindow* window, int focused)
	{
		if(focused)
		{
			EngineIsWindowInFocus = true;
		}
		else
		{
			EngineIsWindowInFocus = false;
		}
	}

	void Engine::CursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
	{
		if(EngineIsWindowInFocus)
		{
			EngineMouseXPos = xpos;
			EngineMouseYPos = ypos;
		}	
		else
		{
			EngineMouseXPos = 0.0;
			EngineMouseYPos = 0.0;
		}
	}

	void Engine::CursorEnterLeaveCallback(GLFWwindow* window, int entered)
	{
		if(EngineIsWindowInFocus)
		{
			EngineIsWindowInContent = true;
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
		else
		{
			EngineIsWindowInContent = false;
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}
	
	void Engine::OnKeyEventCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		if (action == GLFW_PRESS)
		{
			Rendering::VulkanRenderingEngine* renderer = (Rendering::VulkanRenderingEngine*)glfwGetWindowUserPointer(window);
			EventHandling::Event event = EventHandling::Event(EventHandling::EventType::ON_KEYDOWN);
			event.keycode = key;
			event.deltaTime = renderer->owner_engine->GetLastDeltaTime();
			event.raisedFrom = renderer->owner_engine;
			renderer->owner_engine->FireEvent(event);
		}
		else if(action == GLFW_RELEASE)
		{
			Rendering::VulkanRenderingEngine* renderer = (Rendering::VulkanRenderingEngine*)glfwGetWindowUserPointer(window);
			EventHandling::Event event = EventHandling::Event(EventHandling::EventType::ON_KEYUP);
			event.keycode = key;
			event.deltaTime = renderer->owner_engine->GetLastDeltaTime();
			event.raisedFrom = renderer->owner_engine;
			renderer->owner_engine->FireEvent(event);
		}
	}

	void Engine::RenderCallback(VkCommandBuffer commandBuffer, uint32_t currentFrame, Engine* engine)
	{
		
		for (auto& [name, ptr] : *(engine->scene_manager->GetAllObjects()))
		{
			try
			{
				ptr->renderer->Render(commandBuffer, currentFrame);
			}
			catch (const std::exception& e)
			{
				ptr->renderer->logger->SimpleLog(Logging::LogLevel::Exception, "Caught exception while rendering object %s: %s", name.c_str(), e.what());
				exit(1);
			}
		}
	}

	void Engine::engineLoop()
	{		
		Object* object = new Object();
		object->renderer = new Rendering::AxisRenderer(this);
		object->Translate(glm::vec3(0, 0, 0));
		object->Scale(glm::vec3(1, 1, 1));
		object->Rotate(glm::vec3(0, 0, 0));
		object->ScreenSizeInform(this->windowX, this->windowY);
		((Rendering::AxisRenderer*)object->renderer)->UpdateMesh();
		((Rendering::AxisRenderer*)object->renderer)->scene_manager = this->scene_manager;
		this->scene_manager->AddObject("_axis", object);
		
		// Pre-make ON_UPDATE event so we don't have to create it over and over again in hot loop
		EventHandling::Event premadeOnUpdateEvent = EventHandling::Event(EventHandling::EventType::ON_UPDATE);
		premadeOnUpdateEvent.raisedFrom = this;
		
		glfwShowWindow(this->rendering_engine->GetWindow().window);

		int counter = 0;
		auto prevClock = std::chrono::steady_clock::now();
		// hot loop
		while (!glfwWindowShouldClose(this->rendering_engine->GetWindow().window))
		{
			const auto nextClock = std::chrono::steady_clock::now();
			const double deltaTime = (nextClock - prevClock).count() / 1e9;
			if (counter == this->framerateTarget)
			{
				// For debugging, use onscreen counter if possible
				//printf("current frame time: %.2lf ms (%.1lf fps)\n", deltaTime * 1e3, (1 / (deltaTime)));
				counter = 0;
			}
			this->lastDeltaTime = deltaTime;

			// Update deltaTime of premade ON_UPDATE event and fire it
			premadeOnUpdateEvent.deltaTime = deltaTime;
			if(this->FireEvent(premadeOnUpdateEvent) != 0)
			{
				break;
			}

			// Render
			this->rendering_engine->drawFrame();
			
			//glfwSwapBuffers(this->rendering_engine->GetWindow().window);
			glfwPollEvents();
			
			// spin sleep if framerate locked
			if(this->framerateTarget == 0)
			{
				this->framerateTarget = 5000;
			}

			const auto frameClock = std::chrono::steady_clock::now();
			const double sleepSecs = 1.0 / this->framerateTarget - (frameClock - nextClock).count() / 1e9;
			if (sleepSecs > 0 && framerateTarget != 0)
			{
				spinSleep(sleepSecs);
			}

			prevClock = nextClock;
			counter++;
		}
	}

	int Engine::FireEvent(EventHandling::Event& event)
	{
		int sum = 0;

		auto handler_range = this->event_handlers.equal_range(event.event_type); 
		for(auto handler = handler_range.first; handler != handler_range.second; ++handler)
		{
			sum += handler->second(event);
		}

		auto lua_handler_range = this->scene_manager->GetSceneCurrent()->lua_event_handlers.equal_range(event.event_type);
		for(auto handler = lua_handler_range.first; handler != lua_handler_range.second; ++handler)
		{
			sum += this->script_executor->CallIntoScript(Scripting::ExecuteType::EventHandler, handler->second.first, handler->second.second, &event);
		}
		return sum;
	}

	double Engine::GetLastDeltaTime()
	{
		return this->lastDeltaTime;
	}

	Engine::Engine(std::shared_ptr<Logging::Logger> logger, EngineConfig& config) noexcept : logger(logger)
	{
		this->config = config;
	}

	Engine::~Engine() noexcept
	{
		delete this->asset_manager;

		delete this->script_executor;

		this->scene_manager.reset();

		this->rendering_engine->cleanup();

		delete this->rendering_engine;
	}

	void Engine::SetFramerateTarget(unsigned framerate) noexcept
	{
		this->framerateTarget = framerate;
	}

	void Engine::Init()
	{
		this->logger->MapCurrentThreadToName("engine");

		// Engine info printout
#if defined(_MSC_VER)
		this->logger->SimpleLog(Logging::LogLevel::Info, 
			"Engine info:\n////\nRunning CMEP EngineCore %s %s build, configured %s\nCompiled by MSVC compiler version: %u.%u\n////\n", 
			__TIME__, __DATE__, _DEBUG ? "DEBUG" : "RELEASE", _MSC_FULL_VER, _MSC_BUILD
		);
#elif defined(__GNUC__)
		this->logger->SimpleLog(Logging::LogLevel::Info, 
			"Engine info:\n////\nRunning CMEP EngineCore %s %s build\nCompiled by GCC compiler version: %u.%u.%u\n////\n", 
			__TIME__, __DATE__, __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__
		);
#else
		this->logger->SimpleLog(Logging::LogLevel::Info, 
			"Engine info:\n////\nRunning CMEP EngineCore %s %s build\nCompiled by unknown compiler\n////\n", 
			__TIME__, __DATE__
		);
#endif

		this->script_executor = new Scripting::LuaScriptExecutor();
		this->script_executor->UpdateHeldLogger(this->logger);

		this->asset_manager = new AssetManager();
		this->asset_manager->current_load_path = this->config.lookup.scenes + std::string("/") + this->config.defaultScene + std::string("/");
		this->asset_manager->owner_engine = this;
		this->asset_manager->logger = this->logger;
		this->asset_manager->lua_executor = this->script_executor;

		this->scene_manager = std::make_shared<SceneManager>(this->logger);
		this->scene_manager->owner_engine = this;
		this->scene_manager->SetSceneLoadPrefix(this->config.lookup.scenes);

		this->rendering_engine = new Rendering::VulkanRenderingEngine();
		this->rendering_engine->logger = this->logger;
	}

	void Engine::Run()
	{
		auto start = std::chrono::steady_clock::now();

		this->rendering_engine->owner_engine = this;
		this->rendering_engine->init(this->config.window.sizeX, this->config.window.sizeY, this->config.window.title);
		this->rendering_engine->SetRenderCallback(this->RenderCallback);

		try
		{
			this->HandleConfig();
		}
		catch(std::exception e)
		{
			this->logger->SimpleLog(Logging::LogLevel::Exception, "Failed handling config! e.what(): %s", e.what());
			exit(1);
		}

		Rendering::GLFWwindowData windowdata = this->rendering_engine->GetWindow();
		glfwSetWindowFocusCallback(windowdata.window, Engine::OnWindowFocusCallback);
		glfwSetCursorPosCallback(windowdata.window, Engine::CursorPositionCallback);
		glfwSetCursorEnterCallback(windowdata.window, Engine::CursorEnterLeaveCallback);
		glfwSetKeyCallback(windowdata.window, Engine::OnKeyEventCallback);
		glfwSetErrorCallback(Engine::ErrorCallback);
		
		// Fire ON_INIT event
		EventHandling::Event onInitEvent = EventHandling::Event(EventHandling::EventType::ON_INIT);
		onInitEvent.raisedFrom = this;
		int onInitEventRet = this->FireEvent(onInitEvent);

		// Measure and log ON_INIT time
		double total = (std::chrono::steady_clock::now() - start).count() / 1e6;
		this->logger->SimpleLog(Logging::LogLevel::Debug1, "Handling ON_INIT took %.3lf ms total and returned %i", total, onInitEventRet);
		
		if(onInitEventRet != 0)
		{
			return;
		}

		this->engineLoop();
	}

	void Engine::ConfigFile(std::string path)
	{
		this->config_path = std::move(path);
	}

	void Engine::RegisterEventHandler(EventHandling::EventType event_type, std::function<int(EventHandling::Event&)> function)
	{
		this->event_handlers.emplace(event_type, function);
		
	}
}