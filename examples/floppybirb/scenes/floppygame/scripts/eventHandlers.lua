-- Include modules
require("util")

-----------------------
---->  Game data  <----

-- Debugging data
local deltaTime_accum = 0.0
local deltaTime_count = 0
-- Minimum & Maximum reached deltatime
local deltaTime_max = 0.0
local deltaTime_min = 2000.0

-- Related to spawning pipes
local spawn_pipe_every = 4.0 -- Configurable spawn rate
local spawn_pipe_last_idx = 0
local spawn_pipe_first_idx = 1
local spawn_pipe_count = 0
local spawn_pipe_since_last = spawn_pipe_every - 0.1

-- Pipe sprite size in pixels (note: pipe original size is 110x338)
local pipe_x_size = 110
local pipe_y_size = 450

-- Pipe behavior
local pipe_spacing_start = 200 -- Configurable pipe spacing (between top and bottom)
local pipe_spacing = pipe_spacing_start
local pipe_move_speed = 0.135 -- Configurable pipe move speed (/speed of flying)

-- Birb sprite size in pixels
local birb_x_size = 72
local birb_y_size = 44

-- Birb behavior
local birb_jump_velocity = 0.46 -- Configurable velocity set on jump
local birb_gravity = 0.89 -- Configurable gravity
local birb_velocity = 0.25 -- Internal velocity value
local birb_is_velociting = false -- Internal is-jumping check

-- Game state
local game_gameover_state = false
local game_midgameover_state = false
local game_last_scored_pipe_idx = 0 -- Index of last scored pipe
local game_score = 0 -- The score

---->  Game data  <----
-----------------------

-----------------------
--> Local functions <--

--local checkCollisions2DBox = function(x1, y1, w1, h1, x2, y2, w2, h2)
--	if ((x1 < x2 + w2) and (x1 + w1 > x2) and (y1 < y2 + h2) and (y1 + h1 > y2)) then
--		return true
--	else
--		return false
--	end
--end

-- Scrolls ground objects
local handleGroundLayer = function(scene, event, layer)
	local ground1 = scene:FindObject(layer.."1")
	local ground2 = scene:FindObject(layer.."2")
	local ground3 = scene:FindObject(layer.."3")

	local g1_x, g1_y, g1_z = ground1:GetPosition()
	local g2_x, g2_y, g2_z = ground2:GetPosition()
	local g3_x, g3_y, g3_z = ground3:GetPosition()
	
	-- Move ground (slightly faster than pipes)
	g1_x = g1_x - (pipe_move_speed * 1.1) * event.deltaTime
	g2_x = g2_x - (pipe_move_speed * 1.1) * event.deltaTime
	g3_x = g3_x - (pipe_move_speed * 1.1) * event.deltaTime
	
	-- Check if ground is still on-screen
	if	(g1_x > util.pxToScreenX(-630))
	then
	-- If yes just move it
		ground1:SetPosition(g1_x, g1_y, g1_z)
	-- If it's not, put it on the other side
	else
		ground1:SetPosition(g3_x + util.pxToScreenX(630), g1_y, g1_z)
	end

	if (g2_x > util.pxToScreenX(-630))
	then
		ground2:SetPosition(g2_x, g2_y, g2_z)
	else
		ground2:SetPosition(g1_x + util.pxToScreenX(630), g2_y, g2_z)
	end

	if (g3_x > util.pxToScreenX(-630))
	then
		ground3:SetPosition(g3_x, g3_y, g3_z)
	else
		ground3:SetPosition(g2_x + util.pxToScreenX(630), g3_y, g3_z)
	end
end

local gameOnHandleGrounds = function(scene, event)
	handleGroundLayer(scene, event, "ground_top")
	handleGroundLayer(scene, event, "ground_bottom")
end

local checkCollisionsGrounds = function(birbx, birby)
	return (util.checkCollisions2DBox(birbx, 	birby,								util.pxToScreenX(birb_x_size),	util.pxToScreenY(birb_y_size),
									  0.0,		0.0,								1.0,						util.pxToScreenY(60)) or
			util.checkCollisions2DBox(birbx, 	birby,								util.pxToScreenX(birb_x_size), 	util.pxToScreenY(birb_y_size),
								 	  0.0,		util.pxToScreenY(util.screen_size_y - 60), 	1.0, 					 	util.pxToScreenY(60)))
end

