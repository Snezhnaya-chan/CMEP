#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "Object.hpp"
#include "PlatformSemantics.hpp"
#include "SceneLoader.hpp"
#include "Scene.hpp"

#include "Logging/Logging.hpp"

#include <unordered_map>
#include <memory>

namespace Engine
{
	class CMEP_EXPORT SceneManager final
	{
	private:
		std::unordered_map<std::string, std::shared_ptr<Scene>> scenes{};
		std::string current_scene = "_default";

		glm::vec3 cameraTransform{}; // XYZ position
		glm::vec2 cameraHVRotation{}; // Horizontal and Vertical rotation

		glm::vec3 lightPosition{};

		void CameraUpdated();

		std::shared_ptr<SceneLoader> scene_loader;
	public:
		std::shared_ptr<Logging::Logger> logger;
		Engine* owner_engine;
		
		SceneManager(std::shared_ptr<Logging::Logger> logger);
		~SceneManager();

		void UpdateHeldLogger(std::shared_ptr<Logging::Logger> new_logger);

		void SetSceneLoadPrefix(std::string scene_prefix);
		void LoadScene(std::string scene_name);
		void SetScene(std::string scene_name);
		std::shared_ptr<Scene> GetSceneCurrent();

		const std::unordered_map<std::string, Object*>* const GetAllObjects() noexcept;

		void AddObject(std::string name, Object* ptr);
		Object* FindObject(std::string name);
		size_t RemoveObject(std::string name) noexcept;

		glm::vec3 GetLightTransform();
		void SetLightTransform(glm::vec3 newpos);

		glm::vec3 GetCameraTransform();
		glm::vec2 GetCameraHVRotation();
		glm::mat4 GetCameraViewMatrix();

		void SetCameraTransform(glm::vec3 transform);
		void SetCameraHVRotation(glm::vec2 hvrotation);
	};
}
