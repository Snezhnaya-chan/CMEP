#pragma once

#include <unordered_map>
#include <functional>
#include <memory>
#include <vector>
#include <optional>
#include <atomic>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include "glfw/include/GLFW/glfw3.h"

#include "vk_mem_alloc.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "../EngineCore/include/Rendering/tinyobjloader/tiny_obj_loader.h"
#include "lua.hpp"

#pragma region forward decls

namespace Logging
{
	class Logger;
}

namespace Engine
{
	class AssetManager;
	class Object;
	namespace Rendering
	{
		struct VulkanBuffer;
		struct VulkanImage;
		struct VulkanTextureImage;
		struct VulkanPipeline;

		typedef enum class Texture_InitFiletypeEnum : int
		{
			FILE_PNG = 3
		} Texture_InitFiletype;

		class TextRenderer;
		class SpriteRenderer;
		class MeshRenderer;
		class AxisRenderer;
		class Texture;
		class VulkanRenderingEngine;
		class Font;
		class Mesh;
	}
	class SceneManager;
}

#pragma endregion

#pragma region Logging.hpp

	namespace Logging
	{
		enum class LogLevel
		{
			Debug3 = 0,
			Debug2,
			Debug1,
			Info,
			Success,
			Warning,
			Error,
			Exception
		};

		struct LoggerInternalMapping
		{
			LogLevel min_level;
			FILE* handle;
			bool hasStartedLogging;
			bool useColors;
		};

		class Logger
		{
		private:
			std::vector<LoggerInternalMapping*> outputs;
			std::map<int16_t, std::string> threadid_name_map;
			std::atomic<bool> threadLocked;

		public:
			Logger() : threadLocked(false) {}

			void AddOutputHandle(LogLevel min_level, FILE* handle, bool useColors = false);
			void MapCurrentThreadToName(std::string name);

			void StartLog(LogLevel level);
			void Log(const char* format, ...);
			void StopLog();

			void SimpleLog(LogLevel level, const char* format, ...);
		};
	}

#pragma endregion

namespace Engine
{
#pragma region InternalEngineObject.hpp

	class InternalEngineObject
    {
    protected:
    public:
        std::shared_ptr<Logging::Logger> logger{};
        
        InternalEngineObject() {}
        //~InternalEngineObject() {}

        void UpdateHeldLogger(std::shared_ptr<Logging::Logger> new_logger)
        {
            this->logger = new_logger;
        }
    };

#pragma endregion

	namespace Rendering
	{

#pragma region Mesh.hpp

	class Mesh final : public InternalEngineObject
	{
	private:
	public:
		std::vector<glm::vec3> mesh_vertices;
		std::vector<glm::vec2> mesh_uvs;
		std::vector<glm::vec3> mesh_normals;

		std::vector<glm::vec3> mesh_tangents;
		std::vector<glm::vec3> mesh_bitangents;
		
		std::vector<unsigned int> matids;
		std::vector<tinyobj::material_t> materials;
		std::vector<glm::vec3> mesh_ambient;
		std::vector<glm::vec3> mesh_diffuse;
		std::vector<glm::vec3> mesh_specular;
		std::vector<float> mesh_dissolve;
		std::vector<glm::vec3> mesh_emission;

		std::vector<std::shared_ptr<Rendering::Texture>> diffuse_textures;
		// std::vector<std::shared_ptr<Rendering::Texture>> bump_textures;
		// std::vector<std::shared_ptr<Rendering::Texture>> roughness_textures;
		// std::vector<std::shared_ptr<Rendering::Texture>> reflective_textures;

		Mesh();
		~Mesh();

		void CreateMeshFromObj(std::string path);
	};

#pragma endregion

#pragma region Font.hpp

		struct FontChar
		{
			int x, y, width, height, xoffset, yoffset, xadvance, page, channel;
		};

		class Font final : public InternalEngineObject
		{
		private:
			AssetManager* asset_manager;
			std::string fntfile;
			unsigned int char_count = 0;

			// Data from fnt file
			std::unordered_map<std::string, std::string> info;
			std::unordered_map<int, std::shared_ptr<Texture>> pages;
			std::unordered_map<int, FontChar> chars;

			void EvalBmfont(FILE* file);
			void EvalBmfontLine(int type, char* data);
		public:
			Font(AssetManager* managed_by = nullptr);
			~Font();

