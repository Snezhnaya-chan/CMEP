#include "Engine.hpp"

#include "Assets/AssetManager.hpp"
#include "Rendering/MeshBuilders/AxisMeshBuilder.hpp"
#include "Rendering/Renderers/Renderer.hpp"
#include "Rendering/Vulkan/backend.hpp"
#include "Rendering/Vulkan/exports.hpp"

#include "Factories/ObjectFactory.hpp"

#include "Logging/Logging.hpp"

#include "EnumStringConvertor.hpp"
#include "EventHandling.hpp"
#include "Exception.hpp"
#include "GLFW/glfw3.h"
#include "Object.hpp"
#include "SceneManager.hpp"
#include "buildinfo.hpp"
#include "nlohmann/json.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <fstream>
#include <memory>
#include <queue>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

namespace Engine
{
	namespace
	{
		// Utility sleep function
		// algorithm I found somewhere on stackoverflow
		void spinSleep(double seconds)
		{
			static constexpr double nano_to_sec = 1e9;
			static constexpr double spin_init	= 5e-3;

			double	estimate   = spin_init;
			double	mean	   = spin_init;
			double	sumsquares = 0; // also known as m2
			int64_t count	   = 1;

			while (seconds > estimate)
			{
				// Perform measured sleep
				const auto start = std::chrono::steady_clock::now();
				std::this_thread::sleep_for(std::chrono::milliseconds(2));
				const auto end = std::chrono::steady_clock::now();

				// Observed passage of time (as was performed by sleep_for)
				const double observed = static_cast<double>((end - start).count()) /
										nano_to_sec;
				// Substract observed passage of time
				// from total needed amount of sleep
				seconds -= observed;

				count++;
				// change against mean average
				const double delta = observed - mean;
				// create new mean by adding current change to it
				mean += delta / static_cast<double>(count);
				// sum of squares accumulate
				sumsquares += delta * (observed - mean);
				// calculate standard deviation
				const double stddev = sqrt(sumsquares / static_cast<double>(count - 1));
				estimate			= mean + stddev;
			}

			// spin lock
			const auto start = std::chrono::steady_clock::now();
			while (static_cast<double>((std::chrono::steady_clock::now() - start).count()
				   ) / nano_to_sec <
				   seconds)
			{}
		}

		void handleMouseInput(
			Engine*							 origin_engine,
			double							 delta_time,
			const Rendering::Vulkan::Window* window
		)
		{
			const auto& screen_size = window->getFramebufferSize();

			// Initial last position is the center of the window
			static glm::vec<2, double> last_pos = {
				static_cast<double>(screen_size.x) / 2,
				static_cast<double>(screen_size.y) / 2
			};

			// Clamp the mouse event movement to some maximum
			// reduces the chance that the window manager will mess with us
			// (i.e. when gaining focus and the mouse is somewhere far outside the window)
			constexpr double clamp_difference = 128;

			// Check whether we have focus and the mouse is in the "content" area
			if (window->status.is_focus && window->status.is_content)
			{
				if ((window->cursor_position.x - last_pos.x) != 0.0 ||
					(window->cursor_position.y - last_pos.y) != 0.0)
				{
					auto event = EventHandling::Event(
						origin_engine,
						EventHandling::EventType::ON_MOUSEMOVED
					);

					event.mouse.x = std::clamp(
						window->cursor_position.x - last_pos.x,
						-clamp_difference,
						clamp_difference
					);
					event.mouse.y = std::clamp(
						window->cursor_position.y - last_pos.y,
						-clamp_difference,
						clamp_difference
					);

					event.delta_time = delta_time;

					// Fire event, throw exception if return code is non-zero
					int event_return = origin_engine->fireEvent(event);
					ENGINE_EXCEPTION_ON_ASSERT_NOMSG(event_return == 0)

					// Set last position to the current position
					last_pos = window->cursor_position;
				}
			}
		}

