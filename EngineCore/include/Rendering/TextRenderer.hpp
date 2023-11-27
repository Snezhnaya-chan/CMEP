#pragma once

#include <memory>
#include <string>

#include "IRenderer.hpp"
#include "PlatformSemantics.hpp"

namespace Engine::Rendering
{
	class Texture;
	class Font;

	/// <summary>
	/// Implementation of <seealso cref="IRenderer"/> for text renderables.
	/// </summary>
	/// <inheritdoc cref="IRenderer"/>
	class CMEP_EXPORT TextRenderer final : public IRenderer
	{
	private:
		size_t vbo_vert_count = 0;

		std::string text = "";

		VulkanPipeline* pipeline = nullptr;
		VulkanBuffer* vbo = nullptr;
		VulkanTextureImage* textureImage = nullptr;

		Rendering::Font* font;
		
		glm::mat4 matMVP{};

	public:
		TextRenderer(Engine* engine);
		~TextRenderer();

		void Update(glm::vec3 pos, glm::vec3 size, glm::vec3 rotation, uint_fast16_t screenx, uint_fast16_t screeny, glm::vec3 parent_position, glm::vec3 parent_rotation, glm::vec3 parent_size) override;

		int UpdateFont(Rendering::Font* const font);
		
		int UpdateText(const std::string text);
		
		void UpdateMesh() override;
		void Render(VkCommandBuffer commandBuffer, uint32_t currentFrame) override;
	};
}