local gameOnGameOver = function(asset_manager, scene_manager)
	game_midgameover_state = true
	birb_velocity = -0.4

	cmepmeta.logger.SimpleLog(string.format("Game over!"))

	local font = asset_manager:GetFont("myfont")
	local object = cmepapi.ObjectFactoryCreateTextObject(scene_manager, "GAME OVER", font)
	object:SetPosition(0.34, 0.45, -0.01)
	object:SetSize(64, 64, 1.0)
	scene_manager:GetSceneCurrent():AddObject("text_gameover", object)
end

--> Local functions <--
-----------------------

-----------------------
--->  Game events  <---


-- ON_MOUSEMOVED event
-- 
-- this event is called when the mouse moved
--
-- while specifying event handlers is optional
-- it is left here for illustration purposes
-- (events for which no event handler is specified are discarded)
-- 
onMouseMoved = function(event)
	return 0
end

-- ON_KEYDOWN event
-- 
-- this event is called every time the engine receives a keypress
-- for non-toggleable keys this event may be fired multiple times
-- always check whether a key was unpressed when necessary
-- 
onKeyDown = function(event)
	-- Check for space press
	--
	-- note: it is not recommended to convert event.keycode with string.char
	--       as it may fail when encoutering a non-character value
	--
	-- note: event.keycode is a 16-bit value
	--       ASCII characters match keycode if converted with string.byte
	--       for function key keycodes search for "GLFW key tokens"
	--
	if event.keycode == string.byte(' ') and
	   game_midgameover_state == false
	then
		if birb_is_velociting == false then
			birb_velocity = birb_jump_velocity
			birb_is_velociting = true
			return 0
		end
	end

	-- Stop engine if ESC is pressed
	-- 256 is the keycode of the ESC key
	--
	if event.keycode == 256 then
		event.engine:Stop()
		return 0
	end

	return 0
end

-- ON_KEYUP event
-- 
-- this event is called exactly once for every release of a key
-- 
onKeyUp = function(event)
	-- event.keycode is the same for ON_KEYUP and ON_KEYDOWN events
	--
	if event.keycode == string.byte(' ') then
		birb_is_velociting = false
	end
end

-- Minimum & Maximum reached deltatime
local deltaTime_max = 0.0
local deltaTime_min = 2000.0

