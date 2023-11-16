#pragma once

#include <unordered_map>
#include <functional>
#include <vector>
#include <memory>
#include <atomic>

#include "Rendering/VulkanRenderingEngine.hpp"

#include "SceneManager.hpp"
#include "Logging/Logging.hpp"
#include "EventHandling.hpp"
#include "PlatformSemantics.hpp"

namespace Engine
{
	class AssetManager;
	class ImageObject;
	class Object;
	namespace Rendering { class Window; }

	namespace Scripting
	{
		class LuaScriptExecutor;
		class LuaScript;
	}

	extern bool EngineIsWindowInFocus;
	extern bool EngineIsWindowInContent;
	extern double EngineMouseXPos;
	extern double EngineMouseYPos;

	typedef struct structEngineConfig
	{
		struct
		{
			unsigned int sizeX = 0;
			unsigned int sizeY = 0;
			std::string title = "I am an title!";
		} window;

		struct {
			unsigned int framerateTarget = 0;
		} rendering;

		struct {
			std::string textures;
			std::string models;
			std::string scripts;
			std::string scenes; 
		} lookup;

		std::string defaultScene;
	} EngineConfig;

	class CMEP_EXPORT Engine final
	{
	private:
		std::string config_path = "";

		// Window
		unsigned int windowX = 0, windowY = 0;
		std::string windowTitle;
		unsigned int framerateTarget = 30;

		double lastDeltaTime = 0.0;

		EngineConfig config{};

		// Engine parts
		Rendering::VulkanRenderingEngine* rendering_engine = nullptr;
		AssetManager* asset_manager = nullptr;
		Scripting::LuaScriptExecutor* script_executor = nullptr;
        std::shared_ptr<Logging::Logger> logger{};

		// Event handler storage
		std::multimap<EventHandling::EventType, std::function<int(EventHandling::Event&)>> event_handlers;
		std::multimap<EventHandling::EventType, std::pair<std::shared_ptr<Scripting::LuaScript>, std::string>> lua_event_handlers;
		
		static void spinSleep(double seconds);

		static void RenderCallback(VkCommandBuffer commandBuffer, uint32_t currentFrame, Engine* engine);

		static void ErrorCallback(int code, const char* message);
		static void OnWindowFocusCallback(GLFWwindow* window, int focused);
		static void CursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
		static void CursorEnterLeaveCallback(GLFWwindow* window, int entered);
		static void OnKeyEventCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

		void handleInput(const double deltaTime) noexcept;

		void engineLoop();

		void HandleConfig();

	public:
		std::shared_ptr<SceneManager> scene_manager{};
		
		Engine(std::shared_ptr<Logging::Logger> logger, EngineConfig& config) noexcept;
		~Engine() noexcept;

		void SetFramerateTarget(unsigned framerate) noexcept;

		void Init();
		void Run();

		void ConfigFile(std::string path);
		void RegisterEventHandler(EventHandling::EventType event_type, std::function<int(EventHandling::Event&)> function);
		inline void UnregisterAllLuaEventHandlers() {  this->lua_event_handlers.clear(); }
		void RegisterLuaEventHandler(EventHandling::EventType event_type, std::shared_ptr<Scripting::LuaScript> script, std::string function);
		
		int FireEvent(EventHandling::Event& event);

		double GetLastDeltaTime();

		inline AssetManager* GetAssetManager() noexcept { return this->asset_manager; }
		inline Rendering::VulkanRenderingEngine* GetRenderingEngine() noexcept { return this->rendering_engine; }
		inline std::weak_ptr<SceneManager> GetSceneManager() noexcept { return std::weak_ptr<SceneManager>(this->scene_manager); }
	};
}