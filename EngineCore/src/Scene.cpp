#include "Scene.hpp"
#include "Engine.hpp"
#include "Rendering/SpriteRenderer.hpp"

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
		
		for (auto& [name, ptr] : this->templates)
		{
			delete ptr;
		}
		this->templates.clear();
    }

    const std::unordered_map<std::string, Object*>* const Scene::GetAllObjects() noexcept
	{
		return &(this->objects);
	}
	
	Object* Scene::AddTemplatedObject(std::string name, std::string template_name)
	{
		auto templated_object = this->templates.find(template_name);

		if(templated_object != this->templates.end())
		{
			Object* object = new Object();

			if(templated_object->second->renderer_type == "sprite")
			{
				object->renderer = new Rendering::SpriteRenderer(this->owner_engine);
				//*(Rendering::SpriteRenderer*)object->renderer = *(Rendering::SpriteRenderer*)templated_object->second->renderer;
				((Rendering::SpriteRenderer*)object->renderer)->texture = ((Rendering::SpriteRenderer*)templated_object->second->renderer)->texture;
				object->UpdateHeldLogger(this->logger);
				object->renderer->UpdateHeldLogger(this->logger);
			}

			this->AddObject(name, object);
			return object;
		}
		return nullptr;
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

	void Scene::LoadTemplatedObject(std::string name, Object* ptr)
	{
		if (ptr != nullptr)
		{
			Rendering::GLFWwindowData data = this->owner_engine->GetRenderingEngine()->GetWindow();
			ptr->ScreenSizeInform(data.windowX, data.windowY);
			ptr->UpdateHeldLogger(this->logger);
			ptr->renderer->UpdateHeldLogger(this->logger);
			this->templates.emplace(name, ptr);
		}
	}
}