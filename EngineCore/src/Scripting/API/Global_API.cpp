#include "Scripting/API/Global_API.hpp"

#include "Rendering/MeshBuilders/IMeshBuilder.hpp"
#include "Rendering/Renderers/Renderer.hpp"
#include "Rendering/SupplyData.hpp"

#include "Scripting/API/LuaFactories.hpp"
#include "Scripting/API/framework.hpp"
#include "Scripting/LuaValue.hpp"

#include "Factories/ObjectFactory.hpp"

#include "Logging/Logging.hpp"

#include "Engine.hpp"
#include "EnumStringConvertor.hpp"
#include "Exception.hpp"
#include "KVPairHelper.hpp"
#include "lua.hpp"

#include <cassert>
#include <exception>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Engine::Scripting::API
{
	int printReplace(lua_State* state)
	{
		int argc = lua_gettop(state);

		std::weak_ptr<Logging::Logger> logger =
			*static_cast<std::weak_ptr<Logging::Logger>*>(
				lua_touserdata(state, lua_upvalueindex(1))
			);

		if (auto locked_logger = logger.lock())
		{
			locked_logger->startLog<void>(Logging::LogLevel::Info);

			for (int arg = 1; arg <= argc; arg++)
			{
				LuaValue	value(state, arg);
				std::string string = LuaValue::toString(value);

				locked_logger->log("%s\t", string.c_str());
			}

			locked_logger->stopLog();
		}
		else { return luaL_error(state, "Could not lock global meta logger"); }

		return 0;
	}

	namespace
	{
#pragma region ObjectFactory

		template <typename supply_data_t>
		[[nodiscard]] supply_data_t interpretSupplyData(LuaValue table)
		{
			assert(table.toTable().size() == 2);

			EnumStringConvertor<typename supply_data_t::Type> type	= table[1];
			LuaValue										  value = table[2];

			using namespace Factories::ObjectFactory;

			if constexpr (std::is_same_v<supply_data_t, Rendering::RendererSupplyData>)
			{
				return generateRendererSupplyData(
					type,
					static_cast<supply_data_value_t>(value)
				);
			}
			else if constexpr (std::is_same_v<supply_data_t, Rendering::MeshBuilderSupplyData>)
			{
				return generateMeshBuilderSupplyData(
					type,
					static_cast<supply_data_value_t>(value)
				);
			}
		}

		template <typename supply_data_t>
		std::vector<supply_data_t> interpretSupplyDataTable(const LuaValue& table)
		{
			std::vector<supply_data_t> supply_data;

			ENGINE_EXCEPTION_ON_ASSERT_NOMSG(table.type == LuaValue::Type::TABLE)

			for (const auto& [key, val] : table.toTable())
			{
				try
				{
					supply_data.emplace_back(interpretSupplyData<supply_data_t>(val));
				}
				catch (...)
				{
					std::throw_with_nested(ENGINE_EXCEPTION(
						"Exception occured during interpreting of supply data"
					));
				}
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

			std::string template_params = LuaValue(state, 2);

			// separates out params for the object
			// valid format is 'renderer/mesh_builder'
			auto [renderer_type, mesh_builder_type] =
				::Engine::Utility::SplitKVPair(template_params, "/");

			std::string shader_name = LuaValue(state, 3);

			// Parse renderer supply data
			std::vector<Rendering::RendererSupplyData> renderer_supply_data =
				interpretSupplyDataTable<Rendering::RendererSupplyData>(LuaValue(state, 4)
				);

			// Parse mesh builder supply data
			std::vector<Rendering::MeshBuilderSupplyData> meshbuilder_supply_data =
				interpretSupplyDataTable<Rendering::MeshBuilderSupplyData>(
					LuaValue(state, 5)
				);

			// Get a factory for the desired object
			const auto factory = Factories::ObjectFactory::getSceneObjectFactory(
				renderer_type,
				mesh_builder_type
			);

			if (!factory)
			{
				return luaL_error(state, "No factory was found for this object!");
			}

			// Invoke the factory
			Object* obj = factory(
				owner_engine,
				shader_name,
				renderer_supply_data,
				meshbuilder_supply_data
			);

			if (obj == nullptr) { return luaL_error(state, "Object was nullptr!"); }

			API::LuaFactories::objectFactory(state, obj);

			return 1;
		}

#pragma endregion

#pragma region Renderer / MeshBuilder

		int rendererSupplyData(lua_State* state)
		{
			CMEP_LUACHECK_FN_ARGC(state, 3)

			auto* renderer = static_cast<Rendering::IRenderer*>(lua_touserdata(state, 1));

			auto type  = LuaValue(state, 2);
			auto value = LuaValue(state, 3);

			using namespace Factories::ObjectFactory;

			renderer->supplyData(
				generateRendererSupplyData(type, static_cast<supply_data_value_t>(value))
			);

			return 0;
		}

		int meshBuilderSupplyData(lua_State* state)
		{
			CMEP_LUACHECK_FN_ARGC(state, 3)

			auto* mesh_builder =
				static_cast<Rendering::IMeshBuilder*>(lua_touserdata(state, 1));

			auto type  = LuaValue(state, 2);
			auto value = LuaValue(state, 3);

			using namespace Factories::ObjectFactory;

			mesh_builder->supplyData(
				generateMeshBuilderSupplyData(type, static_cast<supply_data_value_t>(value))
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
