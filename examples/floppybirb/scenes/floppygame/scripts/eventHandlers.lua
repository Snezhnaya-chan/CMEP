-- ON_MOUSEMOVED event, left here for your usage!
onMouseMoved = function(event)
	return 0;
end

birbVelocity = 0.1;
birbIsVelociting = true;

-- ON_KEYDOWN event
onKeyDown = function(event)
	-- check for esc
	if event.keycode == 256 then
		cmepapi.engine_Stop(event.raisedFrom);
	end

	-- check for space press
	if string.char(event.keycode) == ' ' then
		if birbIsVelociting == false then
			birbVelocity = birbVelocity + 0.5;
			birbIsVelociting = true;
		end
	end

	return 0;
end

-- ON_KEYUP event
onKeyUp = function(event)
	if string.char(event.keycode) == ' ' then
		birbIsVelociting = false;
	end
end

deltaTimeAvg = 0.0;
deltaTimeCount = 0;

spawnPipeSinceLast = 4.0;
spawnPipeEvery = 8.0;
spawnPipeLastIdx = 0;
spawnPipeFirstIdx = 1;
spawnPipeCount = 0;

gameIsGameOver = false;
gameLastScoredPipeIdx = 0;
gameScore = 0;

checkCollisions2DBox = function(x1, y1, w1, h1, x2, y2, w2, h2)
	if (
	 x1 < x2 + w2 and
	 x1 + w1 > x2 and
	 y1 < y2 + h2 and
	 y1 + h1 > y2
	) then
		return true;
	else
		return false
	end
end

