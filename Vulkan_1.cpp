#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <map>
#include <optional>
#include <set>
#include <cstdint> // Necessary for uint32_t
#include <limits> // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp
#include <fstream>
#include <string>
#include <glm/glm.hpp>
#include <array>
//定义同时使用的最大帧数
const int MAX_FRAMES_IN_FLIGHT = 2;

struct Vertex {
	glm::vec2 pos;
	glm::vec3 color;
	//指定顶点输入约束性属性描述
	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		//指定顶点输入属性的偏移量,我们将使用 sizeof(Vertex) 来指定偏移量
		bindingDescription.stride = sizeof(Vertex);
		//这里可以使用实例化渲染，但是我们不需要，所以我们将其设置为 VK_VERTEX_INPUT_RATE_VERTEX
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	//指定顶点输入属性描述
	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}

};
//顶点数据,我们将使用一个三角形来测试渲染管线
const std::vector<Vertex> vertices = {
	{{0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}},
	{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
	{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};

class HelloTriangleApplication {
public:
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	};

	//读取文件内容到vector<char>中
	static std::vector<char> readFile(const std::string& filename) {
		//std::ios::ate表示打开文件时将文件指针移动到文件末尾，std::ios::binary表示以二进制模式打开文件
		//从文件末尾开始读取的优势在于，我们可以利用读取位置来确定文件的大小并分配缓冲区：
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		//文件大小
		if (!file.is_open()) {
			throw std::runtime_error("failed to open file!");
		}
		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;

	}

	//获取扩展
	void getExtensions() {
		uint32_t extensionCount = 0;
		//获取扩展数量
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		//获取扩展属性
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
		//打印扩展
		std::cout << "available extensions:" << std::endl;
		for (const auto& extension : extensions) {
			std::cout << '\t' << extension.extensionName << std::endl;
		}
	}


private:
	const uint32_t WIDTH = 800;
	const uint32_t HEIGHT = 600;
	GLFWwindow* window;
	VkInstance instance;
	//物理设备,即显卡,该设备在销毁实例时会自动销毁
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	//调试回调函数
	VkDebugUtilsMessengerEXT debugMessenger;

	//当前使用的帧索引
	uint32_t currentFrame = 0;

	//顶点缓冲区
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
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
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
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

	//同步对象
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	//标记
	bool framebufferResized = false;

	//图形管线
	VkRenderPass renderPass;
	VkPipeline graphicsPipeline;
	VkPipelineLayout pipelineLayout;
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
		//等待命令缓冲可用，可接受一个栅栏数组，第三个参数指定等待的栅栏数量（任一或者全部），第四个参数指定等待的时间
		vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
		
		
		//获取交换链图像索引
		uint32_t imageIndex;
		/* vkAcquireNextImageKHR ` 的前两个参数是逻辑设备和我们希望从中获取图像的交换链。
		第三个参数指定了图像可用的超时时间（以纳秒为单位）。使用 64 位无符号整数的最大值意味着
		我们实际上禁用了超时。接下来的两个参数指定了在呈现引擎完成使用图像时要发出信号的同步对象。
		这是我们可以开始向其绘制的时刻。可以指定一个信号量、栅栏或两者都指定。在这里，我们将为此目的使
		用我们的  imageAvailableSemaphore  。最后一个参数指定了一个变量，用于输出已变为可用状态的交
		换链图像的索引。该索引指的是我们  swapChainImages  数组中的  VkImage  。
		我们将使用该索引来选取VkFrameBuffer。*/
		//查看交换链是否适配，如果不适配则重新创建交换链
		VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		//手动重置栅栏
		vkResetFences(device, 1, &inFlightFences[currentFrame]);

		vkResetCommandBuffer(commandBuffers[currentFrame], 0);
		recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

		//提交命令缓冲区
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		
		//前三个参数指定了在执行开始前要等待哪些信号量以及在图形管线的哪个（些）阶段进行等待。
		// 我们希望在图像可用之前不要将颜色写入图像，因此我们指定了图形管线中写入颜色附件的阶段。
		// 这意味着理论上，在图像尚未可用时，实现可以已经开始执行我们的顶点着色器等操作。 
		// waitStages  数组中的每个条目都对应于  pWaitSemaphores  中具有相同索引的信号量。
		VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		//指定要执行的命令缓冲区
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

		//指定要发出信号的信号量
		VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr; // Optional




		//提交交换链图像
		result = vkQueuePresentKHR(presentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
			framebufferResized = false;
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}

		//切换到下一个帧
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
		createFramebuffers();
	}

	void cleanupSwapChain() {
		for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
			vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
		}

		for (size_t i = 0; i < swapChainImageViews.size(); i++) {
			vkDestroyImageView(device, swapChainImageViews[i], nullptr);
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
		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandPool();
		createVertexBuffer();
		createCommandBuffers();
		createSyncObjects();
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
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate buffer memory!");
		}

		vkBindBufferMemory(device, buffer, bufferMemory, 0);

	}

	//复制缓冲区
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		//使用临时命令缓冲区来复制缓冲区
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		//指定命令缓冲区的使用方式,我们将使用 VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT 
		// 标志来指定命令缓冲区只会被提交一次。
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0; // Optional
		copyRegion.dstOffset = 0; // Optional
		copyRegion.size = size;
		//复制缓冲区,我们将使用 vkCmdCopyBuffer 函数来复制缓冲区
		//第一个参数是命令缓冲区，第二个参数是源缓冲区，第三个参数是目标缓冲区
		//第四个参数是复制区域的数量，我们将使用 1 来指定一个复制区域
		//第五个参数是复制区域的指针，我们将使用 &copyRegion 来指定复制区域
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
		vkEndCommandBuffer(commandBuffer);
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);
		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	void createVertexBuffer() {
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
		
		//映射内存到应用程序地址空间
		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		//复制数据到内存
		memcpy(data, vertices.data(), (size_t)bufferSize);
		//取消映射内存
		 vkUnmapMemory(device, stagingBufferMemory);

		 createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
		 copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		 //销毁临时缓冲区
		 vkDestroyBuffer(device, stagingBuffer, nullptr);
		 vkFreeMemory(device, stagingBufferMemory, nullptr);

	}

	//找到合适的内存类型
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		//遍历内存类型，找到合适的内存类型。具体来说，我们需要找到一个内存类型，
		// 它的类型索引与 typeFilter 相同，并且它的属性标志与 properties 相同。
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");

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
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		//指定命令缓冲区的使用方式,
		// 我们可以使用VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT标志来允许命令缓冲区在多个队列上同时使用。
		//或者我们可以使用VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT标志来指定命令缓冲区只会被提交一次。
		//VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT: 这是一个辅助命令缓冲区，它将完全处于单个渲染通道内。
		//目前来说我们都不用到
		beginInfo.flags = 0; // Optional
		//只对二级命令缓冲区有用
		beginInfo.pInheritanceInfo = nullptr; // Optional

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		//开始渲染通道
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];

		//指定渲染区域,我们将使用交换链图像的大小来获得最佳效果
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChainExtent;

		//指定清除值,我们将使用黑色作为清除颜色
		VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		//提交命令缓冲区，vkCmd*函数返回值都是void类型，所以我们不需要检查返回值
		//第三个参数指定子通道的内容,我们将使用 VK_SUBPASS_CONTENTS_INLINE 来指定命令缓冲区的内容
		//VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: 该子通道的内容是二级命令缓冲区
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		//绑定图形管线，第二个参数指定管线绑定点,我们将使用 VK_PIPELINE_BIND_POINT_GRAPHICS 来指定图形管线
		//VK_PIPELINE_BIND_POINT_COMPUTE: 计算管线绑定点
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		VkBuffer vertexBuffers[] = { vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);


		//动态设置视口和剪裁矩形
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

		//绘制命令
		/*实际的  vkCmdDraw  函数有点令人失望，但因为它非常简单，所以才这样，这得益于我们提前指定的所有信息。
		除了命令缓冲区之外，它还有以下参数：
		vertexCount: 尽管我们没有顶点缓冲区，但从技术上讲，我们仍然有 3 个顶点要绘制。
		instanceCount: 用于实例化渲染，如果不是这种情况请使用1
		firstVertex: 用作顶点缓冲区的偏移量，定义了  gl_VertexIndex  的最小值。
		firstInstance: 用作实例渲染的偏移量，定义了  gl_InstanceIndex  的最小值。*/
		vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
		//vkCmdDrawIndexed: 用于索引绘制,我们没有使用索引缓冲区，所以我们不需要使用它
		//vkCmdDrawIndirect: 用于间接绘制,我们没有使用间接绘制，所以我们不需要使用它
		//vkCmdDrawIndexedIndirect: 用于间接索引绘制,我们没有使用间接索引绘制，所以我们不需要使用它
		//vkCmdDrawIndirectCount: 用于间接绘制计数,我们没有使用间接绘制计数，所以我们不需要使用它
		//vkCmdDrawIndexedIndirectCount: 用于间接索引绘制计数,我们没有使用间接索引绘制计数，所以我们不需要使用它
		//vkCmdDrawMeshTasksNV: 用于网格任务绘制,我们没有使用网格任务绘制，所以我们不需要使用它
		//vkCmdDrawMeshTasksIndirectNV: 用于间接网格任务绘制,我们没有使用间接网格任务绘制，所以我们不需要使用它
		//vkCmdDrawMeshTasksIndirectCountNV: 用于间接网格任务绘制计数,我们没有使用间接网格任务绘制计数，所以我们不需要使用它
		
		vkCmdEndRenderPass(commandBuffer);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
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

		//遍历图像视图，创建帧缓冲区
		for (size_t i = 0; i < swapChainImageViews.size(); i++) {
			VkImageView attachments[] = {
				swapChainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			//指定图像的层数,如果是立体视觉交换链，则为2（左右眼）
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
	}

	//创建渲染通道
	void createRenderPass() {
		//渲染通道描述
		VkAttachmentDescription colorAttachment{};
		//颜色附件的格式应与交换链图像的格式相匹配，而且我们目前还没有使用多重采样，所以就采用 1 次采样。
		colorAttachment.format = swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

		//指定颜色附件的加载操作和存储操作,有以下几种操作：
		//VK_ATTACHMENT_LOAD_OP_LOAD：在开始渲染之前从颜色附件中加载数据
		//VK_ATTACHMENT_LOAD_OP_CLEAR：在开始渲染之前清除颜色附件
		//VK_ATTACHMENT_LOAD_OP_DONT_CARE：不使用颜色附件中的数据
		//VK_ATTACHMENT_STORE_OP_STORE：在渲染完成后将颜色附件的内容存储到内存中
		//VK_ATTACHMENT_STORE_OP_DONT_CARE：不存储颜色附件的内容
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		//我们暂时不对模版缓冲区进行操作，所以我们将 stencilLoadOp 和 stencilStoreOp 
		// 设置为 VK_ATTACHMENT_LOAD_OP_DONT_CARE 和 VK_ATTACHMENT_STORE_OP_DONT_CARE。
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		//指定颜色附件的初始布局和最终布局，常见的布局有：
		//VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL：用作颜色附件时的布局
		//VK_IMAGE_LAYOUT_PRESENT_SRC_KHR：交换链图像的布局
		//VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL：用作内存传输的目标时的布局
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		/*initialLayout  指定了在渲染过程开始前图像将采用的布局。 finalLayout指定了在渲染过程结束后自动转换到的布局。
		使用 VK_IMAGE_LAYOUT_UNDEFINED 作为  initialLayout  意味着我们不在乎图像之前的布局是什么。
		这个特殊值的注意事项是图像的内容不能保证被保留，但这没关系，因为我们无论如何都要清除它。
		我们希望在渲染完成后图像能准备好通过交换链进行展示,这就是为什么我们将 VK_IMAGE_LAYOUT_PRESENT_SRC_KHR  
		用作 finalLayout 的原因。*/

		//渲染通道子通道和附件引用描述
		/*` attachment ` 参数通过其在附件描述数组中的索引来指定要引用的附件。我们的数组仅包含一个 ` VkAttachmentDescription `，
		因此其索引为 ` 0 `。` layout ` 指定了在使用此引用的子传递期间，我们希望附件采用的布局。
		当子传递开始时，Vulkan 会自动将附件转换为此布局。我们打算将附件用作颜色缓冲区，
		而 ` VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ` 布局将为我们提供最佳性能，正如其名称所暗示的那样。*/
		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		//子通道描述
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		//指定子通道的输入附件数量,并指定输入附件的引用
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		/*此数组中附件的索引通过  layout(location = 0) out vec4 outColor  指令直接从片段着色器中引用！
		以下其他类型的附件可以被子传递引用：
		pInputAttachments: 从着色器中读取的附件
		pResolveAttachments: 用于多重采样颜色附件的附件
		pDepthStencilAttachment: 用于深度和模板数据的附件
		pPreserveAttachments: 此子流程未使用的附件，但其中的数据必须保留*/

		//指定子通道的依赖关系
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;


		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;

		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;

		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}

	}


	//创建图形管线
	void createGraphicsPipeline() {
		
		auto vertShaderCode = readFile("shaders/vert.spv");
		auto fragShaderCode = readFile("shaders/frag.spv");

		/*着色器模块只是我们之前从文件中加载的着色器字节码以及其中定义的函数的一个薄薄的封装。
		SPIR-V 字节码编译和链接为 GPU 执行的机器代码的过程要到图形管线创建时才会发生。
		这意味着一旦管线创建完成，我们就可以再次销毁着色器模块，这就是为什么我们要在 
		createGraphicsPipeline  函数中将它们声明为局部变量，而不是类成员的原因。*/

		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		//着色器阶段创建信息
		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		//指定作用的渲染管线阶段
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		//指定着色器模块的入口函数
		vertShaderStageInfo.pName = "main";
		/*还有一个（可选）成员  pSpecializationInfo ，我们在此处不会使用它，但它值得讨论一下。
		它允许您为着色器常量指定值。您可以使用单个着色器模块，在创建管线时通过为其中使用的常量
		指定不同的值来配置其行为。这比在渲染时使用变量配置着色器更高效，因为编译器可以进行诸如
		消除依赖于这些值的  if  语句之类的优化。如果您没有这样的常量，则可以将该成员设置为  nullptr ，
		我们的结构体初始化会自动执行此操作。*/

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		//支持管线状态动态设置
		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		//因为我们之前在顶点着色器中没有使用任何顶点输入，所以我们将其设置为  nullptr 。
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();

		//绑定：数据之间的间距以及数据是按顶点还是按实例（参见实例化）
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; // Optional
		//属性描述：传递给顶点着色器的属性类型、从哪个绑定加载它们以及偏移量是多少
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); // Optional

		//输入装配状态
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		//视口和剪裁
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		//指定视口的最小和最大深度范围,其值范围为 0.0 到 1.0。
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		//创建剪裁矩形
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;
		
		//指定视口和剪裁矩形
		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.pScissors = &scissor;

		//光栅化状态
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		//如果启用深度夹紧，任何超出范围的片段都将被丢弃，而不是在深度缓冲区中写入无效值。
		//这对于实现后期处理效果很有用，例如模糊或阴影映射。
		rasterizer.depthClampEnable = VK_FALSE;
		//如果被设置为  VK_TRUE ，那么几何图形永远不会被光栅化。实际上会禁用向帧缓冲区的写入。
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		//指定多边形的填充模式
		/* VK_POLYGON_MODE_FILL  表示填充多边形，
		VK_POLYGON_MODE_LINE  表示绘制多边形的边界，
		VK_POLYGON_MODE_POINT  表示绘制多边形的顶点。
		除了填充模式之外，其他任何模式都需要启用GPU特性。
		*/ 
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

		//指定多边形的线宽，任何粗于 1.0 的线宽都需要启用 GPU 特性。
		rasterizer.lineWidth = 1.0f;

		//决定了要使用的面剔除模式，可以使用的值有：
		//VK_CULL_MODE_NONE：不剔除任何面
		//VK_CULL_MODE_FRONT_BIT：剔除前面
		//VK_CULL_MODE_BACK_BIT：剔除背面
		//VK_CULL_MODE_FRONT_AND_BACK：剔除前后面
		//VK_CULL_MODE_FRONT_AND_BACK  是最常用的值，因为它可以提高性能并减少不必要的片段着色器调用。
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		//指定多边形的前面是顺时针还是逆时针
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

		//指定多边形的深度偏移量
		/*光栅化器可以通过添加一个常量值或根据片段的斜率对其进行偏移来改变深度值。
		这有时用于阴影映射，但我们不会使用它。只需将depthBiasEnable设置为VK_FALSE即可。*/
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

		//多重采样,一般用来消除锯齿。暂时不使用
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f; // Optional
		multisampling.pSampleMask = nullptr; // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional

		//深度和模板测试
		//VkPipelineDepthStencilStateCreateInfo

		//颜色混合状态
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		//colorBlendAttachment.blendEnable = VK_FALSE;
		//colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		//colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		//colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
		//colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		//colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		//colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

		//启用混合，并且按照alpha通道进行混合
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		//指定是否启用逻辑操作，如果启用，则默认禁用混合
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional

		//管线布局
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0; // Optional
		pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
		pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
		pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}

		//管线创建信息
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
	
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr; // Optional
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		//指定管线布局
		pipelineInfo.layout = pipelineLayout;
		//指定渲染通道
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;

		/*此管线也可以与其他渲染通道配合使用，但这里我们不使用该功能*/
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!");
		}

		vkDestroyShaderModule(device, fragShaderModule, nullptr);
		vkDestroyShaderModule(device, vertShaderModule, nullptr);
	}

	//创建着色器模块
	VkShaderModule createShaderModule(const std::vector<char>& code) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		//vector默认满足对齐要求，所以可以直接转换为uint32_t*
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}
		return shaderModule;
	}


	//创建图像视图
	void createImageViews() {
		swapChainImageViews.resize(swapChainImages.size());

		for (size_t i = 0; i < swapChainImages.size(); i++) {
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages[i];

			/*viewType  和  format  字段指定了应如何解释图像数据。
			viewType  参数允许您将图像视为 1D 纹理、2D 纹理、3D 纹理和立方体映射。*/
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapChainImageFormat;
			//指定图像的组件格式
			/*“ components ”字段允许您交换颜色通道。例如，您可以将所有通道都映射到红色通道以生成单色纹理。
			您还可以将常量值 0 和 1 映射到某个通道。在我们的例子中，我们将使用默认映射。*/
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			//指定图像的子资源范围
			/*“ subresourceRange ”字段描述了图像的用途以及应访问图像的哪一部分。
			我们的图像将用作颜色目标，不包含任何 Mip 映射级别或多个图层。*/
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create image views!");
			}

		}
		/*如果您正在开发一个立体 3D 应用程序，那么您会创建一个具有多个层的交换链。
		然后，您可以为每个图像创建多个图像视图，分别代表左眼和右眼的视图，通过访问不同的层来实现。*/

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
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

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

		//创建逻辑设备
		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
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

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device!");
		}
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
		//销毁顶点缓冲区
		vkDestroyBuffer(device, vertexBuffer, nullptr);
		vkFreeMemory(device, vertexBufferMemory, nullptr);

		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		//销毁管线
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		//销毁管线
		vkDestroyRenderPass(device, renderPass, nullptr);
		
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
		appInfo.apiVersion = VK_API_VERSION_1_0;
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
		app.getExtensions();
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
