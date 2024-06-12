#include "Engine.hpp"

#include "Scripting/API/Engine_API.hpp"
#include "Scripting/API/LuaFactories.hpp"
#include "Scripting/API/framework.hpp"
#include "Scripting/lualib/lua.h"

// Prefixes for logging messages
#define LOGPFX_CURRENT LOGPFX_LUA_MAPPED
#include "Logging/LoggingPrefix.hpp"

namespace Engine::Scripting::API
{
	namespace Functions_Engine
	{
		int GetAssetManager(lua_State* state)
		{
			lua_getfield(state, 1, "_pointer");
			Engine* engine = *(Engine**)lua_touserdata(state, -1);

			std::weak_ptr<AssetManager> asset_manager = engine->GetAssetManager();

			if (!asset_manager.expired())
			{
				API::LuaObjectFactories::AssetManagerFactory(state, asset_manager);
			}
			else
			{
				std::weak_ptr<Logging::Logger> logger = API::LuaObjectFactories::MetaLoggerFactory(state);

				if (auto locked_logger = logger.lock())
				{
					locked_logger->SimpleLog(
						Logging::LogLevel::Warning, LOGPFX_CURRENT "AssetManager requested but is expired!"
					);
				}

				return luaL_error(state, "AssetManager is expired");
			}

			return 1;
		}

		int GetSceneManager(lua_State* state)
		{
			lua_getfield(state, 1, "_pointer");
			Engine* engine = *(Engine**)lua_touserdata(state, -1);

			std::weak_ptr<SceneManager> scene_manager = engine->GetSceneManager();

			if (!scene_manager.expired())
			{
				API::LuaObjectFactories::SceneManagerFactory(state, scene_manager);
			}
			else
			{
				std::weak_ptr<Logging::Logger> logger = API::LuaObjectFactories::MetaLoggerFactory(state);

				if (auto locked_logger = logger.lock())
				{
					locked_logger->SimpleLog(
						Logging::LogLevel::Warning, LOGPFX_CURRENT "SceneManager requested but is expired!"
					);
				}

				return luaL_error(state, "SceneManager is expired");
			}

			return 1;
		}

		int SetFramerateTarget(lua_State* state)
		{
			lua_getfield(state, 1, "_pointer");
			Engine* engine = *(Engine**)lua_touserdata(state, -1);

			unsigned int framerate_target = static_cast<unsigned int>(lua_tointeger(state, 2));

			engine->SetFramerateTarget(framerate_target);

			return 0;
		}

		int Stop(lua_State* state)
		{
			lua_getfield(state, 1, "_pointer");
			Engine* engine = *(Engine**)lua_touserdata(state, -1);

			engine->Stop();

			return 0;
		}

	} // namespace Functions_Engine

	std::unordered_map<std::string, lua_CFunction> engine_mappings = {
		CMEP_LUAMAPPING_DEFINE(Functions_Engine, GetAssetManager),
		CMEP_LUAMAPPING_DEFINE(Functions_Engine, GetSceneManager),
		CMEP_LUAMAPPING_DEFINE(Functions_Engine, SetFramerateTarget),
		CMEP_LUAMAPPING_DEFINE(Functions_Engine, Stop),
	};
} // namespace Engine::Scripting::API