-- ON_UPDATE event
onUpdate = function(event)
	deltaTimeAvg = deltaTimeAvg + event.deltaTime;
	deltaTimeCount = deltaTimeCount + 1;

	local asset_manager = cmepapi.engine_GetAssetManager(event.raisedFrom);
	local scene_manager = cmepapi.engine_GetSceneManager(event.raisedFrom);

	-- Update frametime counter, recommend to leave this here for debugging purposes
	if deltaTimeCount >= 30 then
		local object = cmepapi.sm_FindObject(scene_manager, "_debug_info");
		cmepapi.textRenderer_UpdateText(object.renderer, "FT: "..tostring(deltaTimeAvg / deltaTimeCount * 1000).." ms");
		
		deltaTimeAvg = 0;
		deltaTimeCount = 0;
	end
	
	-- We can use the math library in here!
	local offset = -100 + math.random(-150, 150);
	
	if gameIsGameOver == false then
		if spawnPipeSinceLast > spawnPipeEvery then
			-- Spawn new pipes
			local object1 = cmepapi.objectFactory_CreateSpriteObject(scene_manager, 1.0, offset / 720, 80 / 1100, 400 / 720, asset_manager, "textures/pipe_down.png");
			cmepapi.sm_AddObject(scene_manager, "sprite_pipe_down"..tostring(spawnPipeLastIdx + 1), object1);

			local object2 = cmepapi.objectFactory_CreateSpriteObject(scene_manager, 1.0, (400 + 200 + offset) / 720, 80 / 1100, 400 / 720, asset_manager, "textures/pipe_up.png");
			cmepapi.sm_AddObject(scene_manager, "sprite_pipe_up"..tostring(spawnPipeLastIdx + 1), object2);

			spawnPipeLastIdx = spawnPipeLastIdx + 1;
			spawnPipeCount = spawnPipeCount + 1;
			spawnPipeSinceLast = 0.0;
		end

		-- Get birb position
		local birb = cmepapi.sm_FindObject(scene_manager, "birb");
		local birbx, birby, birbz = cmepapi.object_GetPosition(birb);

		if spawnPipeCount >= 1 then
			for pipeIdx = spawnPipeFirstIdx, spawnPipeLastIdx, 1 do
				-- Move pipes
				local pipe1 = cmepapi.sm_FindObject(scene_manager, "sprite_pipe_down"..tostring(pipeIdx));
				local pipe2 = cmepapi.sm_FindObject(scene_manager, "sprite_pipe_up"..tostring(pipeIdx));
				local x1, y1, z1 = cmepapi.object_GetPosition(pipe1);
				local x2, y2, z2 = cmepapi.object_GetPosition(pipe2);
				x1 = x1 - 0.05 * event.deltaTime;
				x2 = x2 - 0.05 * event.deltaTime;
				cmepapi.object_Translate(pipe1, x1, y1, z1);
				cmepapi.object_Translate(pipe2, x2, y2, z2);

				-- Check collisions with both pipes
				if checkCollisions2DBox(birbx, birby, 48 / 1100, 33 / 720, x1, y1, 80 / 1100, 400 / 720) or
				   checkCollisions2DBox(birbx, birby, 48 / 1100, 33 / 720, x2, y2, 80 / 1100, 400 / 720)
				then
					gameIsGameOver = true;
					local font = cmepapi.assetManager_GetFont(asset_manager, "fonts/myfont/myfont.fnt");
					local object = cmepapi.objectFactory_CreateTextObject(scene_manager, 0.4, 0.4, 32, "GAME OVER", font);
					cmepapi.sm_AddObject(scene_manager, "text_gameover", object);
					return 0;
				end

				-- Add score score by colliding with a wall after the pipes
				if checkCollisions2DBox(birbx, birby, 48 / 1100, 33 / 720, x2 + 80 / 1100, 0.0, 80 / 1100, 1.0) and pipeIdx > gameLastScoredPipeIdx then
					gameScore = gameScore + 1;
					gameLastScoredPipeIdx = pipeIdx;
					local score_object = cmepapi.sm_FindObject(scene_manager, "text_score");
					cmepapi.textRenderer_UpdateText(score_object.renderer, tostring(gameScore));
				end

				if x1 < (0.0 - 80 / 1100) then
					-- Destroy objects
					cmepapi.sm_RemoveObject(scene_manager, "sprite_pipe_down"..tostring(pipeIdx));
					cmepapi.sm_RemoveObject(scene_manager, "sprite_pipe_up"..tostring(pipeIdx));
					spawnPipeFirstIdx = spawnPipeFirstIdx + 1;
				end
			end
		end
		
		-- Check collisions with floor and roof
		if 	checkCollisions2DBox(birbx, birby, 48 / 1100, 33 / 720, 0.0, -(40 / 720), 1.0, 40 / 720) or
			checkCollisions2DBox(birbx, birby, 48 / 1100, 33 / 720, 0.0, 1.0, 1.0, 40 / 720)
		then
			gameIsGameOver = true;
			local font = cmepapi.assetManager_GetFont(asset_manager, "fonts/myfont/myfont.fnt");
			local object = cmepapi.objectFactory_CreateTextObject(scene_manager, 0.4, 0.4, 32, "GAME OVER", font);
			cmepapi.sm_AddObject(scene_manager, "text_gameover", object);
			return 0;
		end

		local birb = cmepapi.sm_FindObject(scene_manager, "birb");
		local x, y, z = cmepapi.object_GetPosition(birb);
		y = y - birbVelocity * event.deltaTime;
		cmepapi.object_Translate(birb, x, y, z);

		birbVelocity = birbVelocity - 0.5 * event.deltaTime;

		spawnPipeSinceLast = spawnPipeSinceLast + event.deltaTime;
	end

	local camrotx, camroty = cmepapi.sm_GetCameraHVRotation(scene_manager);
	cmepapi.sm_SetCameraHVRotation(scene_manager, camrotx, camroty + 40.0 * event.deltaTime);

	return 0;
end

onInit = function(event)
	cmepapi.engine_SetFramerateTarget(event.raisedFrom, 60); -- VSYNC enabled

	-- Get managers
	local asset_manager = cmepapi.engine_GetAssetManager(event.raisedFrom);
	local scene_manager = cmepapi.engine_GetSceneManager(event.raisedFrom);

	-- Create frametime counter and add it to scene
	local font = cmepapi.assetManager_GetFont(asset_manager, "fonts/myfont/myfont.fnt");
	local object = cmepapi.objectFactory_CreateTextObject(scene_manager, 0.0, 0.0, 18, "test", font);
	cmepapi.sm_AddObject(scene_manager, "_debug_info", object);
	
	-- Add score
	local object = cmepapi.objectFactory_CreateTextObject(scene_manager, 0.5, 0.0, 64, "0", font);
	cmepapi.sm_AddObject(scene_manager, "text_score", object);

	-- Add birb
	local birb = cmepapi.objectFactory_CreateSpriteObject(scene_manager, 0.2, (720 / 2) / 720, 48 / 1100, 33 / 720, asset_manager, "textures/birb.png");
	cmepapi.sm_AddObject(scene_manager, "birb", birb);

	-- Set-up camera
	cmepapi.sm_SetCameraTransform(scene_manager, -5.0, 2.0, 0.0);
	cmepapi.sm_SetCameraHVRotation(scene_manager, 0, 0);

	-- Set-up light
	cmepapi.sm_SetLightTransform(scene_manager, -1,1,0);

	return 0;
end