#include "Rendering/MeshBuilders/IMeshBuilder.hpp"

#include "Engine.hpp"

namespace Engine::Rendering
{
	IMeshBuilder::IMeshBuilder(Engine* engine)
		: InternalEngineObject(engine), instance(engine->getVulkanInstance())
	{
	}
} // namespace Engine::Rendering
