#include <assert.h>

#include "Rendering/Font.hpp"
#include "Rendering/MeshRenderer.hpp"
#include "Rendering/SpriteRenderer.hpp"
#include "Rendering/TextRenderer.hpp"
#include "Rendering/Texture.hpp"

#include "Factories/ObjectFactory.hpp"

#include "glm/vec3.hpp"

namespace Engine::ObjectFactory
{
	Object* CreateSpriteObject(
		std::weak_ptr<SceneManager> scene_manager,
		double x,
		double y,
		double z,
		double sizex,
		double sizey,
		std::shared_ptr<::Engine::Rendering::Texture> sprite
	)
	{
		assert(sprite != nullptr);
		if (auto locked_scene_manager = scene_manager.lock())
		{
			Object* object = new Object();
			object->renderer = new Rendering::SpriteRenderer(locked_scene_manager->GetOwnerEngine());
			object->Translate(glm::vec3(x, y, z));
			object->Scale(glm::vec3(sizex, sizey, 0));

			Rendering::RendererSupplyData texture_supply(Rendering::RendererSupplyDataType::TEXTURE, sprite);
			// data.type = Rendering::RendererSupplyDataType::TEXTURE;
			// data.payload = sprite;

			object->renderer->scene_manager = scene_manager;

			object->renderer->SupplyData(texture_supply);

			((Rendering::SpriteRenderer*)object->renderer)->UpdateMesh();
			return object;
		}
		return nullptr;
	}

	Object* CreateTextObject(
		std::weak_ptr<SceneManager> scene_manager,
		double x,
		double y,
		double z,
		int size,
		std::string text,
		std::shared_ptr<Rendering::Font> font
	)
	{
		assert(font != nullptr);
		if (auto locked_scene_manager = scene_manager.lock())
		{
			Object* object = new Object();

			object->renderer = new Rendering::TextRenderer(locked_scene_manager->GetOwnerEngine());

			object->Translate(glm::vec3(x, y, z));
			object->Scale(glm::vec3(size, size, 0));

			//Rendering::RendererSupplyData font_supply(Rendering::RendererSupplyDataType::FONT, font);

			object->renderer->scene_manager = scene_manager;

			Rendering::RendererSupplyData font_supply(Rendering::RendererSupplyDataType::FONT, font);

			object->renderer->SupplyData(font_supply);

			//((Rendering::TextRenderer*)object->renderer)->UpdateFont(font);
			((Rendering::TextRenderer*)object->renderer)->UpdateText(std::move(text));
			((Rendering::TextRenderer*)object->renderer)->UpdateMesh();

			return object;
		}
		return nullptr;
	}

	Object* CreateGeneric3DObject(
		std::weak_ptr<SceneManager> scene_manager,
		double x,
		double y,
		double z,
		double sizex,
		double sizey,
		double sizez,
		double rotx,
		double roty,
		double rotz,
		std::shared_ptr<Rendering::Mesh> mesh
	)
	{
		if (auto locked_scene_manager = scene_manager.lock())
		{
			Object* object = new Object();

			object->renderer = new Rendering::MeshRenderer(locked_scene_manager->GetOwnerEngine());

			object->Translate(glm::vec3(x, y, z));
			object->Scale(glm::vec3(sizex, sizey, sizez));
			object->Rotate(glm::vec3(rotx, roty, rotz));

			object->renderer->scene_manager = scene_manager;

			((Rendering::MeshRenderer*)object->renderer)->AssignMesh(mesh);
			return object;
		}
		return nullptr;
	}
} // namespace Engine::ObjectFactory