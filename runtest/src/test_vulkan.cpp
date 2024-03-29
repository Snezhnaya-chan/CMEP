#include <string>

#include "Rendering/VulkanRenderingEngine.hpp"
#include "Logging/Logging.hpp"
#include "boilerplate.hpp"

#define TEST_BUFFER_SIZE 0xffff

Engine::Rendering::VulkanRenderingEngine* rendering_engine;

int boilerplate_rendering_engine_init()
{
	boilerplate_logger_init();
	rendering_engine = new Engine::Rendering::VulkanRenderingEngine();
	
	rendering_engine->init(300, 300, std::string("TESTING"));

	return 0;
}

int boilerplate_rendering_engine_cleanup()
{	
	rendering_engine->cleanup();
	boilerplate_logger_release();

	delete rendering_engine;
	
	return 0;
}

int test_vulkan_init_cleanup()
{
	assert(!boilerplate_rendering_engine_init());
	assert(!boilerplate_rendering_engine_cleanup());

	return 0;
}

int test_vulkan_draw_frame()
{
	assert(!boilerplate_rendering_engine_init());

	rendering_engine->drawFrame();

	assert(!boilerplate_rendering_engine_cleanup());

	return 0;
}

int test_vulkan_buffer_create_cleanup()
{
	assert(!boilerplate_rendering_engine_init());

	Engine::Rendering::VulkanBuffer* test_buffer = rendering_engine->createVulkanBuffer(TEST_BUFFER_SIZE, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 0);
	
	rendering_engine->cleanupVulkanBuffer(test_buffer);
	
	assert(!boilerplate_rendering_engine_cleanup());

	return 0;
}
