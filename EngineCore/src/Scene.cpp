#include "Scene.hpp"
#include "Engine.hpp"

namespace Engine
{
    Scene::Scene() {}
    Scene::~Scene()
    {
		for (auto& [name, ptr] : this->objects)
		{
			delete ptr;
		}
		this->objects.clear();
    }

    const std::unordered_map<std::string, Object*>* const Scene::GetAllObjects() noexcept
	{
		return &(this->objects);
	}
	
	void Scene::AddObject(std::string name, Object* ptr)
	{
		this->logger->SimpleLog(Logging::LogLevel::Info, "Adding object \"%s\" to managed scene", name.c_str());
		if (ptr != nullptr)
		{
			Rendering::GLFWwindowData data = this->owner_engine->GetRenderingEngine()->GetWindow();
			ptr->ScreenSizeInform(data.windowX, data.windowY);
			ptr->UpdateHeldLogger(this->logger);
			ptr->renderer->UpdateHeldLogger(this->logger);
			this->objects.emplace(name, ptr);
		}
	}

	Object* Scene::FindObject(std::string name)
	{
		auto find_ret = this->objects.find(name);
		if (find_ret != this->objects.end())
		{
			return find_ret->second;
		}
		return nullptr;
	}

	size_t Scene::RemoveObject(std::string name) noexcept
	{
		Object* object = this->FindObject(name);
		
		if(object)
		{
			this->logger->SimpleLog(Logging::LogLevel::Info, "Removing object \"%s\" from managed scene, deleting object", name.c_str());
			delete object;
		}

		return this->objects.erase(name);
	}
}