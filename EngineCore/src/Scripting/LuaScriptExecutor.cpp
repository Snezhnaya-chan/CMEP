#include "Scripting/LuaScriptExecutor.hpp"

#include "Scripting/API/LuaFactories.hpp"
#include "Scripting/Mappings.hpp"

#include "Logging/Logging.hpp"

#include "Engine.hpp"

// #include "Scripting/lualib/lua.h"
#include "lua.hpp"

// Prefixes for logging messages
#define LOGPFX_CURRENT LOGPFX_LUA_SCRIPT_EXECUTOR
#include "Logging/LoggingPrefix.hpp"

namespace Engine
{
	namespace Scripting
	{
		// Register C callback functions from mappings
		void LuaScriptExecutor::RegisterCallbacks(lua_State* state)
		{
			lua_newtable(state);

			for (auto& mapping : Mappings::mappings)
			{
				lua_pushcfunction(state, mapping.second);
				lua_setfield(state, -2, mapping.first.c_str());
			}

			lua_setglobal(state, "cmepapi");
		}

		// Register meta information (logger etc)
		// this information can be used in C callbacks
		void LuaScriptExecutor::RegisterMeta(lua_State* state)
		{
			// cmepmeta table (not an actual Lua metatable !!!)
			lua_newtable(state);

			/*******************************/
			// Logger table
			lua_newtable(state);

			void* ptr_obj = lua_newuserdata(state, sizeof(std::weak_ptr<Logging::Logger>));
			new (ptr_obj) std::weak_ptr<Logging::Logger>(this->logger);

			lua_setfield(state, -2, "_smart_ptr");

			lua_pushcfunction(state, Mappings::Functions::MetaLoggerSimpleLog);
			lua_setfield(state, -2, "SimpleLog");

			lua_setfield(state, -2, "logger");
			/*******************************/
			/*******************************/

			lua_setglobal(state, "cmepmeta");
		}

		static int LuaErrorHandler(lua_State* state)
		{
			// We can handle errors here if necessary
			//
			// last value on stack is the error object
			//
			// last value on stack when returning has to
			// be an error object - original or another
			//
			(void)(state);

			// Simply pass the error object through to pcall
			return 1;
		}

		static int LuajitExceptionWrap(lua_State* state, lua_CFunction func)
		{
			try
			{
				return func(state);
			}
			catch (const char* str)
			{
				return luaL_error(state, str);
			}
			catch (std::exception& e)
			{
				return luaL_error(state, "Wrapper caught exception! e.what(): %s", e.what());
			}
			catch (...)
			{
				return luaL_error(state, "Error caught");
			}
		}

		void LuaScriptExecutor::RegisterWrapper(lua_State* state)
		{
			lua_pushlightuserdata(state, (void*)LuajitExceptionWrap);
			luaJIT_setmode(state, -1, LUAJIT_MODE_WRAPCFUNC | LUAJIT_MODE_ON);
			lua_pop(state, 1);
		}

		int LuaScriptExecutor::CallIntoScript(
			ExecuteType etype,
			const std::shared_ptr<LuaScript>& script,
			const std::string& function,
			void* data
		)
		{
			// this->logger->SimpleLog(Logging::LogLevel::Debug2,
			//	"Running lua script '%s', called function '%s'",
			//	script->path.c_str(), function.c_str());

			// Get script state
			lua_State* state = script->GetState();

			// Clear stack
			lua_settop(state, 0);

			// Push error handler
			lua_pushcfunction(state, LuaErrorHandler);

			// Get start function
			lua_getglobal(state, function.c_str());

			// Lua return code
			int errcall = LUA_OK;

			// Run the start function in a way decided by the ExecuteType
			switch (etype)
			{
				case ExecuteType::EVENT_HANDLER:
				{
					auto* event = static_cast<EventHandling::Event*>(data);

					// Event table
					lua_newtable(state);
					lua_pushnumber(state, event->delta_time);
					lua_setfield(state, -2, "deltaTime");

					lua_pushinteger(state, event->keycode);
					lua_setfield(state, -2, "keycode");

					Scripting::API::LuaObjectFactories::EngineFactory(state, event->raised_from);
					lua_setfield(state, -2, "engine");

					// Mouse table
					lua_newtable(state);
					lua_pushnumber(state, event->mouse.x);
					lua_setfield(state, -2, "x");
					lua_pushnumber(state, event->mouse.y);
					lua_setfield(state, -2, "y");
					lua_setfield(state, -2, "mouse");

					// Call into script
					// Stack content: [
					//		...,
					//		(-3, func)LuaErrorHandler,
					//		(-2, func)StartFunction,
					//		(-1, table)Event
					//	] <- stack top is here
					//
					errcall = lua_pcall(state, 1, 1, -3);
					break;
				}
				default:
				{
					return 1;
				}
			}

			if (errcall != LUA_OK)
			{
				const char* errormsg = "";
				errormsg			 = lua_tostring(state, -1);

				this->logger->SimpleLog(
					Logging::LogLevel::Warning,
					LOGPFX_CURRENT "Error when calling Lua\n\tscript '%s' function: "
								   "'%s'\n\tCall error code: %i\n\tError message: %s",
					script->path.c_str(),
					function.c_str(),
					errcall,
					errormsg
				);
			}

			lua_Integer ret = lua_tointeger(state, -1);
			lua_pop(state, 1);

			return static_cast<int>(ret);
		}

		int LuaScriptExecutor::LoadAndCompileScript(LuaScript* script)
		{
			// Get script state
			lua_State* state = script->GetState();

			// Load file and compile it
			int errload			  = luaL_loadfile(state, script->path.c_str());
			int errexec			  = lua_pcall(state, 0, LUA_MULTRET, 0);
			const char* errorexec = lua_tostring(state, -1);

			// Register c callback functions
			LuaScriptExecutor::RegisterCallbacks(state);
			this->RegisterMeta(state);

			// Register exception-handling wrapper
			LuaScriptExecutor::RegisterWrapper(state);

			if (errload != LUA_OK || errexec != LUA_OK)
			{
				this->logger->SimpleLog(
					Logging::LogLevel::Error,
					LOGPFX_CURRENT "Error when loading and compiling Lua script "
								   "'%s'\n   Error codes:\n    load: %i\n    compile: "
								   "%i\n  Compilation error: %s",
					script->path.c_str(),
					errload,
					errexec,
					errorexec
				);

				return 1;
			}

			this->logger->SimpleLog(
				Logging::LogLevel::Debug1,
				LOGPFX_CURRENT "Loaded and compiled Lua script: '%s'",
				script->path.c_str()
			);

			return 0;
		}
	} // namespace Scripting
} // namespace Engine
