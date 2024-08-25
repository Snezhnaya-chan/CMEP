#pragma once

#include "Assets/Asset.hpp"

#include "InternalEngineObject.hpp"
#include "lua.hpp"

#include <filesystem>
#include <string>

namespace Engine::Scripting
{
	class LuaScriptExecutor;

	struct ScriptPerfState
	{
		int native_count	  = 0;
		int interpreted_count = 0;
		int engine_count	  = 0;
	};

	std::string UnwindStack(lua_State* of_state);
	int LuaErrorHandler(lua_State* state);

	class ILuaScript : public InternalEngineObject, public Asset
	{
	public:
		ILuaScript(Engine* with_engine, std::filesystem::path with_path, bool with_enable_profiling = false);
		virtual ~ILuaScript();

		int CallFunction(const std::string& function, void* data);

		[[nodiscard]] const ScriptPerfState& GetProfilerState() const
		{
			return profiler_state;
		}

		[[nodiscard]] lua_State* GetState()
		{
			return state;
		}

		[[nodiscard]] const std::filesystem::path& GetPath() const
		{
			return path;
		}

	protected:
		lua_State* state;
		ScriptPerfState profiler_state;

		std::filesystem::path path;

		void LoadAndCompileScript();

		virtual int InternalCall(const std::string& function, void* data) = 0;

	private:
		const bool enable_profiling;
	};
} // namespace Engine::Scripting
