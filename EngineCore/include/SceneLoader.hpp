#pragma once

#include "Scene.hpp"
#include "Logging/Logging.hpp"
#include "InternalEngineObject.hpp"

#include <memory>

namespace Engine
{
    class SceneLoader : public InternalEngineObject
    {
    protected:
        void LoadSceneInternal(std::shared_ptr<Scene>& scene, std::string scene_name);

    public:
        std::string scene_prefix;

        SceneLoader(std::shared_ptr<Logging::Logger> logger);
        ~SceneLoader();

        std::shared_ptr<Scene> LoadScene(std::string name);
    };
}