			int Init(std::string path);

			FontChar* GetChar(char ch);
			std::shared_ptr<Texture> GetPageTexture(int page);
			std::string* GetFontInfoParameter(std::string name);
		};

#pragma endregion

#pragma region Texture.hpp

		class Texture final : public InternalEngineObject
		{
		private:
			std::vector<unsigned char> data;
			unsigned int x = 0, y = 0;
			int color_fmt = 4;
			unsigned int texture = 0;

			VulkanBuffer* staging_buffer = nullptr;
			bool managedStagingBuffer = false;
			VulkanTextureImage* textureImage = nullptr;

		public:
			Texture();
			~Texture();

			void UsePremadeStagingBuffer(VulkanBuffer* staging_buffer);

			int InitRaw(std::vector<unsigned char> raw_data, int color_format, unsigned int xsize, unsigned int ysize);
			int InitFile(Texture_InitFiletype filetype, std::string path, unsigned int sizex = 0, unsigned int sizey = 0);

			void GetSize(unsigned int& x, unsigned int& y) const noexcept;
			const std::vector<unsigned char> GetData() const;
			unsigned int GetTexture() const noexcept;
			VulkanTextureImage* GetTextureImage() const noexcept;
			int GetColorFormat() const noexcept;
		};

#pragma endregion

#pragma region IRenderer.hpp

		class IRenderer : public InternalEngineObject
		{
		protected:
			glm::vec3 _pos = glm::vec3();
			glm::vec3 _size = glm::vec3();
			glm::vec3 _rotation = glm::vec3();
			
			glm::vec3 _parent_pos = glm::vec3();
			glm::vec3 _parent_size = glm::vec3();
			glm::vec3 _parent_rotation = glm::vec3();

			uint_fast16_t _screenx = 0, _screeny = 0;

			bool has_updated_mesh = false;

		public:
			std::weak_ptr<::Engine::SceneManager> scene_manager{};

			IRenderer() {};
			virtual ~IRenderer() {};

			virtual void Update(glm::vec3 pos, glm::vec3 size, glm::vec3 rotation, uint_fast16_t screenx, uint_fast16_t screeny, glm::vec3 parent_position, glm::vec3 parent_rotation, glm::vec3 parent_size) = 0;
			
			virtual void UpdateMesh() = 0;
			
			virtual void Render(VkCommandBuffer commandBuffer, uint32_t currentFrame) = 0;
		};

#pragma endregion

#pragma region TextRenderer.hpp

		/// <summary>
		/// Implementation of <seealso cref="IRenderer"/> for text renderables.
		/// </summary>
		/// <inheritdoc cref="IRenderer"/>
	class TextRenderer final : public IRenderer
	{
	private:
		size_t vbo_vert_count = 0;

		std::string text = "";

		VulkanPipeline* pipeline = nullptr;
		VulkanBuffer* vbo = nullptr;
		VulkanTextureImage* textureImage = nullptr;

		Rendering::Font* font;

	public:
		TextRenderer();
		~TextRenderer();

		void Update(glm::vec3 pos, glm::vec3 size, glm::vec3 rotation, uint_fast16_t screenx, uint_fast16_t screeny, glm::vec3 parent_position, glm::vec3 parent_rotation, glm::vec3 parent_size) override;

		int UpdateFont(Rendering::Font* const font);
		
		int UpdateText(const std::string text);
		
		void UpdateMesh() override;
		void Render(VkCommandBuffer commandBuffer, uint32_t currentFrame) override;
	};

#pragma endregion
		
#pragma region SpriteRenderer.hpp

		/// <summary>
		/// Implementation of <seealso cref="IRenderer"/> for 2D sprite renderables.
		/// </summary>
		/// <inheritdoc cref="IRenderer"/>
		class SpriteRenderer final : public IRenderer
		{
		private:
			/// <summary>
			/// GL Vertex Array Object
			/// </summary>
			unsigned int vao = 0;
			/// <summary>
			/// GL Vertex Buffer Object
			/// </summary>
			VulkanBuffer* vbo = nullptr;
			//unsigned int vbo = 0;

			glm::mat4 matMVP{};

			VulkanPipeline* pipeline = nullptr;
			//std::unique_ptr<Rendering::Shader> program;
			std::shared_ptr<const Rendering::Texture> texture;

		public:
			SpriteRenderer();
			~SpriteRenderer();

