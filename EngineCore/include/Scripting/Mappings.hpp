#pragma once

#include "lualib/lua.hpp"
#include "SceneManager.hpp"

#include <unordered_map>

#define CMEP_LUAMAPPING_DEFINE(mapping) {#mapping, Functions::mapping }

namespace Engine::Scripting::Mappings
{
	namespace Functions
	{
		int metaLogger_SimpleLog(lua_State* state);

		int sm_GetCameraHVRotation(lua_State* state);
		int sm_SetCameraHVRotation(lua_State* state);
		int sm_GetCameraTransform(lua_State* state);
		int sm_SetCameraTransform(lua_State* state);
		int sm_GetLightTransform(lua_State* state);
		int sm_SetLightTransform(lua_State* state);
		int sm_AddObject(lua_State* state);
		int sm_FindObject(lua_State* state);
		int sm_RemoveObject(lua_State* state);

		int engine_GetAssetManager(lua_State* state);
		int engine_SetFramerateTarget(lua_State* state);

		int textRenderer_UpdateText(lua_State* state);

		int meshRenderer_UpdateTexture(lua_State* state);

		int object_GetRotation(lua_State* state);

		int assetManager_GetFont(lua_State* state);
		int assetManager_GetTexture(lua_State* state);
		int assetManager_AddTexture(lua_State* state);

		int objectFactory_CreateSpriteObject(lua_State* state);
		int objectFactory_CreateTextObject(lua_State* state);
		int objectFactory_CreateGeneric3DObject(lua_State* state);

		//int mesh_Mesh(lua_State* state);
		//int mesh_CreateMeshFromObj(lua_State* state);
	}

	// const uint32_t countMappings = 24;

	extern std::unordered_map<std::string, lua_CFunction> sceneManager_Mappings;
	extern std::unordered_map<std::string, lua_CFunction> object_Mappings;

	[[deprecated]]
	extern std::unordered_map<std::string, lua_CFunction> mappings;

	// extern const char* nameMappings[];
	// extern lua_CFunction functionMappings[];
}
