#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define STB_IMAGE_IMPLEMENTATION
#define GLM_ENABLE_EXPERIMENTAL
#include "Vertex.h"
#include "MeshObject.h"
#include "ModelLoader.h"
#include "Render.h"
#include "SamplerBuilder.h"
#include "VulkanUtils.h"
#include "Camera.h"
#include "LightSource.h"
#include <stb_image.h>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <map>
#include <optional>
#include <set>
#include <cstdint> // Necessary for uint32_t
#include <limits> // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp
#include <fstream>
#include <unordered_map>
//定义同时使用的最大帧数
const int MAX_FRAMES_IN_FLIGHT = 2;

//统一缓冲对象UBO
struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj[2];
};

struct UniformBufferMaterial {
	glm::vec4 ambient;   // Ka: 环境光反射系数
	glm::vec4 diffuse;   // Kd: 漫反射系数
	glm::vec4 specular;  // Ks: 镜面反射系数
	float shininess;          // Ns: 镜面高光指数
	float opacity;
};
struct UniformBufferLight {
	glm::vec4 lightPosition; // 光源位置
	glm::vec4 lightColor;    // 光源颜色
	float lightIntensity;    // 光源强度
};

class HelloTriangleApplication {
public:
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	};
private:
	
	struct MeshDrawInfo {
		uint32_t indexOffset;
		uint32_t indexCount;
		uint32_t materialIndex;
	};
	struct VulkanMesh {
		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;
		VkBuffer indexBuffer;
		VkDeviceMemory indexBufferMemory;
		std::vector<MeshDrawInfo> materialDrawInfos;
	};
	struct VulkanMaterial {
		VkImage diffuseTexture;
		VkDeviceMemory diffuseTextureMemory;
		VkImageView diffuseImageView;
		VkSampler textureSampler;
		VkDescriptorSet descriptorSet;
	};
	struct PerObjectUniformBuffer {
		std::vector<VkBuffer> buffers;
		std::vector<VkDeviceMemory> memories;
		std::vector<void*> mapped;
	};


	Camera camera{ glm::vec3(0.0f, 1.0f, 3.5f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), 37.0f, WIDTH / (float)HEIGHT, 0.1f, 10.0f };


	std::vector<MeshObject> meshObjects; // 存储逻辑层的网格体
	std::vector<VulkanMesh> vulkanMeshes; // 存储 Vulkan 相关的网格体缓冲
	std::vector<PerObjectUniformBuffer> perObjectUniformBuffers; // 每个物体的统一缓冲区
	std::vector<std::vector<VulkanMaterial>> vulkanMaterials; // 存储 Vulkan 相关的材质
	// 定义一个网格结构体，代表模型的一部分
	
	LightSource light{ LightSource::LightType::Point, glm::vec3(0.0f, 1.5f, -3.0f), glm::vec3(1.0f, 1.0f, 1.0f), 5.0f };


	Render<Vertex>* renderer;
	ColorBlendRender<Vertex>* colorBlendRenderer;

	const uint32_t WIDTH = 800;
	const uint32_t HEIGHT = 600;
	
	GLFWwindow* window;
	VkInstance instance;
	//物理设备,即显卡,该设备在销毁实例时会自动销毁
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	size_t minimumUboAlignment = 0;
	//调试回调函数
	VkDebugUtilsMessengerEXT debugMessenger;

	//当前使用的帧索引
	uint32_t currentFrame = 0;

	//深度缓冲
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;


	//解析附件
	VkImageView resolveImageView[4];
	VkImage resolveImage[2];
	VkDeviceMemory resolveImageMemory[2];
	
	//着色器描述符，每个不同的材质都需要一个描述符
	VkDescriptorPool descriptorPool;
	std::vector<std::vector<VkDescriptorSet>> descriptorSets;

	//显示器抽象
	VkSurfaceKHR surface;

	//命令
	VkCommandPool commandPool;
	//将在命令池被销毁时自动销毁
	std::vector<VkCommandBuffer> commandBuffers;

	//验证层
	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	//设备扩展
	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_MULTIVIEW_EXTENSION_NAME,
		VK_NV_VIEWPORT_ARRAY2_EXTENSION_NAME
	};

	//队列族索引
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	//设备扩展
	VkDevice device;

	//队列族索引
	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;
		bool isComplete() {
			//通过has_value()函数检查是否有值
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};
	//交换链
	VkSwapchainKHR swapChain;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImage> swapChainImages;

	//交换链支持信息
	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;

	std::vector<VkFramebuffer> BlendFramebuffers;
	//同步对象
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	//标记
	bool framebufferResized = false;

	//图形管线
	VkDescriptorSetLayout descriptorSetLayout;
	//混合管线
	VkDescriptorSetLayout descriptorSetLayoutBlend;
	std::vector<VkDescriptorSet>   descriptorSetBlend;
	//纹理
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;
	//开启验证层
