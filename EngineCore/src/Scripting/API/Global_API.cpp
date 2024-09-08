#include "Scripting/API/Global_API.hpp"

#include "Rendering/MeshBuilders/IMeshBuilder.hpp"
#include "Rendering/Renderers/Renderer.hpp"
#include "Rendering/SupplyData.hpp"

#include "Scripting/API/LuaFactories.hpp"
#include "Scripting/API/framework.hpp"
#include "Scripting/Utility.hpp"

#include "Factories/ObjectFactory.hpp"

#include "Logging/Logging.hpp"

#include "Engine.hpp"
#include "EnumStringConvertor.hpp"
#include "Exception.hpp"
#include "KVPairHelper.hpp"
#include "lua.hpp"

#include <exception>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Engine::Scripting::API
{
	// Replaces the default lua print() with a custom one that uses the logger
	int printReplace(lua_State* state)
	{
		// LuaJIT has no guarantees to the ID of CDATA type
		// It can be checked manually against 'lj_obj_typename' symbol
		// static constexpr int cdata_type_id = 10;

		int argc = lua_gettop(state);

		std::weak_ptr<Logging::Logger> logger = *static_cast<std::weak_ptr<Logging::Logger>*>(
			lua_touserdata(state, lua_upvalueindex(1))
		);

		if (auto locked_logger = logger.lock())
		{
			locked_logger->startLog<void>(Logging::LogLevel::Info);

			for (int arg = 1; arg <= argc; arg++)
			{
				Utility::LuaValue value(state, arg);
				std::string		  string = Utility::LuaValue::toString(value);

				locked_logger->log("%s\t", string.c_str());
			}

			locked_logger->stopLog();
		}
		else
		{
			return luaL_error(state, "Could not lock global meta logger");
		}

		return 0;
	}

	namespace
	{

#pragma region ObjectFactory

		template <typename supply_data_t>
		[[nodiscard]] supply_data_t interpretSupplyData(lua_State* state)
		{
			ENGINE_EXCEPTION_ON_ASSERT(lua_objlen(state, -1) == 2, "Incorrect data supplied")

			lua_rawgeti(state, -1, 1);
			EnumStringConvertor<typename supply_data_t::Type> type =
				Utility::LuaValue(state, -1);

			lua_rawgeti(state, -2, 2);
			Utility::LuaValue value(state, -1);

			if constexpr (std::is_same_v<supply_data_t, Rendering::RendererSupplyData>)
			{
				return Factories::ObjectFactory::generateRendererSupplyData(type, value);
			}
			else if constexpr (std::is_same_v<supply_data_t, Rendering::MeshBuilderSupplyData>)
			{
				return Factories::ObjectFactory::generateMeshBuilderSupplyData(type, value);
			}
		}

		template <typename supply_data_t>
		std::vector<supply_data_t> interpretSupplyDataTable(lua_State* state, int start_idx)
		{
			std::vector<supply_data_t> supply_data;

			int idx = 1;
			while (true)
			{
				// static constexpr size_t start_idx = 4;
				lua_rawgeti(state, start_idx, idx);

				if (lua_isnil(state, -1))
				{
					break;
				}

				try
				{
					supply_data.emplace_back(interpretSupplyData<supply_data_t>(state));
				}
				catch (...)
				{
					std::throw_with_nested(ENGINE_EXCEPTION(
						"Exception occured during interpreting of supply data"
					));
				}

				lua_pop(state, 3);

				idx++;
			}

			return supply_data;
		}

		int createSceneObject(lua_State* state)
		{
			CMEP_LUACHECK_FN_ARGC(state, 5)

			lua_getfield(state, 1, "_ptr");
			auto* owner_engine = static_cast<Engine*>(lua_touserdata(state, -1));

			ENGINE_EXCEPTION_ON_ASSERT(
				owner_engine != nullptr,
				"Tried to create scene object with invalid Engine pointer!"
			)

			std::string template_params = Utility::LuaValue(state, 2);

			// separates out params for the object
			// valid format is 'renderer/mesh_builder'
			auto [renderer_type, mesh_builder_type] =
				::Engine::Utility::SplitKVPair(template_params, "/");

			std::string shader_name = Utility::LuaValue(state, 3);

			constexpr int r_table_idx  = 4;
			constexpr int mb_table_idx = 5;

			// Check for table
			if (lua_istable(state, r_table_idx) && lua_istable(state, mb_table_idx))
			{
				std::vector<Rendering::RendererSupplyData> renderer_supply_data =
					interpretSupplyDataTable<Rendering::RendererSupplyData>(state, r_table_idx);

				std::vector<Rendering::MeshBuilderSupplyData> meshbuilder_supply_data =
					interpretSupplyDataTable<Rendering::MeshBuilderSupplyData>(state, mb_table_idx);

				// Check if mesh builder type is valid, if so, get a factory for it
				const auto factory = Factories::ObjectFactory::getSceneObjectFactory(
					renderer_type,
					mesh_builder_type
				);

				if (factory)
				{
					Object* obj = factory(
						owner_engine,
						shader_name,
						renderer_supply_data,
						meshbuilder_supply_data
					);

					if (obj != nullptr)
					{
						API::LuaFactories::objectFactory(state, obj);

						return 1;
					}

					return luaL_error(state, "Object was nullptr!");
				}

				return luaL_error(state, "No factory was found for this object!");
			}

			return luaL_error(state, "Invalid parameter type (expected 'table')");
		}

#pragma endregion

#pragma region Renderer / MeshBuilder

		int rendererSupplyData(lua_State* state)
		{
			CMEP_LUACHECK_FN_ARGC(state, 3)

			auto* renderer = static_cast<Rendering::IRenderer*>(lua_touserdata(state, 1));

			auto type  = Utility::LuaValue(state, 2);
			auto value = Utility::LuaValue(state, 3);

			renderer->supplyData(
				Factories::ObjectFactory::generateRendererSupplyData(type, value)
			);

			return 0;
		}

		int meshBuilderSupplyData(lua_State* state)
		{
			CMEP_LUACHECK_FN_ARGC(state, 3)

			auto* mesh_builder = static_cast<Rendering::IMeshBuilder*>(
				lua_touserdata(state, 1)
			);

			auto type  = Utility::LuaValue(state, 2);
			auto value = Utility::LuaValue(state, 3);

			mesh_builder->supplyData(
				Factories::ObjectFactory::generateMeshBuilderSupplyData(type, value)
			);

			return 0;
		}

#pragma endregion

	} // namespace

	std::unordered_map<std::string, const lua_CFunction> global_mappings = {
		CMEP_LUAMAPPING_DEFINE(rendererSupplyData),
		CMEP_LUAMAPPING_DEFINE(meshBuilderSupplyData),

		CMEP_LUAMAPPING_DEFINE(createSceneObject)
	};
} // namespace Engine::Scripting::API