-- ON_UPDATE event
-- 
-- called every frame
--
-- event.deltaTime (float) contains the delta time in microseconds
-- and is the period between the last onUpdate and the current one
--
onUpdate = function(event)
	deltaTime_accum = deltaTime_accum + event.deltaTime

	deltaTime_max = math.max(deltaTime_max, event.deltaTime)
	deltaTime_min = math.min(deltaTime_min, event.deltaTime)

	deltaTime_count = deltaTime_count + 1

	local asset_manager = event.engine:GetAssetManager()
	local scene_manager = event.engine:GetSceneManager()
	local scene = scene_manager:GetSceneCurrent()

	-- Updates frametime counter, recommend to leave this here for debugging purposes
	if deltaTime_accum >= 1.0 then
		local deltaTime_avg = deltaTime_accum / deltaTime_count
		--cmepmeta.logger.SimpleLog(string.format("Frametime is: %f ms!", deltaTime_accum / deltaTime_count * 1000))
		local object = scene:FindObject("_debug_info")
		--cmepapi.TextRendererUpdateText(object.renderer, string.format("avg: %fms\nmin: %fms\nmax: %fms", deltaTime_avg * 1000, deltaTime_min * 1000, deltaTime_max * 1000))
		
		cmepapi.RendererSupplyText(object.renderer, string.format("avg: %fms\nmin: %fms\nmax: %fms", deltaTime_avg * 1000, deltaTime_min * 1000, deltaTime_max * 1000))

		deltaTime_min = 2000.0
		deltaTime_max = 0.0
		deltaTime_accum = 0
		deltaTime_count = 0
	end

	if (game_gameover_state == false and game_midgameover_state == false) then
		-- Spawn new pipes
		if spawn_pipe_since_last > spawn_pipe_every then
			-- Constant offset to center the pipes
			local const_pipe_y_offset = -100
			-- We can use the math library in here!
			local pipe_y_offset = const_pipe_y_offset + math.random(-15, 15) * 10
			--local pipe_y_offset = const_pipe_y_offset
			
			local pipe_id = tostring(spawn_pipe_last_idx + 1)

			scene:AddTemplatedObject("sprite_pipe_down"..pipe_id, "pipe_down")
			local pipe1 = scene:FindObject("sprite_pipe_down"..pipe_id)
			pipe1:SetPosition(1.0, util.pxToScreenY(pipe_y_offset - (pipe_spacing / 2)), -0.15)
			pipe1:SetSize(util.pxToScreenX(pipe_x_size), util.pxToScreenY(pipe_y_size), 1.0)

			scene:AddTemplatedObject("sprite_pipe_up"..pipe_id, "pipe_up")
			local pipe2 = scene:FindObject("sprite_pipe_up"..pipe_id)
			pipe2:SetPosition(1.0, util.pxToScreenY(pipe_y_size + (pipe_spacing / 2) + pipe_y_offset), -0.15)
			pipe2:SetSize(util.pxToScreenX(pipe_x_size), util.pxToScreenY(pipe_y_size), 1.0)

			spawn_pipe_last_idx = spawn_pipe_last_idx + 1
			spawn_pipe_count = spawn_pipe_count + 1
			spawn_pipe_since_last = 0.0
		end

		-- Get birb position
		local birb = scene:FindObject("birb")
		local birbx, birby, birbz = birb:GetPosition()

		if spawn_pipe_count >= 1 then
			for pipeIdx = spawn_pipe_first_idx, spawn_pipe_last_idx, 1 do
				-- Move pipes
				local pipe1 = scene:FindObject("sprite_pipe_down"..tostring(pipeIdx))
				local pipe2 = scene:FindObject("sprite_pipe_up"..tostring(pipeIdx))
				local x1, y1, z1 = pipe1:GetPosition()
				local x2, y2, z2 = pipe2:GetPosition()
				x1 = x1 - pipe_move_speed * event.deltaTime
				x2 = x2 - pipe_move_speed * event.deltaTime
				pipe1:SetPosition(x1, y1, z1)
				pipe2:SetPosition(x2, y2, z2)

				-- Check collisions with both pipes
				if util.checkCollisions2DBox(birbx, birby, util.pxToScreenX(birb_x_size), util.pxToScreenY(birb_y_size), x1, y1, util.pxToScreenX(pipe_x_size), util.pxToScreenY(pipe_y_size)) or -- pipe 1
				   util.checkCollisions2DBox(birbx, birby, util.pxToScreenX(birb_x_size), util.pxToScreenY(birb_y_size), x2, y2, util.pxToScreenX(pipe_x_size), util.pxToScreenY(pipe_y_size))	 -- pipe 2
				then
					gameOnGameOver(asset_manager, scene_manager)
					return 0
				end

				-- Add score by colliding with a wall after the pipes
				if util.checkCollisions2DBox(birbx, birby, util.pxToScreenX(birb_x_size), util.pxToScreenY(birb_y_size), x2 + util.pxToScreenX(80), 0.0, util.pxToScreenX(80), 1.0) and
				   pipeIdx > game_last_scored_pipe_idx
				then
					game_score = game_score + 1
					game_last_scored_pipe_idx = pipeIdx
					local score_object = scene:FindObject("text_score")
					cmepapi.RendererSupplyText(score_object.renderer, tostring(game_score))

					pipe_move_speed = pipe_move_speed * 1.005 -- Increase pipe move speed
					pipe_spacing = pipe_spacing * 0.9990 -- Decrease pipe spacing (between top and bottom pipe)
					spawn_pipe_every = spawn_pipe_every * 0.9950 -- Increase spawn rate (decrease timeout between spawns)
				end

				if x1 < (0.0 - util.pxToScreenX(pipe_x_size + 5)) then
					-- Destroy objects
					scene:RemoveObject("sprite_pipe_down"..tostring(pipeIdx))
					scene:RemoveObject("sprite_pipe_up"..tostring(pipeIdx))
					spawn_pipe_first_idx = spawn_pipe_first_idx + 1
				end
			end
		end
		
		-- Check collisions with grounds
		if checkCollisionsGrounds(birbx, birby)
		then
			gameOnGameOver(asset_manager, scene_manager)
			return 0
		end

		-- Fall birb
		-- we already have birbx/y/z from before
		birby = birby - birb_velocity * event.deltaTime
		birb:SetPosition(birbx, birby, birbz)

		gameOnHandleGrounds(scene, event)

		-- Add birb_gravity to birb velocity
		birb_velocity = birb_velocity - birb_gravity * event.deltaTime
		spawn_pipe_since_last = spawn_pipe_since_last + event.deltaTime

	elseif (game_midgameover_state == true and game_gameover_state == false) then

		local birb = scene:FindObject("birb")
		local birbx, birby, birbz = birb:GetPosition()

		birby = birby - birb_velocity * event.deltaTime
		birb:SetPosition(birbx, birby, birbz)
			
		-- Check collisions with grounds
		if checkCollisionsGrounds(birbx, birby)
		then
			game_gameover_state = true
		end
	end

	return 0