			void Update(glm::vec3 pos, glm::vec3 size, glm::vec3 rotation, uint_fast16_t screenx, uint_fast16_t screeny, glm::vec3 parent_position, glm::vec3 parent_rotation, glm::vec3 parent_size) override;
			void UpdateTexture(std::shared_ptr<Rendering::Texture> texture);
			void UpdateMesh() override;

			void Render(VkCommandBuffer commandBuffer, uint32_t currentFrame) override;
		};

#pragma endregion

#pragma region MeshRenderer.hpp

		/// <summary>
		/// Implementation of <seealso cref="IRenderer"/> for custom mesh renderables.
		/// </summary>
		/// <inheritdoc cref="IRenderer"/>
		class MeshRenderer final : public IRenderer
		{
		private:
			size_t vbo_vert_count = 0;

			VulkanPipeline* pipeline = nullptr;
			VulkanBuffer* vbo = nullptr;

			glm::mat4 matM{};
			glm::mat4 matV{};
			glm::mat4 matMV{};
			glm::mat3 matMV3x3{};
			glm::mat4 matMVP{};

			/// <summary>
			/// Currently used shader
			/// </summary>
			std::unique_ptr<const Rendering::Texture> texture;

			bool has_updated_meshdata = false;

			std::shared_ptr<Mesh> mesh;
		public:
			MeshRenderer();
			~MeshRenderer();

			void AssignMesh(std::shared_ptr<Mesh> new_mesh);

			void UpdateTexture(const Rendering::Texture* texture);
			void Update(glm::vec3 pos, glm::vec3 size, glm::vec3 rotation, uint_fast16_t screenx, uint_fast16_t screeny, glm::vec3 parent_position, glm::vec3 parent_rotation, glm::vec3 parent_size) override;
			void UpdateMesh() override;

			void Render(VkCommandBuffer commandBuffer, uint32_t currentFrame) override;
		};

#pragma endregion

#pragma region VulkanRenderingEngine.hpp

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	struct GLFWwindowData
	{
		GLFWwindow* window;
		unsigned int windowX = 0, windowY = 0;
		std::string windowTitle;
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct VulkanBuffer
	{
		VkBuffer buffer;
		VkDeviceMemory bufferMemory;
		void* mappedMemory;
	};

	struct VulkanImage
	{
		VkImage image;
		VkDeviceMemory imageMemory;
		VkFormat imageFormat;
		VkImageView imageView;
	};

	struct VulkanTextureImage
	{
		VulkanImage* image;
		VkSampler textureSampler;
	};

	struct VulkanDescriptorLayoutSettings
	{
		std::vector<uint32_t> binding;
		std::vector<VkDescriptorType> types;
		std::vector<VkShaderStageFlags> stageFlags;
		std::vector<uint32_t> descriptorCount;
	};

	struct VulkanPipelineSettings
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssembly;
		VkViewport viewport;
		VkRect2D scissor;
		VkPipelineViewportStateCreateInfo viewportState;
		VkPipelineRasterizationStateCreateInfo rasterizer;
		VkPipelineMultisampleStateCreateInfo multisampling;
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		VkPipelineColorBlendStateCreateInfo colorBlending;
		VkPipelineDepthStencilStateCreateInfo depthStencil;

		VulkanDescriptorLayoutSettings descriptorLayoutSettings;
	};

	struct VulkanPipeline
	{
		VkPipeline pipeline;
		VkPipelineLayout vkPipelineLayout;
		VkDescriptorPool vkDescriptorPool;
		VkDescriptorSetLayout vkDescriptorSetLayout;
		std::vector<VkDescriptorSet> vkDescriptorSets{};
		std::vector<VulkanBuffer*> uniformBuffers;
	};

	enum VulkanTopologySelection
	{
		VULKAN_RENDERING_ENGINE_TOPOLOGY_TRIANGLE_LIST,
		VULKAN_RENDERING_ENGINE_TOPOLOGY_LINE_LIST
	};

	struct RenderingVertex {
		glm::vec3 pos{};
		glm::vec3 color{};
		glm::vec2 texcoord{};
		glm::vec3 normal{};

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription{};

			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(RenderingVertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};
			
			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(RenderingVertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(RenderingVertex, color);

			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(RenderingVertex, texcoord);
			
			attributeDescriptions[3].binding = 0;
			attributeDescriptions[3].location = 3;
			attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[3].offset = offsetof(RenderingVertex, normal);

			return attributeDescriptions;
		}
	};