		void handleKeyboardInput(
			Engine*										  origin_engine,
			double										  delta_time,
			std::queue<Rendering::Vulkan::KeyboardEvent>& event_queue
		)
		{
			// Handle every event in the queue
			for (; !event_queue.empty(); event_queue.pop())
			{
				const auto& input_event = event_queue.front();

				int event_return = 0;

				switch (input_event.type)
				{
					case Rendering::Vulkan::KeyboardEvent::KEY_PRESS:
					case Rendering::Vulkan::KeyboardEvent::KEY_REPEAT:
					{
						auto event = EventHandling::Event(
							origin_engine,
							EventHandling::EventType::ON_KEYDOWN
						);
						event.keycode	 = static_cast<uint16_t>(input_event.key);
						event.delta_time = delta_time;
						event_return	 = origin_engine->fireEvent(event);
						break;
					}
					case Rendering::Vulkan::KeyboardEvent::KEY_RELEASE:
					{
						auto event = EventHandling::Event(
							origin_engine,
							EventHandling::EventType::ON_KEYUP
						);
						event.keycode	 = static_cast<uint16_t>(input_event.key);
						event.delta_time = delta_time;
						event_return	 = origin_engine->fireEvent(event);
						break;
					}
					default:
					{
						throw ENGINE_EXCEPTION("Unknown input event!");
					}
				}

				if (event_return != 0) { origin_engine->stop(); }
			}
		}
	} // namespace

	void Engine::handleInput(const double delta_time)
	{
		auto* window_data = vk_instance->getWindow();

		handleMouseInput(this, delta_time, window_data);

		handleKeyboardInput(this, delta_time, window_data->keyboard_events);
	}

	void Engine::handleConfig()
	{
		std::ifstream file(config_path);

		nlohmann::json data;
		try
		{
			data = nlohmann::json::parse(file);
		}
		catch (...)
		{
			std::throw_with_nested(ENGINE_EXCEPTION("Exception parsing config!"));
		}

		config = {
			.window =
				{
					.size =
						{data["window"]["sizeX"].get<uint16_t>(),
						 data["window"]["sizeY"].get<uint16_t>()},
					.title = data["window"]["title"].get<std::string>(),
				},
			.framerate_target = data["rendering"]["framerateTarget"].get<uint16_t>(),

			.scene_path	 = data["scene_path"].get<std::string>(),
			.shader_path = data["shader_path"].get<std::string>(),

			.default_scene = data["default_scene"].get<std::string>(),

		};
	}

	void Engine::renderCallback(
		Rendering::Vulkan::CommandBuffer* command_buffer,
		uint32_t						  current_frame,
		void*							  engine
	)
	{
		auto* engine_cast	= static_cast<Engine*>(engine);
		auto& current_scene = engine_cast->scene_manager->getSceneCurrent();

		const auto& objects = current_scene->getAllObjects();

		for (const auto& [name, ptr] : objects)
		{
			try
			{
				ptr->getRenderer()->render(command_buffer, current_frame);
			}
			catch (...)
			{
				std::throw_with_nested(
					ENGINE_EXCEPTION("Caught exception rendering object!")
				);
			}
		}
	}