end

-- ON_INIT event
--
-- this event is fired after the scene is loaded and before the first rendered frame
-- perform initialization here
--
onInit = function(event)
	-- By default the framerate target is 0 (VSYNC)
	-- uncomment this to set the desired framerate target
	-- the engine will spinsleep until the target is reached
	--
	--event.engine:SetFramerateTarget(30)

	-- Get managers
	local asset_manager = event.engine:GetAssetManager()
	local scene_manager = event.engine:GetSceneManager()
	local scene = scene_manager:GetSceneCurrent();

	-- Create frametime counter and add it to scene
	local font = asset_manager:GetFont("myfont")
	local object = cmepapi.ObjectFactoryCreateTextObject(scene_manager, "avg: \nmin: \nmax: ", font)
	object:SetPosition(0.0, 0.0, -0.01)
	object:SetSize(24, 24, 1.0)
	scene:AddObject("_debug_info", object)

	--cmepmeta.logger.SimpleLog(string.format("package returns %u", util.testfn()));

	-- Add score
	local object = cmepapi.ObjectFactoryCreateTextObject(scene_manager, "0", font)
	object:SetPosition(0.5, 0.0, -0.01)
	object:SetSize(64, 64, 1.0)
	scene:AddObject("text_score", object)

	-- Add top grounds
	local ground_top1 = cmepapi.ObjectFactoryCreateSpriteObject(scene_manager, asset_manager, "ground_top")
	ground_top1:SetPosition(0.0, 0.0, -0.1)
	ground_top1:SetSize(util.pxToScreenX(630), util.pxToScreenY(60), 1)
	scene:AddObject("ground_top1", ground_top1)
	
	local ground_top2 = cmepapi.ObjectFactoryCreateSpriteObject(scene_manager, asset_manager, "ground_top")
	ground_top2:SetPosition(util.pxToScreenX(630), 0.0, -0.1)
	ground_top2:SetSize(util.pxToScreenX(630), util.pxToScreenY(60), 1)
	scene:AddObject("ground_top2", ground_top2)

	local ground_top3 = cmepapi.ObjectFactoryCreateSpriteObject(scene_manager, asset_manager, "ground_top")
	ground_top3:SetPosition(util.pxToScreenX(630) * 2, 0.0, -0.1)
	ground_top3:SetSize(util.pxToScreenX(630), util.pxToScreenY(60), 1)
	scene:AddObject("ground_top3", ground_top3)

	-- Add bottom grounds
	local ground_bottom1 = cmepapi.ObjectFactoryCreateSpriteObject(scene_manager, asset_manager, "ground_bottom")
	ground_bottom1:SetPosition(0.0, util.pxToScreenY(util.screen_size_y - 60), -0.1)
	ground_bottom1:SetSize(util.pxToScreenX(630), util.pxToScreenY(60), 1)
	scene:AddObject("ground_bottom1", ground_bottom1)
	
	local ground_bottom2 = cmepapi.ObjectFactoryCreateSpriteObject(scene_manager, asset_manager, "ground_bottom")
	ground_bottom2:SetPosition(util.pxToScreenX(630), util.pxToScreenY(util.screen_size_y - 60), -0.1)
	ground_bottom2:SetSize(util.pxToScreenX(630), util.pxToScreenY(60), 1)
	scene:AddObject("ground_bottom2", ground_bottom2)

	local ground_bottom3 = cmepapi.ObjectFactoryCreateSpriteObject(scene_manager, asset_manager, "ground_bottom")
	ground_bottom3:SetPosition(util.pxToScreenX(630) * 2, util.pxToScreenY(util.screen_size_y - 60), -0.1)
	ground_bottom3:SetSize(util.pxToScreenX(630), util.pxToScreenY(60), 1)
	scene:AddObject("ground_bottom3", ground_bottom3)

	-- Set-up camera
	-- (this is essentially unnecessary for 2D-only scenes)
	scene_manager:SetCameraTransform(0.0, 0.0, 0.0)
	scene_manager:SetCameraHVRotation(0, 0)

	-- Set-up light
	-- (unnecessary for scenes that don't employ renderers or shaders with lighting)
	scene_manager:SetLightTransform(-1, 1, 0)

	return 0
end

--->  Game events  <---
-----------------------
