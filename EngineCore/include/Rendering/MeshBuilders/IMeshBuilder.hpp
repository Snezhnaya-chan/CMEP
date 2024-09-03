#pragma once

#include "Rendering/MeshBuilders/MeshBuildContext.hpp"
#include "Rendering/SupplyData.hpp"

#include "InternalEngineObject.hpp"

namespace Engine::Rendering
{
	class IMeshBuilder : public InternalEngineObject
	{
	public:
		IMeshBuilder() = delete;
		IMeshBuilder(Engine* engine);

		virtual ~IMeshBuilder()
		{
			delete context.vbo;
		}

		// Always call IMeshBuilder::SupplyData when overriding!
		virtual void SupplyData(const MeshBuilderSupplyData& data)
		{
			needs_rebuild = true;
			(void)(data);
		}

		// TODO: Remove
		void SupplyWorldPosition(const glm::vec3& with_world_position)
		{
			world_pos = with_world_position;
		}

		virtual void Build() = 0;

		[[nodiscard]] virtual vk::PrimitiveTopology GetSupportedTopology() const noexcept = 0;

		[[nodiscard]] const MeshBuildContext& GetContext() const
		{
			return context;
		}

		[[nodiscard]] bool NeedsRebuild() const noexcept
		{
			return needs_rebuild;
		}

	protected:
		std::vector<RenderingVertex> mesh;
		bool needs_rebuild = true;

		MeshBuildContext context = {};
		Vulkan::Instance* instance;

		glm::vec3 world_pos;
	};
} // namespace Engine::Rendering
