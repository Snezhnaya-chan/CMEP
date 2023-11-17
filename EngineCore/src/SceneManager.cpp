#include "SceneManager.hpp"
#include "Logging/Logging.hpp"
#include "glm/common.hpp"
#include "Engine.hpp"

namespace Engine
{
	SceneManager::SceneManager(std::shared_ptr<Logging::Logger> logger)
	{
		// Reset transform and rotation
		this->cameraTransform = glm::vec3(2.0, 0, 1.0);
		this->cameraHVRotation = glm::vec2(0.0, 0.0);

		this->logger = logger;

		this->scenes.emplace("_default", std::make_shared<Scene>());
		this->scenes.at(this->current_scene)->logger = this->logger;

		this->scene_loader = std::make_shared<SceneLoader>(logger);
	}

	SceneManager::~SceneManager()
	{
	}

	void SceneManager::CameraUpdated()
	{
		for (auto& [name, ptr] : *(this->scenes.at(this->current_scene)->GetAllObjects()))
		{
			ptr->renderer->UpdateMesh();
		}
	}

	void SceneManager::SetSceneLoadPrefix(std::string scene_prefix)
	{
		this->scene_loader->scene_prefix = scene_prefix;
	}
	
	void SceneManager::LoadScene(std::string scene_name)
	{
		this->scene_loader->owner_engine = this->owner_engine;
		this->scenes.emplace(scene_name, this->scene_loader->LoadScene(scene_name));
	}

	void SceneManager::SetScene(std::string scene_name)
	{
		this->current_scene = scene_name;
	}

	std::shared_ptr<Scene> SceneManager::GetSceneCurrent()
	{
		return this->scenes.at(this->current_scene);
	}

	const std::unordered_map<std::string, Object*>* const SceneManager::GetAllObjects() noexcept
	{
		return this->scenes.at(this->current_scene)->GetAllObjects();
	}
	
	void SceneManager::AddObject(std::string name, Object* ptr)
	{
		this->scenes.at(this->current_scene)->owner_engine = this->owner_engine;
		this->scenes.at(this->current_scene)->AddObject(name, ptr);
	}

	Object* SceneManager::FindObject(std::string name)
	{
		return this->scenes.at(this->current_scene)->FindObject(name);
	}

	size_t SceneManager::RemoveObject(std::string name) noexcept
	{
		return this->scenes.at(this->current_scene)->RemoveObject(name);
	}

	glm::vec3 SceneManager::GetLightTransform()
	{
		return this->lightPosition;
	}

	void SceneManager::SetLightTransform(glm::vec3 newpos)
	{
		this->lightPosition = newpos;
	}

	glm::vec3 SceneManager::GetCameraTransform()
	{
		return this->cameraTransform;
	}

	glm::vec2 SceneManager::GetCameraHVRotation()
	{
		return this->cameraHVRotation;
	}

	glm::mat4 SceneManager::GetCameraViewMatrix()
	{
		glm::vec3 direction = glm::vec3(
			cos(this->cameraHVRotation.x) * cos(this->cameraHVRotation.y),
			sin(this->cameraHVRotation.y),
			sin(this->cameraHVRotation.x) * cos(this->cameraHVRotation.y)
		);

		glm::vec3 front = glm::normalize(direction);
		glm::vec3 right = glm::normalize(glm::cross(direction, glm::vec3(0, 1, 0)));
		glm::vec3 up = glm::normalize(glm::cross(right, front));

		glm::mat4 ViewMatrix = glm::lookAt(this->cameraTransform, this->cameraTransform + front, up);
		
		return ViewMatrix;
	}

	void SceneManager::SetCameraTransform(glm::vec3 transform)
	{
		this->cameraTransform = transform;
		this->CameraUpdated();
	}

	void SceneManager::SetCameraHVRotation(glm::vec2 hvrotation)
	{
		if (hvrotation.y < 1.7f)
		{
			hvrotation.y = 1.7f;
		}
		else if (hvrotation.y > 4.5f)
		{
			hvrotation.y = 4.5f;
		}

		this->cameraHVRotation = hvrotation;
		this->CameraUpdated();
	}

	void SceneManager::UpdateHeldLogger(std::shared_ptr<Logging::Logger> new_logger)
	{
		this->logger = new_logger;
	}
}