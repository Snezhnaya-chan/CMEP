#include "Logging/Logging.hpp"
#include "Rendering/Font.hpp"
#include "AssetManager.hpp"

namespace Engine
{
	AssetManager::~AssetManager()
	{
		for(auto& texture : this->textures)
		{
			this->logger->SimpleLog(Logging::LogLevel::Debug3, "Texture %s use_count: %u", texture.first.c_str(), texture.second.use_count());
		}

		this->textures.clear();

		this->logger->SimpleLog(Logging::LogLevel::Debug3, "Deleting asset manager");
	}

#pragma region Adding Assets
	void AssetManager::AddTexture(std::string name, std::string path, Rendering::Texture_InitFiletype filetype)
	{
		std::shared_ptr<Rendering::Texture> texture = std::make_shared<Rendering::Texture>();
		texture->owner_engine = this->owner_engine;

		texture->UpdateHeldLogger(this->logger);

		if (texture->InitFile(filetype, this->current_load_path + path) != 0)
		{
			this->logger->SimpleLog(Logging::LogLevel::Error, "Error occured when adding Texture %s as %s, this may be unintentional", path.c_str(), name.c_str());
			return;
		}

		this->textures.emplace(name, texture);
		this->logger->SimpleLog(Logging::LogLevel::Debug2, "Added texture %s as %s", path.c_str(), name.c_str());
	}

	void AssetManager::AddFont(std::string name, std::string path)
	{
		std::shared_ptr<Rendering::Font> font = std::make_shared<Rendering::Font>(this);
		font->owner_engine = this->owner_engine;
		
		font->UpdateHeldLogger(this->logger);
		font->Init(std::move(this->current_load_path + path));

		this->fonts.emplace(name, font);
	}

	void AssetManager::AddLuaScript(std::string name, std::string path)
	{
		Scripting::LuaScript* script = new Scripting::LuaScript(this->lua_executor, std::move(path));
		
		this->luascripts.emplace(name, script);
	}

	void AssetManager::AddModel(std::string name, std::string path)
	{
		std::shared_ptr<Rendering::Mesh> mesh = std::make_shared<Rendering::Mesh>();
		mesh->owner_engine = this->owner_engine;
		mesh->UpdateHeldLogger(this->logger);
		
		mesh->CreateMeshFromObj(this->current_load_path + path);

		this->models.emplace(name, std::move(mesh));
	}
#pragma endregion

#pragma region Getting Assets
	std::shared_ptr<Rendering::Texture> AssetManager::GetTexture(std::string name)
	{
		if (this->textures.find(name) != this->textures.end())
		{
			this->logger->SimpleLog(Logging::LogLevel::Debug2, "Texture %s requested and is loaded", name.c_str());
			return this->textures.at(name);
		}
		else
		{
			this->logger->SimpleLog(Logging::LogLevel::Debug1, "Texture %s requested and is not loaded", name.c_str());
			this->AddTexture(name, name, Rendering::Texture_InitFiletype::FILE_PNG);
			
			return this->GetTexture(name);
		}
	}

	std::shared_ptr<Rendering::Font> AssetManager::GetFont(std::string name)
	{
		this->logger->SimpleLog(Logging::LogLevel::Debug2, "Font %s requested", name.c_str());
		if (this->fonts.find(name) != this->fonts.end())
		{
			return this->fonts.at(name);
		}
		else
		{
			std::shared_ptr<Rendering::Font> font = std::make_shared<Rendering::Font>(this);
			
			font->UpdateHeldLogger(this->logger);
			if (font->Init(this->current_load_path + name) != 0)
			{
				return nullptr;
			}
			
			this->fonts.emplace(name, font);
			return font;
		}
	}

	std::shared_ptr<Scripting::LuaScript> AssetManager::GetLuaScript(std::string name)
	{
		this->logger->SimpleLog(Logging::LogLevel::Debug2, "LuaScript %s requested", name.c_str());
		if (this->luascripts.find(name) != this->luascripts.end())
		{
			return this->luascripts.at(name);
		}
		else
		{
			return nullptr;
		}
	}

	std::shared_ptr<Rendering::Mesh> AssetManager::GetModel(std::string name)
	{
		this->logger->SimpleLog(Logging::LogLevel::Debug2, "Model %s requested", name.c_str());
		if (this->models.find(name) != this->models.end())
		{
			return this->models.at(name);
		}
		else
		{
			this->logger->SimpleLog(Logging::LogLevel::Debug1, "Model %s requested and is not loaded", name.c_str());
			this->AddModel(name, name);
			return nullptr;
		}
	}
#pragma endregion
}