	class VulkanRenderingEngine
	{
	private:
		const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

		GLFWwindow* window = nullptr;
		unsigned int windowX = 0, windowY = 0;
		std::string windowTitle{};

		static std::vector<char> readShaderFile(std::string path);

		uint32_t currentFrame = 0;
		bool framebufferResized = false;

		// Vulkan instance
		VkInstance vkInstance = VK_NULL_HANDLE;
		
		// Queues
		QueueFamilyIndices graphicsQueueIndices{};
		VkQueue vkGraphicsQueue = VK_NULL_HANDLE;
		VkQueue vkPresentQueue = VK_NULL_HANDLE;

		// Devices
		VkPhysicalDevice vkPhysicalDevice = VK_NULL_HANDLE;
		VkDevice vkLogicalDevice = VK_NULL_HANDLE;

		// Surfaces
		VkSurfaceKHR vkSurface = VK_NULL_HANDLE;

		// Swap chains
		VkSwapchainKHR vkSwapChain = VK_NULL_HANDLE;
		std::vector<VkImage> vkSwapChainImages{};
		VkFormat vkSwapChainImageFormat{};
		VkExtent2D vkSwapChainExtent{};
		std::vector<VkImageView> vkSwapChainImageViews{};

		// Multisampling
		VulkanImage* multisampledColorImage{};

		// Framebuffers
		std::vector<VkFramebuffer> vkSwapChainFramebuffers{};
		
		// Command pools and buffers
		VkCommandPool vkCommandPool = VK_NULL_HANDLE;
		std::vector<VkCommandBuffer> vkCommandBuffers{};

		// Synchronisation
		std::vector<VkSemaphore> imageAvailableSemaphores{};
		std::vector<VkSemaphore> renderFinishedSemaphores{};
		std::vector<VkFence> inFlightFences{};

		// Pipeline
		VulkanPipeline* graphicsPipelineDefault = nullptr;
		VkRenderPass vkRenderPass = VK_NULL_HANDLE;

		// Depth buffers
		VulkanImage* vkDepthBuffer = nullptr;

		// Memory management 
		VmaAllocator vmaAllocator;

		// External callback for rendering
		std::function<void(VkCommandBuffer, uint32_t)> external_callback;

		// Required extensions to be supported
		const std::vector<const char*> deviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
			VK_EXT_ROBUSTNESS_2_EXTENSION_NAME
		};
		// Required validation layers to be supported
		const std::vector<const char*> vkValidationLayers = {
			"VK_LAYER_KHRONOS_validation",
		};

		// Validation layers
		VkDebugUtilsMessengerEXT vkDebugMessenger = VK_NULL_HANDLE;
#ifndef _DEBUG
		const bool enableVkValidationLayers = false;
#else
		const bool enableVkValidationLayers = true;
#endif

		// Leak check counters
		size_t leakBufferCounter = 0;
		size_t leakUniformBufferCounter = 0;
		size_t leakImageCounter = 0;
		size_t leakTextureImageCounter = 0;
		size_t leakPipelineCounter = 0;

		// Physical device functions
		int checkVulkanPhysicalDeviceScore(VkPhysicalDevice device);
		QueueFamilyIndices findVulkanQueueFamilies(VkPhysicalDevice device);
		bool checkVulkanDeviceExtensionSupport(VkPhysicalDevice device);
		SwapChainSupportDetails queryVulkanSwapChainSupport(VkPhysicalDevice device);
		VkSampleCountFlagBits getMaxUsableSampleCount();

		// VkFormat functions
		VkFormat findVulkanSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
		VkFormat findVulkanSupportedDepthFormat();
		bool doesVulkanFormatHaveStencilComponent(VkFormat format);

		// Swap chain functions
		VkSurfaceFormatKHR chooseVulkanSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR chooseVulkanSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D chooseVulkanSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
		void createVulkanSwapChainViews();
		void recreateVulkanSwapChain();
		void cleanupVulkanSwapChain();

		// Command buffer functions
		void recordVulkanCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

		// Shaders functions
		VkShaderModule createVulkanShaderModule(const std::vector<char>& code);

		// Image view functions
		VkImageView createVulkanImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

