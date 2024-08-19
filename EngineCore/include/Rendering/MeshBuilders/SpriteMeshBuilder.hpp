#pragma once

#include "IMeshBuilder.hpp"

namespace Engine::Rendering
{
	class SpriteMeshBuilder final : public IMeshBuilder
	{
	public:
		using IMeshBuilder::IMeshBuilder;
		using IMeshBuilder::SupplyData;

		void Build() override;

		[[nodiscard]] vk::PrimitiveTopology GetSupportedTopology() const noexcept override
		{
			return vk::PrimitiveTopology::eTriangleList; // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		}

		static constexpr bool supports_2d = true;
		static constexpr bool supports_3d = true;
	};
} // namespace Engine::Rendering
