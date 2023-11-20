#include "SceneLoader.hpp"
#include "EventHandling.hpp"
#include "AssetManager.hpp"
#include "Engine.hpp"
#include "Rendering/AxisRenderer.hpp"
#include "Rendering/MeshRenderer.hpp"
#include "Rendering/TextRenderer.hpp"
#include "Rendering/SpriteRenderer.hpp"

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

		EventHandling::EventType event_type = EventHandling::EventType::EVENT_UNDEFINED;

        AssetManager* asset_manager = this->owner_engine->GetAssetManager();

		// Load scene event handlers
		for(auto& event_handler_entry : data["eventHandlers"])
		{
			if(event_handler_entry["type"] == std::string("onInit"))
			{
				event_type = EventHandling::EventType::ON_INIT;
			}
			else if(event_handler_entry["type"] == std::string("onMouseMoved"))
			{
				event_type = EventHandling::EventType::ON_MOUSEMOVED;
			}
			else if(event_handler_entry["type"] == std::string("onKeyDown"))
			{
				event_type = EventHandling::EventType::ON_KEYDOWN;
			}
			else if(event_handler_entry["type"] == std::string("onKeyUp"))
			{
				event_type = EventHandling::EventType::ON_KEYUP;
			}
			else if(event_handler_entry["type"] == std::string("onUpdate"))
			{
				event_type = EventHandling::EventType::ON_UPDATE;
			}

			assert(event_type != EventHandling::EventType::EVENT_UNDEFINED);

			this->logger->SimpleLog(Logging::LogLevel::Debug3, "Event handler for type: %s", static_cast<std::string>(event_handler_entry["type"]).c_str());
			std::shared_ptr<Scripting::LuaScript> event_handler = asset_manager->GetLuaScript(prefix_scene + std::string(event_handler_entry["file"]));
			
			if(event_handler == nullptr)
			{
				asset_manager->AddLuaScript(prefix_scene + std::string(event_handler_entry["file"]), prefix_scene + std::string(event_handler_entry["file"]));
				event_handler = asset_manager->GetLuaScript(prefix_scene + std::string(event_handler_entry["file"]));
			}
			
			scene->lua_event_handlers.emplace(event_type, std::make_pair(event_handler, event_handler_entry["function"]));
		}

		// Load scene object templates
		for(auto& template_entry : data["templates"])
		{
			Object* object = new Object();

			Rendering::GLFWwindowData window_data = this->owner_engine->GetRenderingEngine()->GetWindow();

			if(template_entry["renderer"]["type"] == std::string("sprite"))
			{
				object->renderer = new Rendering::SpriteRenderer(this->owner_engine);
				((Rendering::SpriteRenderer*)object->renderer)->UpdateTexture(asset_manager->GetTexture(template_entry["renderer"]["sprite"]));
				((Rendering::SpriteRenderer*)object->renderer)->scene_manager = this->owner_engine->GetSceneManager();
				object->renderer_type = "sprite";
			}
			
			this->logger->SimpleLog(Logging::LogLevel::Debug2, "Loaded templated object %s", std::string(template_entry["name"]).c_str());
			scene->LoadTemplatedObject(template_entry["name"], object);
		}
    }

    std::shared_ptr<Scene> SceneLoader::LoadScene(std::string scene_name)
    {
        std::shared_ptr<Scene> new_scene = std::make_shared<Scene>();
        new_scene->logger = this->logger;
		new_scene->owner_engine = this->owner_engine;

        this->LoadSceneInternal(new_scene, scene_name);

        return new_scene;
    }
}