	void Engine::engineLoop()
	{
		static constexpr double nano_to_msec = 1e6;
		static constexpr double nano_to_sec	 = 1e9;
		static constexpr double sec_to_msec	 = 1e3;

		auto& scene = scene_manager->getSceneCurrent();

		/**
		 * @todo Remove this!
		 */
		// Create axis object
		{
			auto* object = Factories::ObjectFactory::createSceneObject<
				Rendering::Renderer3D,
				Rendering::AxisMeshBuilder>(this, "axis", {}, {});

			scene->addObject("_axis", object);
		}

		// Pre-make ON_UPDATE event so we don't have to create it over and over again in hot loop
		auto on_update_event =
			EventHandling::Event(this, EventHandling::EventType::ON_UPDATE);

		this->logger->simpleLog<decltype(this)>(
			Logging::LogLevel::VerboseDebug,
			"Locked to framerate %u%s",
			config.framerate_target,
			config.framerate_target == 0 ? " (VSYNC)" : ""
		);

		auto build_clock = std::chrono::steady_clock::now();

		// Build scene
		{
			const auto& objects = scene->getAllObjects();

			this->logger->simpleLog<decltype(this)>(
				Logging::LogLevel::Info,
				"Starting scene build (%lu objects)",
				objects.size()
			);
			for (const auto& [name, object] : objects)
			{
				this->logger->simpleLog<decltype(this)>(
					Logging::LogLevel::Debug,
					"Building object '%s'",
					name.c_str()
				);
				object->getMeshBuilder()->build();
			}

			auto scene_build_time =
				static_cast<double>(
					(std::chrono::steady_clock::now() - build_clock).count()
				) /
				nano_to_msec;

			this->logger->simpleLog<decltype(this)>(
				Logging::LogLevel::Info,
				"Scene build finished in %lums",
				static_cast<uint_fast32_t>(scene_build_time)
			);
		}

		// Show window
		auto* glfw_window = vk_instance->getWindow();
		glfw_window->setVisibility(true);

		double	 average_event_total = 0.00;
		uint64_t average_event_count = 1;

		auto prev_clock = std::chrono::steady_clock::now();

		bool first_frame = true;
		// hot loop
		while (!glfw_window->getShouldClose())
		{
			const auto				next_clock = std::chrono::steady_clock::now();
			static constexpr double min_delta  = 0.1 / sec_to_msec;
			static constexpr double max_delta  = 100000.0;
			const double			delta_time = std::clamp(
				   static_cast<double>((next_clock - prev_clock).count()) / nano_to_sec,
				   min_delta,
				   max_delta
			   );
			last_delta_time = delta_time;

			// Check return code of FireEvent (events should return non-zero codes as failure)
			if (!first_frame)
			{
				handleInput(delta_time);

				on_update_event.delta_time = delta_time;
				const auto ret			   = fireEvent(on_update_event);
				if (ret != 0)
				{
					this->logger->simpleLog<decltype(this)>(
						Logging::LogLevel::Error,
						"Firing event ON_UPDATE returned %u! Exiting event-loop",
						ret
					);
					break;
				}
			}
			first_frame = false;

			const auto event_clock = std::chrono::steady_clock::now();

			// Render
			glfw_window->drawFrame();

			const auto draw_clock = std::chrono::steady_clock::now();

			// Sync with glfw event loop
			glfwPollEvents();

			const auto poll_clock = std::chrono::steady_clock::now();

			const double event_time =
				static_cast<double>((event_clock - next_clock).count()) / nano_to_msec;
			const double draw_time =
				static_cast<double>((draw_clock - event_clock).count()) / nano_to_msec;
			const double poll_time =
				static_cast<double>((poll_clock - draw_clock).count()) / nano_to_msec;

			const auto time_sum = event_time + draw_time + poll_time;

			average_event_total += event_time;
			average_event_count++;

			constexpr double limits[] = {12.0, 19.0, 8.0};

			if (event_time > limits[0] || time_sum > limits[1] || time_sum < limits[2])
			{
				this->logger->simpleLog<decltype(this)>(
					Logging::LogLevel::Warning,
					"delta %lf sum %lf (event %lf draw %lf poll %lf)",
					delta_time * sec_to_msec,
					time_sum,
					event_time,
					draw_time,
					poll_time
				);
			}

			const auto	 frame_clock = std::chrono::steady_clock::now();
			const double sleep_secs =
				1.0 / config.framerate_target -
				static_cast<double>((frame_clock - next_clock).count()) / nano_to_sec;
			// spin sleep if sleep necessary and VSYNC disabled
			if (sleep_secs > 0 && config.framerate_target != 0) { spinSleep(sleep_secs); }

			prev_clock = next_clock;
		}

		this->logger->simpleLog<decltype(this)>(
			Logging::LogLevel::Debug,
			"Closing engine (eventtime avg %lf)",
			average_event_total / static_cast<double>(average_event_count)
		);
	}