		// Init functions
		bool checkVulkanValidationLayers();
		void initVulkanInstance();
		void initVulkanDevice();
		void createVulkanLogicalDevice();
		void createVulkanSurface();
		void createVulkanSwapChain();
		void createVulkanDefaultGraphicsPipeline();
		void createVulkanRenderPass();
		void createVulkanFramebuffers();
		void createVulkanCommandPools();
		void createVulkanCommandBuffers();
		void createVulkanSyncObjects();
		void createVulkanDepthResources();
		void createMultisampledColorResources();
		void createVulkanMemoryAllocator();

	public:
		VulkanRenderingEngine() {}

		// Signaling function for framebuffer resize
		void SignalFramebufferResizeGLFW();
		
		// Cleanup functions
		void cleanup();
		void cleanupVulkanBuffer(VulkanBuffer* buffer);
		void cleanupVulkanTextureImage(VulkanTextureImage* buffer);
		void cleanupVulkanPipeline(VulkanPipeline* pipeline);
		void cleanupVulkanImage(VulkanImage* image);
		
		// Init
		void init(unsigned int xsize, unsigned int ysize, std::string title);

		// Engine functions
		void drawFrame();
		void SetRenderCallback(std::function<void(VkCommandBuffer, uint32_t)> callback);
		
		// Buffer functions
		VulkanBuffer* createVulkanBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VmaAllocationCreateFlags vmaAllocFlags);
		void bufferVulkanTransferCopy(VulkanBuffer* src, VulkanBuffer* dest, VkDeviceSize size);
		VulkanBuffer* createVulkanVertexBufferFromData(std::vector<RenderingVertex> vertices);
		VulkanBuffer* createVulkanStagingBufferWithData(void* data, VkDeviceSize dataSize);

		// Image functions
		VulkanImage* createVulkanImage(uint32_t width, uint32_t height, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
		VulkanTextureImage* createVulkanTextureImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
		void copyVulcanBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
		void appendVulkanImageViewToVulkanTextureImage(VulkanTextureImage* teximage);
		void appendVulkanSamplerToVulkanTextureImage(VulkanTextureImage* teximage);
		void transitionVulkanImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

		// Command buffer functions
		VkCommandBuffer beginVulkanSingleTimeCommandsCommandBuffer();
		void endVulkanSingleTimeCommandsCommandBuffer(VkCommandBuffer commandBuffer);

		// Pipeline functions
		VulkanPipelineSettings getVulkanDefaultPipelineSettings();
		VulkanPipeline* createVulkanPipelineFromPrealloc(VulkanPipeline* pipeline, VulkanPipelineSettings& settings, std::string vert_path, std::string frag_path);
		VulkanPipeline* createVulkanPipeline(VulkanPipelineSettings& settings, std::string vert_path, std::string frag_path);

		// Pipeline descriptor functions
		void createVulkanDescriptorSetLayout(VulkanPipeline* pipeline, VulkanDescriptorLayoutSettings settings);
		void createVulkanUniformBuffers(VulkanPipeline* pipeline);
		void createVulkanDescriptorPool(VulkanPipeline* pipeline, VulkanDescriptorLayoutSettings settings);
		void createVulkanDescriptorSets(VulkanPipeline* pipeline, VulkanDescriptorLayoutSettings settings);

		// Getters
		VkDevice GetLogicalDevice();
		GLFWwindowData const GetWindow();
		const uint32_t GetMaxFramesInFlight();

		// Utility functions
		uint32_t findVulkanMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	};

#pragma endregion

	}

	namespace Scripting
	{
		enum class ExecuteType {
			EventHandler,
			ObjectScript
		};

#pragma region LuaScript.hpp
		class LuaScript
		{
		protected:
			lua_State* state;

		public:
			std::string path;

			LuaScript(std::string path);
			~LuaScript()
			{
				lua_close(this->state);
			}

			lua_State* GetState() { return this->state; }
		};
#pragma endregion

#pragma region LuaScriptExecutor.hpp

		class LuaScriptExecutor : public InternalEngineObject
		{
		protected:
			void registerCallbacks(lua_State* state);
		public:
			LuaScriptExecutor() {};
			~LuaScriptExecutor() {};

			int CallIntoScript(ExecuteType etype, std::shared_ptr<LuaScript> script, std::string function, void* data);

			int LoadAndCompileScript(LuaScript* script);
		};

#pragma endregion
	
	}

	namespace EventHandling
	{

#pragma region EventHandling.hpp

		enum class EventType
		{
			ON_INIT = 0x1,
			ON_UPDATE = 0x2,
			ON_KEYDOWN = 0x4,
			ON_KEYUP = 0x8,
			ON_MOUSEMOVED = 0x10,
			EVENT_UNDEFINED = 0xffff
		};

		class Event final
		{
		private:
		public:
			const EventType event_type;

			double deltaTime = 0.0;
			union
			{
				uint16_t keycode = 0; // ON_KEYDOWN/ON_KEYUP events
				struct {
					double x;
					double y;
				} mouse; // ON_MOUSEMOVED event
			};

			Event(const EventType eventtype) : event_type(eventtype) {};
			~Event() {};
		};

#pragma endregion

	}

