#include "SceneLoader.hpp"
#include "EventHandling.hpp"
#include "AssetManager.hpp"
#include "Engine.hpp"

#include "nlohmann-json/single_include/nlohmann/json.hpp"

#include <fstream>

namespace Engine
{
    SceneLoader::SceneLoader(std::shared_ptr<Logging::Logger> logger)
    {
        this->logger = logger;
    }

    SceneLoader::~SceneLoader()
    {

    }

    void SceneLoader::LoadSceneInternal(std::shared_ptr<Scene>& scene, std::string scene_name)
    {
        std::ifstream file(this->scene_prefix + "/" + scene_name + "/scene.json");
		
		nlohmann::json data;
		try
		{
			data = nlohmann::json::parse(file);
		}
		catch(std::exception& e)
		{
			this->logger->SimpleLog(Logging::LogLevel::Exception, "Error parsing scene.json '%s', what: %s", std::string(this->scene_prefix + "/" + scene_name + "/scene.json").c_str(), e.what());
			throw std::runtime_error("Error parsing scene.json");
		}

		std::string prefix_scene = this->scene_prefix + std::string("/") + scene_name + std::string("/");
		this->logger->SimpleLog(Logging::LogLevel::Info, "Loading scene prefix is: %s", prefix_scene.c_str());

		EventHandling::EventType eventType = EventHandling::EventType::EVENT_UNDEFINED;

        AssetManager* asset_manager = this->owner_engine->GetAssetManager();

        this->owner_engine->UnregisterAllLuaEventHandlers();

		for(auto& eventHandler : data["eventHandlers"])
		{
			if(eventHandler["type"] == std::string("onInit"))
			{
				eventType = EventHandling::EventType::ON_INIT;
			}
			else if(eventHandler["type"] == std::string("onMouseMoved"))
			{
				eventType = EventHandling::EventType::ON_MOUSEMOVED;
			}
			else if(eventHandler["type"] == std::string("onKeyDown"))
			{
				eventType = EventHandling::EventType::ON_KEYDOWN;
			}
			else if(eventHandler["type"] == std::string("onKeyUp"))
			{
				eventType = EventHandling::EventType::ON_KEYUP;
			}
			else if(eventHandler["type"] == std::string("onUpdate"))
			{
				eventType = EventHandling::EventType::ON_UPDATE;
			}

			assert(eventType != EventHandling::EventType::EVENT_UNDEFINED);

			this->logger->SimpleLog(Logging::LogLevel::Debug3, "Event handler for type: %s", static_cast<std::string>(eventHandler["type"]).c_str());
			std::shared_ptr<Scripting::LuaScript> event_handler = asset_manager->GetLuaScript(prefix_scene + std::string(eventHandler["file"]));
			
			if(event_handler == nullptr)
			{
				asset_manager->AddLuaScript(prefix_scene + std::string(eventHandler["file"]), prefix_scene + std::string(eventHandler["file"]));
				event_handler = asset_manager->GetLuaScript(prefix_scene + std::string(eventHandler["file"]));
			}
			
			this->owner_engine->RegisterLuaEventHandler(eventType, event_handler, eventHandler["function"]);
		}
    }

    std::shared_ptr<Scene> SceneLoader::LoadScene(std::string scene_name)
    {
        std::shared_ptr<Scene> new_scene = std::make_shared<Scene>();
        new_scene->logger = this->logger;

        this->LoadSceneInternal(new_scene, scene_name);

        return new_scene;
    }
}