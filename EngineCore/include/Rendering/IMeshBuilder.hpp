#pragma once

#include "InternalEngineObject.hpp"
#include "MeshBuildContext.hpp"
#include "SupplyData.hpp"
#include "Vulkan/VulkanRenderingEngine.hpp"

namespace Engine::Rendering
{
	class IMeshBuilder : public InternalEngineObject
	{
	protected:
		std::vector<RenderingVertex> mesh;
		bool needs_rebuild = true;

		MeshBuildContext context = {};
		Vulkan::VulkanRenderingEngine* renderer;

		glm::vec3 world_pos;

	public:
		IMeshBuilder() = delete;
		IMeshBuilder(Engine* engine);

		virtual ~IMeshBuilder()
		{
			this->logger->SimpleLog(Logging::LogLevel::Debug3, "Cleaning up MeshBuilder");

			this->renderer->SyncDeviceWaitIdle();

			delete this->context.vbo;
		}

		// Always call IMeshBuilder::SupplyData when overriding!
		virtual void SupplyData(const RendererSupplyData& data)
		{
			this->needs_rebuild = true;
			(void)(data);
		}

		void SupplyWorldPosition(const glm::vec3& with_world_position)
		{
			this->world_pos = with_world_position;
		}

		virtual void Build() = 0;

		[[nodiscard]] virtual VkPrimitiveTopology GetSupportedTopology() const noexcept = 0;

		[[nodiscard]] const MeshBuildContext& GetContext() const
		{
			return this->context;
		}

		[[nodiscard]] bool NeedsRebuild() const noexcept
		{
			return this->needs_rebuild;
		}
	};
} // namespace Engine::Rendering