	int Engine::fireEvent(EventHandling::Event event)
	{
		int sum = 0;

		const auto& current_scene = scene_manager->getSceneCurrent();

		// Get all handlers that match this type
		auto [handlers_begin, handlers_end] =
			current_scene->lua_event_handlers.equal_range(event.event_type);

		for (auto handler = handlers_begin; handler != handlers_end; ++handler)
		{
			try
			{
				// Call the handler
				sum += handler->second(&event);
			}
			catch (const std::exception& e)
			{
				// Get the EventType as a string
				std::string_view event_type_str = EnumStringConvertor(event.event_type);

				this->logger->simpleLog<decltype(this)>(
					Logging::LogLevel::Exception,
					"Caught exception trying to fire event '%s'! e.what(): %s",
					event_type_str.data(),
					unrollExceptions(e).c_str()
				);
			}
		}

		return sum;
	}

	Engine::Engine(std::shared_ptr<Logging::Logger>& with_logger) : logger(with_logger)
	{}

	Engine::~Engine()
	{
		this->logger->simpleLog<decltype(this)>(
			Logging::LogLevel::Info,
			"Destructor called"
		);

		scene_manager.reset();

		asset_manager.reset();

		pipeline_manager.reset();

		delete vk_instance;
	}

	void Engine::run()
	{
		const auto init_start = std::chrono::steady_clock::now();

		this->logger->mapCurrentThreadToName("engine");

		// Engine info printout
		this->logger->simpleLog<decltype(this)>(
			Logging::LogLevel::Info,
			"build info:\n////\nRunning %s\nCompiled by %s\n////",
			buildinfo_build,
			buildinfo_compiledby
		);

		// Load configuration
		try
		{
			handleConfig();
		}
		catch (...)
		{
			std::throw_with_nested(ENGINE_EXCEPTION("Exception parsing config!"));
		}

		asset_manager = std::make_shared<AssetManager>(this);
		scene_manager = std::make_shared<SceneManager>(this);

		vk_instance = new Rendering::Vulkan::Instance(
			this->logger,
			{
				config.window.size,
				config.window.title,
				{
					{GLFW_VISIBLE, GLFW_FALSE},
					{GLFW_RESIZABLE, GLFW_TRUE},
				},
			}
		);

		vk_instance->getWindow()->setRenderCallback(Engine::renderCallback, this);

		pipeline_manager = std::make_shared<Rendering::Vulkan::PipelineManager>(
			logger,
			vk_instance,
			config.game_path + config.shader_path
		);

		scene_manager->setSceneLoadPrefix(config.game_path + config.scene_path);
		scene_manager->loadScene(config.default_scene);
		scene_manager->setScene(config.default_scene);

		// auto oninit_start = std::chrono::steady_clock::now();

		/**
		 * @todo Fire ON_INIT on scene load!
		 */
		// Fire ON_INIT event
		// auto on_init_event = EventHandling::Event(this, EventHandling::EventType::ON_INIT);
		// int on_init_event_ret = fireEvent(on_init_event);

		const auto end = std::chrono::steady_clock::now();

		// Measure and log ON_INIT time
		static constexpr double nano_to_msec = 1.e6;
		double init_total = static_cast<double>((end - init_start).count()) / nano_to_msec;
		/* double oninit_total = static_cast<double>((end - oninit_start).count()) /
							  nano_to_msec; */

		this->logger->simpleLog<decltype(this)>(
			Logging::LogLevel::Info,
			"Initialized in %.3lfms", // "Initialized in %.3lfms (of that %.3lfms ON_INIT)",
			init_total				  //,
									  // oninit_total
		);

		/* if (on_init_event_ret != 0)
		{
			this->logger->simpleLog<decltype(this)>(
				Logging::LogLevel::VerboseDebug,
				"ON_INIT returned non-zero %u",
				on_init_event_ret
			);
			return;
		} */

		engineLoop();
	}

	[[noreturn]] void Engine::throwTest()
	{
		throw std::runtime_error("BEBEACAC");
	}

	void Engine::stop()
	{
		vk_instance->getWindow()->setShouldClose(true);
	}

	void Engine::configFile(std::string path)
	{
		config_path = std::move(path);
	}
} // namespace Engine