#pragma region Engine.hpp

	typedef struct structEngineConfig
	{
		struct
		{
			unsigned int sizeX = 0;
			unsigned int sizeY = 0;
			std::string title = "I am an title!";
		} window;

		struct {
			unsigned int framerateTarget = 0;
		} rendering;

		struct {
			std::string textures;
			std::string models;
			std::string scripts;
			std::string scenes; 
		} lookup;

		std::string defaultScene;
	} EngineConfig;

	class Engine final
	{
	private:
		std::string config_path = "";

		// Window
		unsigned int framerateTarget = 30;

		double lastDeltaTime = 0.0;

		EngineConfig config{};

		// Engine parts
		Rendering::VulkanRenderingEngine* rendering_engine = nullptr;
		AssetManager* asset_manager = nullptr;
		Scripting::LuaScriptExecutor* script_executor = nullptr;
        std::shared_ptr<Logging::Logger> logger{};

		// Event handler storage
		std::multimap<EventHandling::EventType, std::function<int(EventHandling::Event&)>> event_handlers;
		
		static void spinSleep(double seconds);

		static void RenderCallback(VkCommandBuffer commandBuffer, uint32_t currentFrame, Engine* engine);

		static void ErrorCallback(int code, const char* message);
		static void OnWindowFocusCallback(GLFWwindow* window, int focused);
		static void CursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
		static void CursorEnterLeaveCallback(GLFWwindow* window, int entered);
		static void OnKeyEventCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

		void handleInput(const double deltaTime) noexcept;

		void engineLoop();

		void HandleConfig();

	public:
		std::shared_ptr<SceneManager> scene_manager{};
		
		Engine(std::shared_ptr<Logging::Logger> logger) noexcept;
		~Engine() noexcept;

		void SetFramerateTarget(unsigned framerate) noexcept;

		void Init();
		void Run();

		void ConfigFile(std::string path);
		void RegisterEventHandler(EventHandling::EventType event_type, std::function<int(EventHandling::Event&)> function);
		
		void Stop();

		int FireEvent(EventHandling::Event& event);

		double GetLastDeltaTime();

		inline AssetManager* GetAssetManager() noexcept { return this->asset_manager; }
		inline Rendering::VulkanRenderingEngine* GetRenderingEngine() noexcept { return this->rendering_engine; }
		inline std::weak_ptr<SceneManager> GetSceneManager() noexcept { return std::weak_ptr<SceneManager>(this->scene_manager); }
	};

#pragma endregion

#pragma region Object.hpp

	class Object : public InternalEngineObject
	{
	protected:
		/// <summary>
		/// Position of object in worldspace.
		/// </summary>
		glm::vec3 _pos = glm::vec3();

		/// <summary>
		/// Size of object.
		/// </summary>
		glm::vec3 _size = glm::vec3();

		/// <summary>
		/// Rotation of object.
		/// </summary>
		glm::vec3 _rotation = glm::vec3();

		/// <summary>
		/// Parent pos size and rot
		/// </summary>
		glm::vec3 _parent_pos = glm::vec3();
		glm::vec3 _parent_size = glm::vec3();
		glm::vec3 _parent_rotation = glm::vec3();

		Object* parent = nullptr;

		std::vector<Object*> children;

		unsigned int screenx = 0, screeny = 0;

		std::function<void(Object*)> _onClick = nullptr;

	public:
		Rendering::IRenderer* renderer = nullptr;
		
		Object() noexcept;
		~Object() noexcept;

		void ScreenSizeInform(unsigned int screenx, unsigned int screeny) noexcept;

		virtual void Translate(const glm::vec3 pos) noexcept;

		virtual void Scale(const glm::vec3 size) noexcept;

		virtual void Rotate(const glm::vec3 rotation) noexcept;

		virtual void UpdateRenderer() noexcept;

		virtual int Render(VkCommandBuffer commandBuffer, uint32_t currentFrame);

		glm::vec3 position() const noexcept;
		glm::vec3 size() const noexcept;
		glm::vec3 rotation() const noexcept;

		void SetParentPositionRotationSize(glm::vec3 position, glm::vec3 rotation, glm::vec3 size);

		void AddChild(Object* object);
		void RemoveChildren();
		void SetParent(Object* object);
	};

