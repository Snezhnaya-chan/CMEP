#pragma once

#include "IRenderer.hpp"

#include <memory>
#include <string>

namespace Engine::Rendering
{
	class Texture;
	class Font;

	class TextRenderer final : public IRenderer
	{
	private:
		std::string text;

		std::shared_ptr<Rendering::Texture> texture = nullptr;
		// std::shared_ptr<Rendering::Font> font		= nullptr;

		glm::mat4 mat_mvp{};

	public:
		TextRenderer(Engine* engine, IMeshBuilder* with_builder);
		~TextRenderer() override;

		void SupplyData(const RendererSupplyData& data) override;

		void UpdateMesh() override;

		void Render(VkCommandBuffer commandBuffer, uint32_t currentFrame) override;

		[[nodiscard]] bool GetIsUI() const override
		{
			return true;
		}
	};
} // namespace Engine::Rendering
