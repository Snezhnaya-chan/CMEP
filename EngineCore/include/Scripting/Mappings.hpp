#pragma once

#include "lualib/lua.hpp"

#include "GlobalSceneManager.hpp"

namespace Engine::Scripting::Mappings
{
	namespace Functions
	{
		int gsm_GetCameraHVRotation(lua_State* state);
		int gsm_SetCameraHVRotation(lua_State* state);
	
		int gsm_GetCameraTransform(lua_State* state);
		int gsm_SetCameraTransform(lua_State* state);

		int engine_AddObject(lua_State* state);
		int engine_FindObject(lua_State* state);
		int engine_GetAssetManager(lua_State* state);
		int engine_SetFramerateTarget(lua_State* state);

		int textRenderer_UpdateText(lua_State* state);

		int meshRenderer_UpdateTexture(lua_State* state);

		int object_GetRotation(lua_State* state);

		int assetManager_GetFont(lua_State* state);
		int assetManager_GetTexture(lua_State* state);
		int assetManager_AddTexture(lua_State* state);

		int objectFactory_CreateTextObject(lua_State* state);
		int objectFactory_CreateGeneric3DObject(lua_State* state);

		int mesh_Mesh(lua_State* state);
		int mesh_CreateMeshFromObj(lua_State* state);
	}

	const uint32_t countMappings = 19;

	const char* nameMappings[];

	lua_CFunction functionMappings[];
}
