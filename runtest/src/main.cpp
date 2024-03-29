#include <string>
#include <map>

#include "Logging/Logging.hpp"

#include "test_vulkan.hpp"
#include "test_logging.hpp"
#include "test_engine.hpp"

std::map<std::string, int(*)(void)> test_map = {
	{"vulkan_init_cleanup", test_vulkan_init_cleanup},
	{"vulkan_draw_frame", test_vulkan_draw_frame},
	{"vulkan_buffer_create_cleanup", test_vulkan_buffer_create_cleanup},

	{"logging_init_release", test_logging_init_release},
	{"logging_log", test_logging_log},
	{"logging_simplelog", test_logging_simplelog},
	{"logging_map_thread_simplelog", test_logging_map_thread_simplelog},

	{"engine", test_engine}
};

int main(int argc, char** argv)
{
	std::string test_name{};
	if(argc > 1)
	{
		test_name = std::string(argv[1]);
	}

	printf("Running test: %s\n", test_name.c_str());

	Logging::GlobalLogger = std::make_unique<Logging::Logger>();

	Logging::GlobalLogger->AddOutputHandle(Logging::LogLevel::Debug3, stdout, true);
	printf("Init'd GlobalLogger\n");

	auto test = test_map.find(test_name);

	if(test_name == std::string("all"))
	{
		int returns = 0;
		for(auto test : test_map)
		{
			
			printf("Running subtest: %s of test '%s'\n", test.first.c_str(), test_name.c_str());
			returns += test.second();
		}
		return returns;
	}

	if(test != test_map.end())
	{
		return test->second();
	}
	else
	{
		printf("TEST NOT FOUND!\n");
		return 1;
	}

	return 0;
}