#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif
	
	//主循环
	void mainLoop() {
		//循环检查窗口是否被关闭
		while (!glfwWindowShouldClose(window)) {
			//等待事件
			glfwPollEvents();
			//绘制帧
			drawFrame();
		}
		//等待逻辑设备完成工作，否则会导致程序崩溃
		vkDeviceWaitIdle(device);
	}

	void drawFrame() {
		vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
			imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		vkResetFences(device, 1, &inFlightFences[currentFrame]);

		// 创建统一的命令缓冲
		vkResetCommandBuffer(commandBuffers[currentFrame], 0);
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo);

		// 第一次渲染：绘制左右眼
		updateUniformBuffer(currentFrame);
		recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

		// 添加图像布局转换屏障（在同一个命令缓冲中）
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = resolveImage[1];
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(
			commandBuffers[currentFrame],
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		// 第二次渲染：混合
		recordCommandBufferBlend(commandBuffers[currentFrame], imageIndex);
		VkImageMemoryBarrier barrier1{};
		barrier1.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER; // 必须明确指定结构体类型
		barrier1.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier1.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barrier1.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier1.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier1.image = resolveImage[1];
		barrier1.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier1.subresourceRange.levelCount = 1;
		barrier1.subresourceRange.layerCount = 1;
		barrier1.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;          // 片段着色器读取完成
		barrier1.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // 颜色附件可以写入

		vkCmdPipelineBarrier(
			commandBuffers[currentFrame],
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier1
		);


		vkEndCommandBuffer(commandBuffers[currentFrame]);

		// 统一提交
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

		VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		// 呈现
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChain;
		presentInfo.pImageIndices = &imageIndex;

		result = vkQueuePresentKHR(presentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
			framebufferResized = false;
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}


	//在某些情况下，您可能需要重新创建交换链，例如，当窗口大小发生变化时。
	void recreateSwapChain() {
		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		//如果窗口的宽度或高度为0，则等待窗口大小发生变化
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(device);
		cleanupSwapChain();
		createSwapChain();
		createImageViews();
		createDepthResources();
		createResolveResources();
		creatDescriptorSetsBlend();
		createFramebuffers();
	}

	void cleanupSwapChain() {
		for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
			vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
		}
		for (size_t i = 0; i < BlendFramebuffers.size(); i++) {
			vkDestroyFramebuffer(device, BlendFramebuffers[i], nullptr);
		}
		for (size_t i = 0; i < swapChainImageViews.size(); i++) {
			vkDestroyImageView(device, swapChainImageViews[i], nullptr);
		}
		// 销毁深度图像视图和内存
		if (depthImageView != VK_NULL_HANDLE) {
			vkDestroyImageView(device, depthImageView, nullptr);
			depthImageView = VK_NULL_HANDLE;
		}
		if (depthImage != VK_NULL_HANDLE) {
			vkDestroyImage(device, depthImage, nullptr);
			depthImage = VK_NULL_HANDLE;
		}
		if (depthImageMemory != VK_NULL_HANDLE) {
			vkFreeMemory(device, depthImageMemory, nullptr);
			depthImageMemory = VK_NULL_HANDLE;
		}

		// 销毁解析图像视图和内存
		for (int i = 0; i < 2; i++) {
			if (resolveImage[i] != VK_NULL_HANDLE) {
				vkDestroyImage(device, resolveImage[i], nullptr);
				resolveImage[i] = VK_NULL_HANDLE;
			}
			if (resolveImageMemory[i] != VK_NULL_HANDLE) {
				vkFreeMemory(device, resolveImageMemory[i], nullptr);
				resolveImageMemory[i] = VK_NULL_HANDLE;
			}
		}

		for (int i = 0; i < 4; i++) {
			if (resolveImageView[i] != VK_NULL_HANDLE) {
				vkDestroyImageView(device, resolveImageView[i], nullptr);
				resolveImageView[i] = VK_NULL_HANDLE;
			}
		}

		vkDestroySwapchainKHR(device, swapChain, nullptr);
	}

	//创建同步对象
	void createSyncObjects() {
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;


		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

				throw std::runtime_error("failed to create synchronization objects for a frame!");
			}
		}
	}

	//窗口初始化
	void initWindow() {
		//初始化glfw
		glfwInit();
		//不创建opengl上下文
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		
		//创建窗口,前三个参数分别指定窗口的宽度、高度和标题。 第四个参数允许您选择性地指定要打开窗口的显示器，最后一个参数仅与 OpenGL 相关。
		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

	}

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
		app->framebufferResized = true;
	}

	//初始化
	void initVulkan() {
		createInstance();
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		//注意这个要在创建深度缓冲之前创建，因为深度缓冲需要使用命令池
		createCommandPool();  
		createCommandBuffers();
		createSwapChain();
		createImageViews();
		createTextureSampler();
		createDepthResources();
		createResolveResources();
		

		

		loadModels();
		setupHierarchy();
		createVulkanBuffersForMeshes();
		createVulkanMaterials();
		createPerObjectUniformBuffers();
		createDescriptorSetLayout();
		createDescriptorPool();
		createDescriptorSets();
		createDescriptorSetLayoutBlend();
		creatDescriptorSetsBlend();
		createRender();
		createFramebuffers();

		initCamera();


		createSyncObjects();
	}

	void createRender() {
		renderer = new Render<Vertex>(device, physicalDevice, swapChainExtent, swapChainImageFormat,
			"shaders/vert.spv", "shaders/frag.spv", descriptorSetLayout);
		renderer->initialize();

		colorBlendRenderer = new ColorBlendRender<Vertex>(device, physicalDevice, swapChainExtent, swapChainImageFormat,
			"shaders/vert.spv", "shaders/color_blend.spv",
			descriptorSetLayoutBlend);
		colorBlendRenderer->initialize();
	}

	void createVulkanMaterials() {
		vulkanMaterials.resize(meshObjects.size());

		for (size_t i = 0; i < meshObjects.size(); i++) {
			const auto& meshObject = meshObjects[i];
			const auto& materials = meshObject.getMaterials();

			vulkanMaterials[i].resize(materials.size());
			for (size_t j = 0; j < materials.size(); j++) {
				const auto& material = materials[j];
				VulkanMaterial vulkanMaterial;

				// 加载纹理
				if (!material.texturePath.empty()) {
					createTextureImage(material.texturePath, vulkanMaterial.diffuseTexture,
						vulkanMaterial.diffuseTextureMemory, vulkanMaterial.diffuseImageView);
				}
				else {
					// 使用默认纹理
					vulkanMaterial.diffuseTexture = textureImage;
					vulkanMaterial.diffuseTextureMemory = textureImageMemory;
					vulkanMaterial.diffuseImageView = textureImageView;
				}

				// 创建采样器
				createSamplerForMaterial(vulkanMaterial);

				vulkanMaterials[i][j] = std::move(vulkanMaterial);
			}
		}
	}

	void createSamplerForMaterial(VulkanMaterial& material) {
		SamplerBuilder samplerBuilder(physicalDevice);
		VkPhysicalDeviceFeatures features{};
		vkGetPhysicalDeviceFeatures(physicalDevice, &features);

		if (features.samplerAnisotropy) {
			material.textureSampler = samplerBuilder
				.enableAnisotropy()
				.setBilinearFiltering()
				.setAddressMode(VK_SAMPLER_ADDRESS_MODE_REPEAT)
				.build(device);
		}
		else {
			material.textureSampler = samplerBuilder
				.setBilinearFiltering()
				.setAddressMode(VK_SAMPLER_ADDRESS_MODE_REPEAT)
				.build(device);
		}
	}
	// 加载多个模型
	void loadModels() {
		std::vector<std::pair<std::string, std::string>> modelPaths = {
			{"asserts/hutao/hutao.obj", "asserts/hutao"},
			{"asserts/grass/grass.obj","asserts/grass"}
		};

		for (size_t i = 0; i < modelPaths.size(); ++i) {
			const auto& [modelPath, materialPath] = modelPaths[i];
			ModelLoader modelLoader;
			if (!modelLoader.loadModel(modelPath, materialPath)) {
				throw std::runtime_error("Failed to load model: " + modelPath);
			}

			// 创建 MeshObject
			MeshObject meshObject(modelLoader.getMeshes(), modelLoader.getMaterials());
			meshObject.resetTransform();
			if (i == 0) {
				// 第一个模型（例如：胡桃）

				meshObject.translate(glm::vec3(0.0f, 0.0f, 0.0f)); // 放置在原点
			}
			else if (i == 1) {
				// 第二个模型（草地）
				glm::vec3 minBounds, maxBounds;
				meshObjects[0].getBoundingBox(minBounds, maxBounds); // 获取第一个模型的包围盒

				// 计算第一个模型的宽度和深度
				glm::vec3 size = maxBounds - minBounds;
				float width = size.x;
				float height = size.y;
				float depth = size.z;

				
				meshObject.rotate(90.0f, glm::vec3(-1.0f, 0.0f, 0.0f)); 
				
				// 平移草地模型，使其位于第一个模型的底部
				float groundLevel = maxBounds.y; // 第一个模型的最低点（Y 轴,因为这个）
				meshObject.translate(glm::vec3(0.0f, -groundLevel, 0.0f)); // 将 Y 轴值映射到 Z 轴
			}

			meshObjects.push_back(std::move(meshObject));
		}
	}


	void initCamera() {
		// 获取第一个物体的包围盒中心
		glm::vec3 minBounds, maxBounds;
		meshObjects[0].getBoundingBox(minBounds, maxBounds);
		glm::vec3 modelCenter = (minBounds + maxBounds) * 0.5f;

		// 调整相机的位置和目标点
		glm::vec3 cameraPosition = modelCenter + glm::vec3(0.0f, 0.0f, -3.7f); // 距离目标物体 3.5 个单位
		camera.reset(cameraPosition, modelCenter, glm::vec3(0.0f, 1.0f, 0.0f)); // 重置相机

	}


	void createVulkanBuffersForMeshes() {
		vulkanMeshes.resize(meshObjects.size());

		for (size_t i = 0; i < meshObjects.size(); i++) {
			const auto& meshObject = meshObjects[i];
			const auto& meshes = meshObject.getMeshes();

			std::vector<Vertex> allVertices;
			std::vector<uint32_t> allIndices;

			// 记录每个 Mesh 的绘制信息
			
			std::vector<MeshDrawInfo> meshDrawInfos;

			for (const auto& mesh : meshes) {
				MeshDrawInfo drawInfo{};
				drawInfo.indexOffset = static_cast<uint32_t>(allIndices.size());
				drawInfo.indexCount = static_cast<uint32_t>(mesh.indices.size());
				drawInfo.materialIndex = mesh.materialIndex;

				// 合并顶点数据
				uint32_t vertexOffset = static_cast<uint32_t>(allVertices.size());
				allVertices.insert(allVertices.end(), mesh.vertices.begin(), mesh.vertices.end());

				// 合并索引数据，并调整索引偏移
				for (uint32_t index : mesh.indices) {
					allIndices.push_back(index + vertexOffset);
				}

				meshDrawInfos.push_back(drawInfo);
			}

			// 创建统一的顶点缓冲区
			VkDeviceSize vertexBufferSize = sizeof(Vertex) * allVertices.size();
			createBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vulkanMeshes[i].vertexBuffer, vulkanMeshes[i].vertexBufferMemory);
			uploadDataToBuffer(allVertices.data(), vertexBufferSize, vulkanMeshes[i].vertexBuffer);

			// 创建统一的索引缓冲区
			VkDeviceSize indexBufferSize = sizeof(uint32_t) * allIndices.size();
			createBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vulkanMeshes[i].indexBuffer, vulkanMeshes[i].indexBufferMemory);
			uploadDataToBuffer(allIndices.data(), indexBufferSize, vulkanMeshes[i].indexBuffer);

			// 保存绘制信息
			vulkanMeshes[i].materialDrawInfos = std::move(meshDrawInfos);
		}
	}


	// 创建每个物体的统一缓冲区
	void createPerObjectUniformBuffers() {
		//这些是每个物体的统一缓冲区
		VkDeviceSize bufferSize =
			VulkanUtils::alignOffset(sizeof(UniformBufferObject), minimumUboAlignment) +
			VulkanUtils::alignOffset(sizeof(UniformBufferLight), minimumUboAlignment)+ 
			VulkanUtils::alignOffset(sizeof(glm::vec3), minimumUboAlignment);

		perObjectUniformBuffers.resize(meshObjects.size());
		for (size_t i = 0; i < meshObjects.size(); i++) {
			int materialCount = meshObjects[i].getMaterials().size();
			perObjectUniformBuffers[i].buffers.resize(MAX_FRAMES_IN_FLIGHT + materialCount);
			perObjectUniformBuffers[i].memories.resize(MAX_FRAMES_IN_FLIGHT + materialCount);
			perObjectUniformBuffers[i].mapped.resize(MAX_FRAMES_IN_FLIGHT + materialCount);

			//创建统一缓冲区
			for (size_t j = 0; j < MAX_FRAMES_IN_FLIGHT; j++) {
				createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					perObjectUniformBuffers[i].buffers[j],
					perObjectUniformBuffers[i].memories[j]);

				vkMapMemory(device, perObjectUniformBuffers[i].memories[j], 0, bufferSize, 0,
					&perObjectUniformBuffers[i].mapped[j]);
			}

			//创建材质统一缓冲区
			// 这里的 materialCount 是材质数量
			VkDeviceSize size = sizeof(UniformBufferMaterial);
			for (int j = MAX_FRAMES_IN_FLIGHT; j < materialCount + MAX_FRAMES_IN_FLIGHT; j++) {
				createBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					perObjectUniformBuffers[i].buffers[j],
					perObjectUniformBuffers[i].memories[j]);
				vkMapMemory(device, perObjectUniformBuffers[i].memories[j], 0, size, 0,
					&perObjectUniformBuffers[i].mapped[j]);
			}
		}
		//材质统一缓冲区只要更新一次就行
		updateMaterialUniformBuffer();
	}

	void updateMaterialUniformBuffer() {
		for (size_t i = 0; i < meshObjects.size(); i++) {
			const auto materials = meshObjects[i].getMaterials();
			
			for (size_t j = 0; j < materials.size(); j++) {
				//更新材质
				UniformBufferMaterial uboMaterial{};
				uboMaterial.ambient =glm::vec4(materials[j].ambientColor,1.0);
				uboMaterial.diffuse = glm::vec4(materials[j].diffuseColor,1.0);
				uboMaterial.specular = glm::vec4(materials[j].specularColor,1.0);
				uboMaterial.shininess = materials[j].shininess;
				uboMaterial.opacity = materials[j].opacity;
				//std::cout << "material: " << materials[j].diffuseColor.x << " " << materials[j].diffuseColor.y << " " << materials[j].diffuseColor.z << std::endl;
				char* data = static_cast<char*>(perObjectUniformBuffers[i].mapped[MAX_FRAMES_IN_FLIGHT + j]);
				memcpy(data, &uboMaterial, sizeof(uboMaterial));
			}
		}
	}

	void setupHierarchy() {
		if (meshObjects.size() > 1) {
			for (size_t i = 1; i < meshObjects.size(); ++i) {
				meshObjects[i].setParentIndex(0); // 将所有物体的父物体设置为第一个物体
			}
		}
	}

	// 更新统一缓冲区
	void updateUniformBuffer(uint32_t currentImage) {
		static auto lastTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
		lastTime = currentTime;
		camera.setAspectRatio(static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height));

		// 每秒旋转 36 度
		float rotationSpeed = 36.0f; // 每秒旋转的角度
		float rotationAngle = rotationSpeed * deltaTime;

		// 旋转根物体
		if (!meshObjects.empty()) {
			meshObjects[0].rotate(rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
		}

		//camera.rotate(rotationAngle, glm::vec3(0.0f, 1.0f,0.0f));
		glm::mat4 ViewMatrix = camera.getViewMatrix();
		
		glm::mat4 projMatrix = camera.getProjectionMatrix();
		projMatrix[1][1] = -projMatrix[1][1]; // 反转 Y 轴
		for (size_t i = 0; i < meshObjects.size(); i++) {
			char* data = static_cast<char*>(perObjectUniformBuffers[i].mapped[0]);

			UniformBufferObject ubo{};
			//应用层级建模
			ubo.model = meshObjects[i].getModelMatrix();
			if (meshObjects[i].getParentIndex() != -1) {
				ubo.model = meshObjects[meshObjects[i].getParentIndex()].getModelMatrix() * ubo.model;
			}
			ubo.view = ViewMatrix;
			// 计算物体的深度
			glm::vec3 objectPosition = glm::vec3(ubo.model[3]); // 提取物体的世界坐标
			float objectDepth = glm::length(objectPosition - camera.getPosition()); // 计算物体到相机的距离


			ubo.proj[0] = projMatrix;
			ubo.proj[1] = camera.getProjectionMatrixWithDepthOffset(objectDepth); 
			ubo.proj[1][1][1] = -ubo.proj[1][1][1]; // 反转 Y 轴
			memcpy(data, &ubo, sizeof(ubo));
			data +=VulkanUtils::alignOffset(sizeof(ubo), minimumUboAlignment);

			//更新观察点
			
			glm::vec3 cameraPos = camera.getPosition();
			memcpy(data, &cameraPos, sizeof(cameraPos));
			data +=VulkanUtils::alignOffset(sizeof(cameraPos), minimumUboAlignment);

			UniformBufferLight l{};
			l.lightPosition = glm::vec4(light.getPosition(),1.0);
			l.lightColor = glm::vec4(light.getColor(),1.0);

			l.lightIntensity = light.getIntensity();

			// 更新光源信息
			memcpy(data, &l, sizeof(UniformBufferLight));
		}
	}

	void createDepthResources() {
		VkFormat depthFormat = findDepthFormat();

		VulkanUtils:: createImage(
			device,
			physicalDevice,
			swapChainExtent.width, 
			swapChainExtent.height, 
			depthFormat, 
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			renderer->getMaxUsableSampleCount(physicalDevice),
			depthImage, depthImageMemory,
			2,
			VK_IMAGE_TYPE_2D
		);
		depthImageView =VulkanUtils:: createImageView(device,depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT,VK_IMAGE_VIEW_TYPE_2D_ARRAY,2);
		std::cout << "创建深度附近视图: " <<  std::endl;
		VulkanUtils:: transitionImageLayout(
			device,
			commandPool,
			graphicsQueue,
			depthImage, 
			depthFormat,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		//std::cout << "DepthImageView create: " << (int*)depthImageView << std::endl;

	}

	void createResolveResources() {
		// 解析附件的格式与交换链图像格式相同
		VkFormat resolveFormat = swapChainImageFormat;
		for (int i = 0; i < 2; i++) {
			
		
			// 创建解析附件的图像
			VulkanUtils:: createImage(
				device,
				physicalDevice,
				swapChainExtent.width,
				swapChainExtent.height,
				resolveFormat,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				i==0?renderer->getMaxUsableSampleCount(physicalDevice):VK_SAMPLE_COUNT_1_BIT,
				resolveImage[i],
				resolveImageMemory[i],
				2,
				VK_IMAGE_TYPE_2D
			);

			// 创建解析附件的图像视图
			resolveImageView[i] = VulkanUtils::createImageView(device, resolveImage[i], resolveFormat, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D_ARRAY, 2);
			std::cout << "创建颜色附件视图: " << (int*)resolveImageView[i] << std::endl;

			if (i == 1) {
				// 为每一层创建单独的 ImageView
				resolveImageView[i * 2] = VulkanUtils::createImageView(
					device,
					resolveImage[i],
					resolveFormat,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_VIEW_TYPE_2D, // 单层视图
					1,                     // 第一层
					0                      // 起始层索引
				);
				std::cout << "创建颜色附件视图: " << (int*)resolveImageView[i*2] << std::endl;
				resolveImageView[i * 2 + 1] = VulkanUtils::createImageView(
					device,
					resolveImage[i],
					resolveFormat,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_VIEW_TYPE_2D, // 单层视图
					1,                     // 第二层
					1                      // 起始层索引
				);
				std::cout << "创建颜色附件视图: " << (int*)resolveImageView[i * 2 + 1] << std::endl; 
			}
			// 转换解析附件的布局
			VulkanUtils:: transitionImageLayout(
				device,
				commandPool,
				graphicsQueue,
				resolveImage[i],
				resolveFormat,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			);
		}
	}


	VkFormat findDepthFormat() {
		return VulkanUtils:: findSupportedFormat(physicalDevice,
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	//创建采样器
	void createTextureSampler() {
		SamplerBuilder samplerBuilder(physicalDevice);

		// 设置默认采样方式
		textureSampler = samplerBuilder
			.setBilinearFiltering()
			.setAddressMode(VK_SAMPLER_ADDRESS_MODE_REPEAT)
			.build(device);
	}

	void createTextureImageView() {
		textureImageView =VulkanUtils:: createImageView(device,textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	//创建纹理图像
	void createTextureImage(const std::string& texturePath, VkImage& textureImage, VkDeviceMemory& textureImageMemory, VkImageView& textureImageView) {
		int texWidth, texHeight, texChannels;
		//stbi_set_flip_vertically_on_load(true);
		stbi_uc* pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		if (!pixels) {
			throw std::runtime_error("failed to load texture image!");
		}

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(device, stagingBufferMemory);
		stbi_image_free(pixels);

		VulkanUtils:: createImage(
			device,
			physicalDevice,
			texWidth, texHeight, 
			VK_FORMAT_R8G8B8A8_SRGB, 
			VK_IMAGE_TILING_OPTIMAL, 
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			VK_SAMPLE_COUNT_1_BIT,
			textureImage, textureImageMemory);

		VulkanUtils:: transitionImageLayout(
			device,
			commandPool,
			graphicsQueue,
			textureImage, 
			VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
		VulkanUtils:: transitionImageLayout(
			device,
			commandPool,
			graphicsQueue,
			textureImage, 
			VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);

		//stbi_set_flip_vertically_on_load(false);
		textureImageView =VulkanUtils:: createImageView(device,textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
		
	}

	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
		VkCommandBuffer commandBuffer =VulkanUtils:: beginSingleTimeCommands(device,commandPool);
		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = {
			width,
			height,
			1
		};
		vkCmdCopyBufferToImage(
			commandBuffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
		);



		VulkanUtils::endSingleTimeCommands(device, commandPool, graphicsQueue, commandBuffer);
	}


	void creatDescriptorSetsBlend() {
		descriptorSetBlend.resize(meshObjects.size());
		for (int i = 0; i < meshObjects.size(); i++) {
			descriptorSetBlend[i] = VK_NULL_HANDLE;
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = descriptorPool;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = &descriptorSetLayoutBlend;
			VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSetBlend[i]);
			if (result != VK_SUCCESS) {
				std::cerr << "Failed to allocate descriptor set for material " << std::endl;
			}
			VkDeviceSize offset = 0;

			VkDescriptorBufferInfo bufferInfo{};
			//全部的都一样，直接用了
			bufferInfo.buffer = perObjectUniformBuffers[i].buffers[0];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = resolveImageView[2];
			imageInfo.sampler = textureSampler;
			std::cout << "resolveImageView[2]: " << (int*)resolveImageView[2] << std::endl;
			VkDescriptorImageInfo imageInfo1{};
			imageInfo1.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo1.imageView = resolveImageView[3];
			imageInfo1.sampler = textureSampler;
			std::cout << "resolveImageView[3]: " << (int*)resolveImageView[3] << std::endl;
			std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = descriptorSetBlend[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = descriptorSetBlend[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imageInfo;

			descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[2].dstSet = descriptorSetBlend[i];
			descriptorWrites[2].dstBinding = 2;
			descriptorWrites[2].dstArrayElement = 0;
			descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[2].descriptorCount = 1;
			descriptorWrites[2].pImageInfo = &imageInfo1;

			vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
				descriptorWrites.data(), 0, nullptr);
		}
	}

	void createDescriptorSets() {
		descriptorSets.resize(meshObjects.size());
		for (size_t i = 0; i < meshObjects.size(); i++) {
			const auto& meshObject = meshObjects[i];
			const auto& materials = meshObject.getMaterials();

			descriptorSets[i].resize(materials.size(), VK_NULL_HANDLE); // 初始化为 VK_NULL_HANDLE
			// 为每个材质创建描述符集
			for (size_t j = 0; j < materials.size(); j++) {
				// 获取材质信息
				auto& vulkanMaterial = vulkanMaterials[i][j];

				VkDescriptorSetAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				allocInfo.descriptorPool = descriptorPool;
				allocInfo.descriptorSetCount = 1;
				allocInfo.pSetLayouts = &descriptorSetLayout;

				VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets[i][j]);
				if (result != VK_SUCCESS) {
					std::cerr << "Failed to allocate descriptor set for material " << j << " of mesh " << i << std::endl;
					descriptorSets[i][j] = VK_NULL_HANDLE; // 确保无效的句柄被标记
					continue;
				}
				VkDeviceSize offset = 0;

				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = perObjectUniformBuffers[i].buffers[0];
				bufferInfo.offset = 0;
				bufferInfo.range = sizeof(UniformBufferObject);

				offset +=VulkanUtils::alignOffset(sizeof(UniformBufferObject), minimumUboAlignment);
				
				// 观察点位置
				VkDescriptorBufferInfo eyeBufferInfo{};
				eyeBufferInfo.buffer = perObjectUniformBuffers[i].buffers[0];
				eyeBufferInfo.offset = offset;
				eyeBufferInfo.range = sizeof(glm::vec3);

				offset +=VulkanUtils::alignOffset(sizeof(glm::vec3), minimumUboAlignment);
				// 材质属性
				VkDescriptorBufferInfo materialBufferInfo{};
				//材质缓冲区是每个材质都不一样的，所以不能用第一个。而是用当前材质的
				materialBufferInfo.buffer = perObjectUniformBuffers[i].buffers[j + MAX_FRAMES_IN_FLIGHT];
				materialBufferInfo.offset = 0;
				materialBufferInfo.range = sizeof(UniformBufferMaterial);

				// 光源属性
				VkDescriptorBufferInfo lightBufferInfo{};
				lightBufferInfo.buffer = perObjectUniformBuffers[i].buffers[0];
				lightBufferInfo.offset = offset;
				lightBufferInfo.range = sizeof(UniformBufferLight);


				VkDescriptorImageInfo imageInfo{};
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.imageView = vulkanMaterial.diffuseImageView;
				imageInfo.sampler = vulkanMaterial.textureSampler;

				std::array<VkWriteDescriptorSet, 5> descriptorWrites{};

				descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[0].dstSet = descriptorSets[i][j];
				descriptorWrites[0].dstBinding = 0;
				descriptorWrites[0].dstArrayElement = 0;
				descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[0].descriptorCount = 1;
				descriptorWrites[0].pBufferInfo = &bufferInfo;

				descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[1].dstSet = descriptorSets[i][j];
				descriptorWrites[1].dstBinding = 1;
				descriptorWrites[1].dstArrayElement = 0;
				descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorWrites[1].descriptorCount = 1;
				descriptorWrites[1].pImageInfo = &imageInfo;

				descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[2].dstSet = descriptorSets[i][j];
				descriptorWrites[2].dstBinding = 2;
				descriptorWrites[2].dstArrayElement = 0;
				descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[2].descriptorCount = 1;
				descriptorWrites[2].pBufferInfo = &eyeBufferInfo;

				descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[3].dstSet = descriptorSets[i][j];
				descriptorWrites[3].dstBinding = 3;
				descriptorWrites[3].dstArrayElement = 0;
				descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[3].descriptorCount = 1;
				descriptorWrites[3].pBufferInfo = &materialBufferInfo;

				descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[4].dstSet = descriptorSets[i][j];
				descriptorWrites[4].dstBinding = 4;
				descriptorWrites[4].dstArrayElement = 0;
				descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[4].descriptorCount = 1;
				descriptorWrites[4].pBufferInfo = &lightBufferInfo;

				vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
					descriptorWrites.data(), 0, nullptr);
				vulkanMaterial.descriptorSet = descriptorSets[i][j]; // 将描述符集存储在材质中
			}
		}
	}

	//创建描述符池
	void createDescriptorPool() {
		// 计算所需的描述符数量
		uint32_t uniformBufferCount = 0;
		uint32_t samplerCount = 0;

		for (const auto& materials : vulkanMaterials) {
			//每个材质需要一个采样器和四个 uniform buffer，分别是观察点，材质属性和光源属性还有物体的模型视图投影矩阵
			uniformBufferCount += materials.size() * 4;
			samplerCount += materials.size();
		}
		
		//还需要两个后处理的采样器,和一个统一缓冲
		samplerCount += meshObjects.size()*2;
		uniformBufferCount += meshObjects.size();
		uint32_t maxSets = uniformBufferCount; // 每个材质对应一个描述符集

		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = uniformBufferCount;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = samplerCount;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = maxSets;

		// 添加VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT标志允许释放单个描述符集
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}

	void createDescriptorSetLayoutBlend() {
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;

		//指定着色器阶段,我们将使用 VK_SHADER_STAGE_VERTEX_BIT 来指定顶点着色器阶段
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding samplerLayoutBinding1{};
		samplerLayoutBinding1.binding = 2;
		samplerLayoutBinding1.descriptorCount = 1;
		samplerLayoutBinding1.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding1.pImmutableSamplers = nullptr;
		samplerLayoutBinding1.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		std::array<VkDescriptorSetLayoutBinding, 3> bindings = {
			uboLayoutBinding,
			samplerLayoutBinding,
			samplerLayoutBinding1
		};
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();


		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayoutBlend) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}
	//创建着色器布局
	void createDescriptorSetLayout() {
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;

		//指定着色器阶段,我们将使用 VK_SHADER_STAGE_VERTEX_BIT 来指定顶点着色器阶段
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		// 2. 观察者位置布局绑定
		VkDescriptorSetLayoutBinding eyePositionLayoutBinding{};
		eyePositionLayoutBinding.binding = 2;
		eyePositionLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		eyePositionLayoutBinding.descriptorCount = 1;
		eyePositionLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		eyePositionLayoutBinding.pImmutableSamplers = nullptr;

		// 3. 材质属性布局绑定
		VkDescriptorSetLayoutBinding materialLayoutBinding{};
		materialLayoutBinding.binding = 3;
		materialLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		materialLayoutBinding.descriptorCount = 1;
		materialLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		materialLayoutBinding.pImmutableSamplers = nullptr;

		// 4. 光源属性布局绑定
		VkDescriptorSetLayoutBinding lightLayoutBinding{};
		lightLayoutBinding.binding = 4;
		lightLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		lightLayoutBinding.descriptorCount = 1;
		lightLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		lightLayoutBinding.pImmutableSamplers = nullptr;

		std::array<VkDescriptorSetLayoutBinding, 5> bindings = { 
			uboLayoutBinding, 
			samplerLayoutBinding,
			eyePositionLayoutBinding,
			materialLayoutBinding,
			lightLayoutBinding
		};
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();


		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}

		

	}

	

	//创建缓冲区
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex =VulkanUtils:: findMemoryType(physicalDevice,memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate buffer memory!");
		}

		vkBindBufferMemory(device, buffer, bufferMemory, 0);

	}

	//复制缓冲区
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		VkCommandBuffer commandBuffer =VulkanUtils:: beginSingleTimeCommands(device,commandPool);

		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		VulkanUtils:: endSingleTimeCommands(device,commandPool,graphicsQueue,commandBuffer);
	}


	void uploadDataToBuffer(const void* data, VkDeviceSize size, VkBuffer buffer) {
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory);

		void* mappedData;
		vkMapMemory(device, stagingBufferMemory, 0, size, 0, &mappedData);
		memcpy(mappedData, data, static_cast<size_t>(size));
		vkUnmapMemory(device, stagingBufferMemory);

		copyBuffer(stagingBuffer, buffer, size);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	//创建命令缓冲区
	void createCommandBuffers() {

		commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		//指定命令缓冲区的级别,我们将使用 VK_COMMAND_BUFFER_LEVEL_PRIMARY 级别的命令缓冲区。
		//主命令缓冲区可以直接提交到队列，而二级命令缓冲区只能由主命令缓冲区调用。VK_COMMAND_BUFFER_LEVEL_SECONDARY
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		//指定命令缓冲区的数量
		allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

		if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}
	}

	//将命令写入命令缓冲区，以及想要写入的图像索引
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
		

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderer->getRenderPass();
		renderPassInfo.framebuffer = BlendFramebuffers[imageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChainExtent;

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->getGraphicsPipeline());

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapChainExtent.width);
		viewport.height = static_cast<float>(swapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent; 
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		for (size_t i = 0; i < meshObjects.size(); i++) {
			const auto& vulkanMesh = vulkanMeshes[i];

			// 绑定统一的顶点缓冲区和索引缓冲区
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vulkanMesh.vertexBuffer, offsets);
			vkCmdBindIndexBuffer(commandBuffer, vulkanMesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			// 绘制每个 Mesh
			for (const auto& drawInfo : vulkanMesh.materialDrawInfos) {
				// 绑定对应的材质描述集
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					renderer->getPipelineLayout(), 0, 1,
					&descriptorSets[i][drawInfo.materialIndex], 0, nullptr);

				// 绘制当前 Mesh
				vkCmdDrawIndexed(commandBuffer, drawInfo.indexCount, 1, drawInfo.indexOffset, 0, 0);
			}
		}

		vkCmdEndRenderPass(commandBuffer);

		
	}


	void recordCommandBufferBlend(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
		

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = colorBlendRenderer ->getRenderPass();
		renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChainExtent;

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, colorBlendRenderer ->getGraphicsPipeline());

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapChainExtent.width);
		viewport.height = static_cast<float>(swapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		for (size_t i = 0; i < meshObjects.size(); i++) {
			const auto& vulkanMesh = vulkanMeshes[i];

			// 绑定统一的顶点缓冲区和索引缓冲区
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vulkanMesh.vertexBuffer, offsets);
			vkCmdBindIndexBuffer(commandBuffer, vulkanMesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			// 绘制每个 Mesh
			for (const auto& drawInfo : vulkanMesh.materialDrawInfos) {
				// 绑定对应的材质描述集
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					 colorBlendRenderer->getPipelineLayout(), 0, 1,
					&descriptorSetBlend[i], 0, nullptr);

				// 绘制当前 Mesh
				vkCmdDrawIndexed(commandBuffer, drawInfo.indexCount, 1, drawInfo.indexOffset, 0, 0);
			}
		}

		vkCmdEndRenderPass(commandBuffer);

		
	}

	//创建命令池
	void createCommandPool() {
		//获取队列族索引
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		//指定命令池的类型,我们将使用 VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
		// 标志来允许我们在命令缓冲区被提交后重用它们。
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}


	}

	//创建帧缓冲区
	void createFramebuffers() {
		swapChainFramebuffers.resize(swapChainImageViews.size());
		BlendFramebuffers.resize(swapChainImageViews.size());
		//遍历图像视图，创建帧缓冲区
		for (size_t i = 0; i < swapChainImageViews.size(); i++) {
			std::array<VkImageView, 3> attachments = {
				//这个顺序要和渲染通道的颜色附件和深度附件定义时指定的顺序一致
				resolveImageView[0],
				depthImageView,
				resolveImageView[1]
			};
			std::array<VkImageView, 1> attachments1 = {
				swapChainImageViews[i]
			};
			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderer->getRenderPass();
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			//指定图像的层数,如果是立体视觉交换链，则为2（左右眼）
			framebufferInfo.layers =1;

			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &BlendFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}

			
			framebufferInfo.renderPass = colorBlendRenderer->getRenderPass();
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments1.size());
			framebufferInfo.pAttachments = attachments1.data();
			//指定图像的层数,如果是立体视觉交换链，则为2（左右眼）
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
	}


	//创建着色器模块
	//VkShaderModule createShaderModule(const std::vector<char>& code) {
	//	VkShaderModuleCreateInfo createInfo{};
	//	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	//	createInfo.codeSize = code.size();
	//	//vector默认满足对齐要求，所以可以直接转换为uint32_t*
	//	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	//	VkShaderModule shaderModule;
	//	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
	//		throw std::runtime_error("failed to create shader module!");
	//	}
	//	return shaderModule;
	//}


	//创建图像视图
	void createImageViews() {
		swapChainImageViews.resize(swapChainImages.size());

		for (uint32_t i = 0; i < swapChainImages.size(); i++) {
			swapChainImageViews[i] =VulkanUtils:: createImageView(device,swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
			//std::cout << "swapChainImageViews created：" << (int*)swapChainImageViews[i] << std::endl;
		}
	}


	//创建交换链
	void createSwapChain() {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
		
		//交换链创建信息
		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		//0表示不限制交换链的图像数量
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}


		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		//除非是立体视觉交换链，否则图像层数必须为1
		createInfo.imageArrayLayers = 1;
	
		/*也有可能您会先将图像渲染到单独的图像上以执行诸如后期处理之类的操作。在这种情况下，
		您可以使用类似  VK_IMAGE_USAGE_TRANSFER_DST_BIT  的值，并使用内存操作将渲染的图像传输到交换链图像*/
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

		/*compositeAlpha  字段指定是否应使用 alpha 通道与其他窗口系统中的窗口进行混合。
		您几乎总是希望简单地忽略 alpha 通道，因此选择  VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR 。*/
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		//指定交换链的呈现模式
		createInfo.presentMode = presentMode;
		/* presentMode  这个成员的含义不言自明。如果将  clipped  这个成员设置为  VK_TRUE
		那就意味着我们不关心被遮挡像素的颜色，例如因为另一个窗口在它们前面。
		除非您确实需要能够读取这些像素并获得可预测的结果，否则启用裁剪将获得最佳性能。*/
		createInfo.clipped = VK_TRUE;

		//假设我们不需要在交换链被销毁时保留旧的交换链图像。
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
			throw std::runtime_error("failed to create swap chain!");
		}

		//获取交换链图像
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
		//获取交换链图像格式和范围
		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}


	//查询交换链支持信息
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
		SwapChainSupportDetails details;
		//查询表面支持信息
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		//查询表面支持的格式
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		//查询表面支持的呈现模式
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	//选择交换链的表面格式
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	//选择交换链的呈现模式
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
		for (const auto& availablePresentMode : availablePresentModes) {
			//选择三重缓冲,实际上移动设备选择VK_PRESENT_MODE_FIFO_KHR模式更好，能耗更低
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	//选择交换链的范围
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		else {
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};
			// 此处使用clamp函数将width和height的值限制在实现所支持的允许最小值和最大值范围内。
			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
			//std::cout << "Available extension: " << extension.extensionName << std::endl;
		}
		
		// 检查是否支持多视图
		VkPhysicalDeviceMultiviewFeatures multiviewFeatures{};
		multiviewFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES;
		
		VkPhysicalDeviceFeatures2 features2{};
		features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		features2.pNext = &multiviewFeatures;
		
		vkGetPhysicalDeviceFeatures2(physicalDevice, &features2);

		if (!multiviewFeatures.multiview) {
			throw std::runtime_error("设备不支持多视图！");
		}
		return requiredExtensions.empty();
	}


	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
		QueueFamilyIndices indices;
		// Assign index to queue families that could be found

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
		/*
		` VkQueueFamilyProperties ` 结构体包含有关队列族的一些详细信息，
		包括支持的操作类型以及基于该族可创建的队列数量。我们需要找到
		至少一个支持 ` VK_QUEUE_GRAPHICS_BIT ` 的队列族。
		*/
		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}
			//检查队列族是否支持显示
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			if (presentSupport) {
				indices.presentFamily = i;
			}
			//找到一个就停止
			if (indices.isComplete()) {
				break;
			}

			i++;
		}

		return indices;
	}



	void createSurface() {
		//创建窗口表面
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}

	//创建逻辑设备
	void createLogicalDevice() {

		//获取队列族索引
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);


		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.sampleRateShading = VK_TRUE;
		//创建逻辑设备
		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;


		// 确认设备支持各向异性过滤
		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

		if (supportedFeatures.samplerAnisotropy) {
			deviceFeatures.samplerAnisotropy = VK_TRUE;
		}
		else {
			std::cout << "Warning: Device does not support anisotropy filtering" << std::endl;
			deviceFeatures.samplerAnisotropy = VK_FALSE;
		}

		// 将deviceFeatures传递给createInfo
		createInfo.pEnabledFeatures = &deviceFeatures;


		//队列创建信息
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		//启用设备扩展
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		VkPhysicalDeviceMultiviewFeatures multiviewFeatures{};
		multiviewFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES;
		multiviewFeatures.multiview = VK_TRUE; // 启用多视图功能
		createInfo.pNext = &multiviewFeatures; // 将 multiviewFeatures 链接到 pNext
		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device!");
		}

		
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
		if (deviceProperties.apiVersion < VK_API_VERSION_1_4) {
			throw std::runtime_error("Device does not support Vulkan 1.4 or higher!");
		}

		//获取最小对齐
		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
		minimumUboAlignment = deviceProperties.limits.minUniformBufferOffsetAlignment;
		/*
		我们可以使用  vkGetDeviceQueue  函数来获取每个队列族的队列句柄。参数包括逻辑设备、队列族、队列索引以
		及一个用于存储队列句柄的变量的指针。由于我们仅从这个队列族创建一个队列，
		因此我们将简单地使用索引0  。*/
		//获取队列
		/*如果队列族相同，那么这两个句柄现在很可能具有相同的值*/
		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
	}

	//选择物理设备
	void pickPhysicalDevice() {
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		if (deviceCount == 0) {
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
		//选择物理设备
		std::multimap<int, VkPhysicalDevice> candidates;

		for (const auto& device : devices) {
			int score = rateDeviceSuitability(device);
			candidates.insert(std::make_pair(score, device));
		}

		// Check if the best candidate is suitable at all
		if (candidates.rbegin()->first > 0) {
			physicalDevice = candidates.rbegin()->second;
		}
		//检查物理设备是否适合
		if (physicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("failed to find a suitable GPU!");
		}
		isDeviceSuitable(physicalDevice);

	}

	//评分设备适用性
	int rateDeviceSuitability(VkPhysicalDevice device) {
	
		int score = 0;
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
		// Discrete GPUs have a significant performance advantage
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			score += 1000;
		}

		// Maximum possible size of textures affects graphics quality
		score += deviceProperties.limits.maxImageDimension2D;

		// Application can't function without geometry shaders
		if (!deviceFeatures.geometryShader) {
			return 0;
		}

		return score;
	}


	//检查物理设备是否适合
	bool isDeviceSuitable(VkPhysicalDevice device) {
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		//获取物理设备属性
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		//检查物理设备是否支持几何着色器
		bool physicalDeviceSuitable = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader;

		//检查物理设备是否支持各向异性过滤
		physicalDeviceSuitable = physicalDeviceSuitable && deviceFeatures.samplerAnisotropy;
		bool extensionsSupported = checkDeviceExtensionSupport(device);

		//检查队列族是否支持
		QueueFamilyIndices indices = findQueueFamilies(device);

		//检查交换链支持
		bool swapChainAdequate = false;
		if (extensionsSupported) {
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		return physicalDeviceSuitable && indices.isComplete() && extensionsSupported && swapChainAdequate;
	}

	//创建调试工具
	VkResult CreateDebugUtilsMessengerEXT(
		VkInstance instance, 
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
		const VkAllocationCallbacks* pAllocator, 
		VkDebugUtilsMessengerEXT* pDebugMessenger) {
			//获取函数指针
			auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
			if (func != nullptr) {
				return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
			}
			else {
				return VK_ERROR_EXTENSION_NOT_PRESENT;
			}
	}


	//销毁调试工具
	static void DestroyDebugUtilsMessengerEXT(
		VkInstance instance, 
		VkDebugUtilsMessengerEXT debugMessenger, 
		const VkAllocationCallbacks* pAllocator) {
			//获取函数指针
			auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
			if (func != nullptr) {
				func(instance, debugMessenger, pAllocator);
			}
	}

	//清理内存,顺序不能错
	void cleanup() {
		//销毁同步对象
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device, inFlightFences[i], nullptr);
		}

		
		//销毁交换链
		cleanupSwapChain();
		//销毁纹理图像
		vkDestroySampler(device, textureSampler, nullptr);
		vkDestroyImageView(device, textureImageView, nullptr);
		vkDestroyImage(device, textureImage, nullptr);
		vkFreeMemory(device, textureImageMemory, nullptr);

		// 销毁 Vulkan 材质资源
		for (const auto& materialList : vulkanMaterials) {
			for (const auto& material : materialList) {
				if (material.descriptorSet != VK_NULL_HANDLE) {
					vkFreeDescriptorSets(device, descriptorPool, 1, &material.descriptorSet);
				}
				if (material.textureSampler != VK_NULL_HANDLE) {
					vkDestroySampler(device, material.textureSampler, nullptr);
				}
				if (material.diffuseImageView != VK_NULL_HANDLE) {
					vkDestroyImageView(device, material.diffuseImageView, nullptr);
				}
				if (material.diffuseTexture != VK_NULL_HANDLE) {
					vkDestroyImage(device, material.diffuseTexture, nullptr);
				}
				if (material.diffuseTextureMemory != VK_NULL_HANDLE) {
					vkFreeMemory(device, material.diffuseTextureMemory, nullptr);
				}
			}
		}

		for (const auto& mesh : vulkanMeshes) {
			vkDestroyBuffer(device, mesh.vertexBuffer, nullptr);
			vkFreeMemory(device, mesh.vertexBufferMemory, nullptr);
			vkDestroyBuffer(device, mesh.indexBuffer, nullptr);
			vkFreeMemory(device, mesh.indexBufferMemory, nullptr);
		}

		for (const auto& uniformBuffer : perObjectUniformBuffers) {
			for (auto & buffer:uniformBuffer.buffers) {
				vkDestroyBuffer(device, buffer, nullptr);
			}
			for (auto& memery : uniformBuffer.memories) {
				vkFreeMemory(device, memery, nullptr);
			}
		}
		for (size_t i = 0; i < BlendFramebuffers.size(); i++) {
			vkDestroyFramebuffer(device, BlendFramebuffers[i], nullptr);
		}
		// 销毁深度图像
		if (depthImageView != VK_NULL_HANDLE) {
			vkDestroyImageView(device, depthImageView, nullptr);
			depthImageView = VK_NULL_HANDLE;
		}
		if (depthImage != VK_NULL_HANDLE) {
			vkDestroyImage(device, depthImage, nullptr);
			depthImage = VK_NULL_HANDLE;
		}
		if (depthImageMemory != VK_NULL_HANDLE) {
			vkFreeMemory(device, depthImageMemory, nullptr);
			depthImageMemory = VK_NULL_HANDLE;
		}

	
		for (int i = 0; i < 2; i++) {
			if (resolveImage[i] != VK_NULL_HANDLE) {
				vkDestroyImage(device, resolveImage[i], nullptr);
				resolveImage[i] = VK_NULL_HANDLE;
			}
			if (resolveImageMemory[i] != VK_NULL_HANDLE) {
				vkFreeMemory(device, resolveImageMemory[i], nullptr);
				resolveImageMemory[i] = VK_NULL_HANDLE;
			}
		}
		
		for (int i = 0; i < 4; i++) {
			if (resolveImageView[i] != VK_NULL_HANDLE) {
				vkDestroyImageView(device, resolveImageView[i], nullptr);
				resolveImageView[i] = VK_NULL_HANDLE;
			}
		}

		vkDestroyDescriptorPool(device, descriptorPool, nullptr);

		for (auto& des : descriptorSetBlend) {
			vkFreeDescriptorSets(device, descriptorPool, 1, &des);
		}

		//销毁描述符池
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayoutBlend, nullptr);
	
		
		delete renderer;
		delete colorBlendRenderer;
		//销毁命令池
		vkDestroyCommandPool(device, commandPool, nullptr);
		//销毁逻辑设备
		vkDestroyDevice(device, nullptr);
		if (enableValidationLayers) {
			//销毁调试工具
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}
		vkDestroySurfaceKHR(instance, surface, nullptr);
		/*确保在实例化之前破坏表面。*/
		vkDestroyInstance(instance, nullptr);
		glfwDestroyWindow(window);
		glfwTerminate();
	}


	//检查验证层是否支持
	bool checkValidationLayerSupport() {
		
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
		
		for (const char* layerName : validationLayers) {
			bool layerFound = false;
			
			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				return false;
			}
		}
		return true;
	}

	//调试回调函数
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
	}

	//调试回调函数
	void setupDebugMessenger() {
		if (!enableValidationLayers) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);
		/*
		倒数第二个参数再次是可选的分配器回调函数，我们将其设置为  nullptr ，除此之外，参数都相当直观。由于调试消息传递器特定于我们的 Vulkan 实例及其层，
		因此需要将其作为第一个参数明确指定。您稍后还会在其他子对象中看到这种模式。
		*/
		if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}

	//调试回调函数
	std::vector<const char*> getRequiredExtensions() {
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (enableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

/*
第一个参数指定了消息的严重程度，它是以下标志之一：
VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:诊断消息
VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:诸如创建资源之类的信息性消息
VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: 关于行为的消息，这不一定是个错误，但很可能是您应用程序中的一个漏洞。
VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: 有关无效行为的消息，该行为可能导致崩溃
此枚举的值设置方式使得您可以使用比较操作来检查消息是否等于或低于某个严重级别，例如：
if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    // Message is important enough to show
}
 messageType  参数可以具有以下值：
VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: 发生了一些与规格或性能无关的事件。
VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: 发生了违反规范或可能表明出错的情况。
VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: 潜在的非最优 Vulkan 使用情况

 pCallbackData  参数指的是一个  VkDebugUtilsMessengerCallbackDataEXT  结构体，其中包含消息本身的详细信息，最重要的成员有：
pMessage: 调试消息作为以空字符结尾的字符串
 pObjects ：与消息相关的 Vulkan 对象句柄数组
objectCount: 数组中的对象数量
最后， pUserData  参数包含一个指针，该指针是在设置回调函数时指定的，允许您向回调函数传递自己的数据。

回调函数会返回一个布尔值，该值表明触发验证层消息的Vulkan 调用是否应被中止。如果回调函数返回 true，则该调用将被中止，并返回VK_ERROR_VALIDATION_FAILED_EXT错误。
这通常仅用于测试验证层本身，因此您应始终返回VK_FALSE
*/
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {

		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}


	//创建实例
	void createInstance() {

		//检查验证层是否支持
		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("validation layers requested, but not available!");
		}

		//创建应用程序实例,使用列表初始化，将结构体的所有成员初始化为零，包括扩展pNext链表和其他结构体。
		VkApplicationInfo appInfo{};
		//指定应用程序的信息
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_4;

		

		//创建实例
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		//获取扩展
		auto extensions = getRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();
		//启用扩展
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
			//启用调试回调函数
			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else {
			createInfo.enabledLayerCount = 0;

			createInfo.pNext = nullptr;
		}
		//创建实例
		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}
	}
};

int main() {

	HelloTriangleApplication app;
	try {
		
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