#pragma endregion

#pragma region AssetManager.hpp

	class AssetManager final
	{
	private:
		std::unordered_map<std::string, std::shared_ptr<Scripting::LuaScript>> luascripts;
		std::unordered_map<std::string, std::shared_ptr<Rendering::Texture>> textures;
		std::unordered_map<std::string, std::shared_ptr<Rendering::Font>> fonts;
		std::unordered_map<std::string, std::shared_ptr<Rendering::Mesh>> models;
	public:
		std::shared_ptr<Logging::Logger> logger;
		Scripting::LuaScriptExecutor* lua_executor;

		AssetManager() {};
		~AssetManager();

		void AddTexture(std::string name, std::string path, Rendering::Texture_InitFiletype filetype);
		void AddFont(std::string name, std::string path);
		void AddLuaScript(std::string name, std::string path);
		void AddModel(std::string name, std::string path);

		std::shared_ptr<Rendering::Texture> GetTexture(std::string name);
		std::shared_ptr<Rendering::Font> GetFont(std::string name);
		std::shared_ptr<Scripting::LuaScript> GetLuaScript(std::string name);
		std::shared_ptr<Rendering::Mesh> GetModel(std::string name);
	};

#pragma endregion

#pragma region Scene.hpp

	class Scene : public InternalEngineObject
    {
    private:
    protected:
        std::unordered_map<std::string, Object*> objects{};
    public:
		Engine* owner_engine;

        Scene();

		const std::unordered_map<std::string, Object*>* const GetAllObjects() noexcept;

		void AddObject(std::string name, Object* ptr);
		Object* FindObject(std::string name);
		size_t RemoveObject(std::string name) noexcept;
    };

#pragma endregion

#pragma region GlobalSceneManager.hpp

	class SceneManager final
	{
	private:
		std::unordered_map<std::string, Object*> objects{};
		std::unique_ptr<Scene> current_scene;

		glm::vec3 cameraTransform{}; // XYZ position
		glm::vec2 cameraHVRotation{}; // Horizontal and Vertical rotation

		glm::vec3 lightPosition{};

		void CameraUpdated();
	public:
		std::shared_ptr<Logging::Logger> logger;
		Engine* owner_engine;

		SceneManager(std::shared_ptr<Logging::Logger> logger);
		~SceneManager();

		void UpdateHeldLogger(std::shared_ptr<Logging::Logger> new_logger);

		const std::unordered_map<std::string, Object*>* const GetAllObjects() noexcept;

		void AddObject(std::string name, Object* ptr);
		Object* FindObject(std::string name);
		size_t RemoveObject(std::string name) noexcept;

		glm::vec3 GetLightTransform();
		void SetLightTransform(glm::vec3 newpos);

		glm::vec3 GetCameraTransform();
		glm::vec2 GetCameraHVRotation();
		glm::mat4 GetCameraViewMatrix();

		void SetCameraTransform(glm::vec3 transform);
		void SetCameraHVRotation(glm::vec2 hvrotation);
	};

#pragma endregion

	namespace ObjectFactory
	{
#pragma region ObjectFactory.hpp

		Object* CreateSpriteObject(double x, double y, double sizex, double sizey, std::shared_ptr<::Engine::Rendering::Texture> sprite);
		Object* CreateTextObject(double x, double y, int size, std::string text, ::Engine::Rendering::Font* font);
		Object* CreateGeneric3DObject(double x, double y, double z, double sizex, double sizey, double sizez, double rotx, double roty, double rotz, std::shared_ptr<::Engine::Rendering::Mesh> mesh);

#pragma endregion
	}
}
