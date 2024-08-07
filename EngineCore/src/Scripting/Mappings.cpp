#include "Scripting/Mappings.hpp"

#include "Assets/AssetManager.hpp"
#include "Assets/Texture.hpp"
#include "Rendering/Renderer2D.hpp"
#include "Rendering/SupplyData.hpp"

#include "Scripting/API/LuaFactories.hpp"
#include "Scripting/API/framework.hpp"

#include "Factories/ObjectFactory.hpp"

#include "Engine.hpp"
#include "EnumStringConvertor.hpp"
#include "KVPairHelper.hpp"
#include "lauxlib.h"
#include "lua.hpp"

// Prefixes for logging messages
#define LOGPFX_CURRENT LOGPFX_LUA_MAPPED
#include "Logging/LoggingPrefix.hpp"

#undef CMEP_LUAMAPPING_DEFINE
#define CMEP_LUAMAPPING_DEFINE(mapping) {#mapping, Functions::mapping}

namespace Engine::Scripting::Mappings
{
	namespace Functions
	{
#pragma region Meta

		int MetaLoggerSimpleLog(lua_State* state)
		{
			CMEP_LUACHECK_FN_ARGC(state, 1)

			const char* string = lua_tostring(state, 1);

			std::weak_ptr<Logging::Logger> logger = Scripting::API::LuaFactories::MetaLoggerFactory(state);

			if (auto locked_logger = logger.lock())
			{
				locked_logger->SimpleLog(Logging::LogLevel::Info, LOGPFX_CURRENT "%s", string);
			}
			else
			{
				return luaL_error(state, "Could not lock global meta logger");
			}

			return 0;
		}

#pragma endregion

#pragma region Renderer functions

		static int RendererForceBuild(lua_State* state)
		{
			CMEP_LUACHECK_FN_ARGC(state, 1)

			auto* renderer = static_cast<Rendering::IRenderer*>(lua_touserdata(state, 1));

			renderer->ForceBuild();

			return 0;
		}

		static int RendererSupplyText(lua_State* state)
		{
			CMEP_LUACHECK_FN_ARGC(state, 2)

			auto* renderer = static_cast<Rendering::IRenderer*>(lua_touserdata(state, 1));

			const char* text = lua_tostring(state, 2);

			Rendering::RendererSupplyData text_supply = {Rendering::RendererSupplyDataType::TEXT, text};
			renderer->SupplyData(text_supply);

			return 0;
		}

		static int RendererSupplyTexture(lua_State* state)
		{
			CMEP_LUACHECK_FN_ARGC(state, 2)

			auto* renderer = static_cast<Rendering::IRenderer*>(lua_touserdata(state, 1));

			lua_getfield(state, 2, "_smart_ptr");
			std::shared_ptr<Rendering::Texture> texture = *static_cast<std::shared_ptr<Rendering::Texture>*>(
				lua_touserdata(state, -1)
			);

			Rendering::RendererSupplyData texture_supply = {Rendering::RendererSupplyDataType::TEXTURE, texture};
			renderer->SupplyData(texture_supply);

			return 0;
		}

#pragma endregion

#pragma region ObjectFactory

		[[nodiscard]] static std::vector<Rendering::RendererSupplyData> InterpretSupplyData(
			lua_State* state,
			AssetManager* asset_manager,
			int start_idx
		)
		{
			std::vector<Rendering::RendererSupplyData> supply_data;

			int idx = 1;
			while (true)
			{
				lua_rawgeti(state, start_idx, idx);

				if (lua_isnil(state, -1))
				{
					break;
				}

				lua_rawgeti(state, -1, 1);
				lua_rawgeti(state, -2, 2);

				std::string type  = lua_tostring(state, -2);
				std::string value = lua_tostring(state, -1);

				Factories::ObjectFactory::PushSupplyData(asset_manager, supply_data, type, value);

				lua_pop(state, 3);

				idx++;
			}

			return supply_data;
		}

		static int CreateSceneObject(lua_State* state)
		{
			CMEP_LUACHECK_FN_ARGC(state, 4)

			lua_getfield(state, 1, "_smart_ptr");
			std::weak_ptr<AssetManager> asset_manager = *static_cast<std::weak_ptr<AssetManager>*>(
				lua_touserdata(state, -1)
			);

			std::string template_params = lua_tostring(state, 2);

			// separates out params for the object
			// valid format is 'renderer/mesh_builder'
			auto [renderer_type, mesh_builder_type] = Utility::SplitKVPair(template_params, "/");

			std::string shader_name = lua_tostring(state, 3);

			// Check for table
			if (lua_istable(state, 4))
			{
				if (auto locked_asset_manager = asset_manager.lock())
				{
					auto supply_data = InterpretSupplyData(state, locked_asset_manager.get(), 4);

					// Check if mesh builder type is valid, if so, get a factory for it
					const auto factory =
						Factories::ObjectFactory::GetSceneObjectFactory(renderer_type, mesh_builder_type);

					if (factory)
					{
						Object* obj = factory(locked_asset_manager->GetOwnerEngine(), shader_name, supply_data);

						if (obj != nullptr)
						{
							API::LuaFactories::ObjectFactory(state, obj);

							return 1;
						}

						return luaL_error(state, "Object was nullptr!");
					}

					return luaL_error(state, "No factory was found for this object!");
				}

				return luaL_error(state, "Could not lock asset manager!");
			}

			return luaL_error(state, "Invalid parameter type (expected 'table')");
		}

#pragma endregion

	} // namespace Functions

	std::unordered_map<std::string, lua_CFunction> mappings = {
		CMEP_LUAMAPPING_DEFINE(RendererSupplyText),
		CMEP_LUAMAPPING_DEFINE(RendererSupplyTexture),
		CMEP_LUAMAPPING_DEFINE(RendererForceBuild),

		CMEP_LUAMAPPING_DEFINE(CreateSceneObject)
	};
} // namespace Engine::Scripting::Mappings
