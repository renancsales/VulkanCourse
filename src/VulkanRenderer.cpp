#include "VulkanRenderer.h"

#include <cstring>

static const std::vector<const char*> s_ValidationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

int VulkanRenderer::Init(GLFWwindow* window)
{
	m_Window = window;

	try
	{
		CreateInstance();
		CreateSurface();
		GetPhysicalDevice();
		CreateLogicalDevice();		
		CreateSwapChain();
		CreateRenderPass();
		CreateDescriptorSetLayout();
		CreateGraphicsPipeline();
		CreateDepthBufferImage();
		CreateColorBufferImage();
		CreateFramebuffers();
		CreateCommandPool();
		CreateCommandBuffers();
		CreateTextureSampler();
		AllocateDynamicBufferTransferSpace();
		CreateUniformBuffers();
		CreateDescriptorPool();
		CreateDescriptorSets();
		CreateInputDescriptorSets();
		CreateSynchronization();

		 //int firstTexture = CreateTexture("src/Textures/bird_painting.jpg");

		// Set mvp
		m_Camera.Projection = glm::perspective(glm::radians(45.0f),
			(float)m_SwapchainExtent.width / (float)m_SwapchainExtent.height, 0.1f, 100.0f);
		m_Camera.View = glm::lookAt(glm::vec3(1.0f, 1.0f, 10.0f), glm::vec3(0.0f, 1.0f, -1.0f),
			glm::vec3(0.0f, 1.0f, 0.0f));

		// Inverse the z direction (similar to opengl)
		m_Camera.Projection[1][1] *= -1;

		// Create mesh object
		// vertex data
		std::vector<Vertex> meshVertices2 = {
			{{-0.1, -0.1, 0.0}, {0.0, 1.0, 0.0}, {1.0f, 1.0f}},
			{{0.1, -0.1, 0.0},  {0.0, 1.0, 0.0}, {1.0f, 0.0f} },
			{{0.1, 0.1, 0.0}, {0.0, 1.0, .5},    {0.0f, 0.0f}},
			{{-0.1, 0.1, 0.0}, {0.0, 1.0, 0.35}, {0.0f, 1.0f}}
		};

		std::vector<Vertex> meshVertices = {
			{{-0.2, -0.2, 1.0}, {1.0, 0.0, 0.0}, {1.0f, 1.0f}},
			{{0.2, -0.2, 1.0},  {1.0, 0.0, 0.0}, {1.0f, 0.0f}},
			{{0.2, 0.2, 1.0}, {1.0, 0.0, 1.0}, {0.0f, 0.0f}},
			{{-0.2, 0.2, 1.0}, {0.5, 0.5, 0.35}, {0.0f, 1.0f}}
		};

		/*std::vector<Vertex> meshVertices2 = {
			{{0.40, 0.40, 0.0}, {1.0, 0.0, 0.0}},
			{{0.50, 0.40, 0.0},  {1.0, 1.0, 0.0}},
			{{0.50, 0.50, 0.0}, {0.0, 1.0, 0.7}},
			{{0.40, 0.50, 0.0}, {0.5, 0.2, 0.35}}
		};*/
		// index data
		std::vector<uint32_t> meshIndices = {
			0, 1, 2,
			2, 3, 0
		};

		/*m_MeshList.push_back(Mesh(m_MainDevice.PhysicalDevice, m_MainDevice.LogicalDevice,
			m_GraphicsQueue, m_GraphicsCommandPool, &meshVertices, &meshIndices,
			CreateTexture("src/Textures/mario.png")));
		m_MeshList.push_back(Mesh(m_MainDevice.PhysicalDevice, m_MainDevice.LogicalDevice,
			m_GraphicsQueue, m_GraphicsCommandPool, &meshVertices2, &meshIndices,
			CreateTexture("src/Textures/bird_painting.jpg")));*/

		
		CreateMeshModel("src/Models/Sora/Sora.obj");
		// CreateMeshModel("C:/Users/engen/Pictures/desert_model/untitled.obj");
		CreateMeshModel("src/Models/Cactuar/cactuar.obj");
		CreateMeshModel("src/Models/Sora/Sora.obj");
		
	}
	catch (const std::runtime_error& e)
	{
		std::cout << "ERROR: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	
	return 0;
}

void VulkanRenderer::UpdateModel(uint32_t meshObjectIndex, glm::mat4& newModel)
{
	m_ModelList[meshObjectIndex].SetModel(newModel);
}

void VulkanRenderer::Draw()
{
	// 1. Get next available image to draw to and set something to signal when we're finished
	// with the image (a semaphore)
	
	// Wait for given fence to signal (open) from last draw before continuing
	vkWaitForFences(m_MainDevice.LogicalDevice, 1, &m_DrawFences[m_CurrentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
	// Manually reset (close) fences
	vkResetFences(m_MainDevice.LogicalDevice, 1, &m_DrawFences[m_CurrentFrame]);

	// -- Get next image
	uint32_t imageIndex;
	vkAcquireNextImageKHR(m_MainDevice.LogicalDevice, m_Swapchain, std::numeric_limits<uint64_t>::max(), m_SemaphoresImageAvailable[m_CurrentFrame],
		VK_NULL_HANDLE, &imageIndex);

	// rec
	RecordCommands(imageIndex);

	UpdateUniformBuffers(imageIndex);

	// 2. Submit command buffer to queue for execution, make sure it watis for the image to be 
	// signalled as available before drawing and signals when it has finished rendering
	// -- Submit command buffer to render
	// Queue submission info
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;				// number of semaphores to wait on
	submitInfo.pWaitSemaphores = &m_SemaphoresImageAvailable[m_CurrentFrame];
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.pWaitDstStageMask = waitStages;  // Stages to check semaphores at
	submitInfo.commandBufferCount = 1;			// number of command buffer to submit
	submitInfo.pCommandBuffers = &m_CommandBuffers[imageIndex];
	submitInfo.signalSemaphoreCount = 1;		// number of semaphores to signal
	submitInfo.pSignalSemaphores = &m_SemaphoresRenderFinished[m_CurrentFrame];		// Semaphores to signal when command buffer finishes

	// Submit command buffer to queue
	VkResult result = vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_DrawFences[m_CurrentFrame]);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to command buffer to queue!");
	}

	// 3. Present image to screen when it has signalled finished rendering
	// Present rendered image to screen
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_SemaphoresRenderFinished[m_CurrentFrame];		// semaphores to wait on
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_Swapchain;		// Swapchain to present images to
	presentInfo.pImageIndices = &imageIndex;	// index of images in swapchain to present

	result = vkQueuePresentKHR(m_PresentationQueue, &presentInfo);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to present rendererd image to screen!");
	}

	// only for max_frame_draw = 2
	m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAME_DRAWS;
}

void VulkanRenderer::CleanUp()
{
	// Wait until no action being run on device before destroying
	vkDeviceWaitIdle(m_MainDevice.LogicalDevice);

	// Clean all the meshes buffer
	for (size_t i = 0; i < m_ModelList.size(); i++)
	{
		m_ModelList[i].DestroyMeshModel();
	}

	vkDestroyDescriptorPool(m_MainDevice.LogicalDevice, m_InputDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(m_MainDevice.LogicalDevice, m_InputDescriptorSetLayout, nullptr);

	vkDestroyDescriptorPool(m_MainDevice.LogicalDevice, m_SamplerDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(m_MainDevice.LogicalDevice, m_SamplerDescriptorSetLayout, nullptr);

	vkDestroySampler(m_MainDevice.LogicalDevice, m_TextureSampler, nullptr);

	// Free texture memory
	for (size_t i = 0; i < m_TextureImages.size(); i++)
	{
		vkDestroyImageView(m_MainDevice.LogicalDevice, m_TextureImageViews[i], nullptr);
		vkDestroyImage(m_MainDevice.LogicalDevice, m_TextureImages[i], nullptr);
		vkFreeMemory(m_MainDevice.LogicalDevice, m_TextureImageMemory[i], nullptr);
	}

	// Clean depth buffer image
	for (size_t i = 0; i < m_DepthBufferImage.size(); i++)
	{
		vkDestroyImageView(m_MainDevice.LogicalDevice, m_DepthBufferImageView[i], nullptr);
		vkDestroyImage(m_MainDevice.LogicalDevice, m_DepthBufferImage[i], nullptr);
		vkFreeMemory(m_MainDevice.LogicalDevice, m_DepthBufferImageMemory[i], nullptr);
	}

	// Clean image buffer 
	for (size_t i = 0; i < m_ColorBufferImage.size(); i++)
	{
		vkDestroyImageView(m_MainDevice.LogicalDevice, m_ColorBufferImageView[i], nullptr);
		vkDestroyImage(m_MainDevice.LogicalDevice, m_ColorBufferImage[i], nullptr);
		vkFreeMemory(m_MainDevice.LogicalDevice, m_ColorBufferImageMemory[i], nullptr);
	}

	// Free object memories (dynamic buffer)
	_aligned_free(m_ModelTransferSpace);
	m_ModelTransferSpace = nullptr;


	vkDestroyDescriptorPool(m_MainDevice.LogicalDevice, m_DescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(m_MainDevice.LogicalDevice, m_DescriptorSetLayout, nullptr);

	for (size_t i = 0; i < m_SwapChainImages.size(); i++)
	{
		vkDestroyBuffer(m_MainDevice.LogicalDevice, m_UniformBuffers[i], nullptr);
		vkFreeMemory(m_MainDevice.LogicalDevice, m_UniformBufferMemory[i], nullptr);

		vkDestroyBuffer(m_MainDevice.LogicalDevice, m_UniformDynamicBuffers[i], nullptr);
		vkFreeMemory(m_MainDevice.LogicalDevice, m_UniformDynamicBufferMemory[i], nullptr);
	}


	for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
	{
		vkDestroySemaphore(m_MainDevice.LogicalDevice, m_SemaphoresImageAvailable[i], nullptr);
		vkDestroySemaphore(m_MainDevice.LogicalDevice, m_SemaphoresRenderFinished[i], nullptr);
		vkDestroyFence(m_MainDevice.LogicalDevice, m_DrawFences[i], nullptr);
	}

	vkDestroyCommandPool(m_MainDevice.LogicalDevice, m_GraphicsCommandPool, nullptr);

	for (size_t i = 0; i < m_SwapChainFramebuffers.size(); i++)
	{
		vkDestroyFramebuffer(m_MainDevice.LogicalDevice, m_SwapChainFramebuffers[i], nullptr);
	}

	// Destroy pipelines
	vkDestroyPipeline(m_MainDevice.LogicalDevice, m_SecondPipeline, nullptr);
	vkDestroyPipelineLayout(m_MainDevice.LogicalDevice, m_SecondPipelineLayout, nullptr);

	vkDestroyPipeline(m_MainDevice.LogicalDevice, m_GraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_MainDevice.LogicalDevice, m_PipelineLayout, nullptr);

	vkDestroyRenderPass(m_MainDevice.LogicalDevice, m_RenderPass, nullptr);
	for (auto image : m_SwapChainImages)
	{
		vkDestroyImageView(m_MainDevice.LogicalDevice, image.ImageView, nullptr);
	}
	vkDestroySwapchainKHR(m_MainDevice.LogicalDevice, m_Swapchain, nullptr);
	vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
	vkDestroyDevice(m_MainDevice.LogicalDevice, nullptr);
	vkDestroyInstance(m_Instance, nullptr);
}

void VulkanRenderer::CreateInstance()
{
	if (enableValidationLayers && !CheckValidationLayerSupport())
	{
		throw std::runtime_error("Validation layers requested, but not available!");
	}

	// Information about the application itself
	// Most data here doesnt affect the program and is for developer convenience
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan App"; // Custom name of the app
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3; // vulkan version

	// Create information for vkInstance
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	// Create a list to hold instance extensions
	std::vector<const char*> instanceExtensions = std::vector<const char*>();

	// Set up the extensions that Instance will use
	uint32_t glfwExtensionCount = 0; // glfw may require multiple extensions
	const char** glfwExtensions;	// Extensions passed as array of cstrings

	// get glfw extensions
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	// Add glfw extensions to list of extensions
	for (size_t i = 0; i < glfwExtensionCount; i++)
		instanceExtensions.push_back(glfwExtensions[i]);

	
	if (!CheckInstanceExtensionSupport(&instanceExtensions))
	{
		throw std::runtime_error("VkInstance does not support required extensions!");
	}

	createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();

	// TODO: Set up validation layers that instance will use
	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(s_ValidationLayers.size());
		createInfo.ppEnabledLayerNames = s_ValidationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}
	

	//Create instance
	VkResult result = vkCreateInstance(&createInfo, nullptr, &m_Instance);
	
	// Check
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Vulkan Instance!");
	}

}

void VulkanRenderer::CreateLogicalDevice()
{
	if (enableValidationLayers && !CheckValidationLayerSupport())
	{
		throw std::runtime_error("Validation layers requested, but not available!");
	}

	// Get queue family indices for the chosen physical device
	QueueFamilyIndices indices = GetQueueFamilies(m_MainDevice.PhysicalDevice);

	// Vector for queue creation information and set for family indices
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> queueFamilyIndices = { indices.GraphicsFamily, indices.PresentationFamily };



	// Queue the logical device needs to create and info to do so 
	for (int queueFamilyIndex : queueFamilyIndices)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
		queueCreateInfo.queueCount = 1;			// number of queues to create
		float priority = 1.0f;
		queueCreateInfo.pQueuePriorities = &priority;

		queueCreateInfos.push_back(queueCreateInfo);
	}
	
	// Information to create logical device (sometimes called "devices")
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data(); // list of queue create infos. 
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(s_DeviceExtensions.size());		// number of enabled logical device extensions
	deviceCreateInfo.ppEnabledExtensionNames = s_DeviceExtensions.data(); // list of enabled logical device extensions

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;		// Enable anisotropy

	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;  // physical device feature will use
	
	if (enableValidationLayers)
	{
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(s_ValidationLayers.size());		// number of enabled logical device extensions
		deviceCreateInfo.ppEnabledLayerNames = s_ValidationLayers.data(); // list of enabled logical device extensions
	}

	// Create the logical device for the given physical device
	VkResult result = vkCreateDevice(m_MainDevice.PhysicalDevice, &deviceCreateInfo, nullptr, &m_MainDevice.LogicalDevice);
	// Check
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Logical Device!");
	}

	// Queues are created at the same time as devices
	// handle to queues
	// From given logical device, of given queue family, of given queue index, place reference in vkQueue
	vkGetDeviceQueue(m_MainDevice.LogicalDevice, indices.GraphicsFamily, 0, &m_GraphicsQueue);
	vkGetDeviceQueue(m_MainDevice.LogicalDevice, indices.PresentationFamily, 0, &m_PresentationQueue);
}

void VulkanRenderer::CreateSurface()
{
	// Create a surface create info struct, create surface function (glfw vulkan wrapper)
	VkResult result = glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &m_Surface);

	// Check
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a window surface!");
	}
}

void VulkanRenderer::CreateSwapChain()
{
	// Get swap chain details so we can pick the best formats
	SwapChainDetails swapChainDetails = GetSwapChainDetails(m_MainDevice.PhysicalDevice);

	// Find optimal surface values for our swap chain
	// 1. CHOOSE BEST SURFACE FORMAT
	VkSurfaceFormatKHR surfaceFormat = ChooseBestSurfaceFormat(swapChainDetails.Formats);
	// 2. CHOOSE BEST PRESENTATION MODE
	VkPresentModeKHR presentMode = ChooseBestPresentationMode(swapChainDetails.PresentationMode);
	// 3. CHOOSE SWAP CHAIN IMAGE RESOLUTION
	VkExtent2D extent = ChooseSwapExtent(swapChainDetails.SurfaceCapabilities);

	// How many image are in the swap chain? Get 1 more than minimum to allow triple buffering
	uint32_t imageCount = swapChainDetails.SurfaceCapabilities.minImageCount + 1;

	if (swapChainDetails.SurfaceCapabilities.maxImageCount > 0 && 
		swapChainDetails.SurfaceCapabilities.maxImageCount < imageCount)
		imageCount = swapChainDetails.SurfaceCapabilities.maxImageCount;
	
	// Create information for swap chain
	VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = m_Surface;
	swapChainCreateInfo.imageFormat = surfaceFormat.format;
	swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapChainCreateInfo.presentMode = presentMode;
	swapChainCreateInfo.imageExtent = extent;
	swapChainCreateInfo.minImageCount = imageCount;
	swapChainCreateInfo.imageArrayLayers = 1; // number of layer for each iamge in chain
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapChainCreateInfo.preTransform = swapChainDetails.SurfaceCapabilities.currentTransform;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;	// how to handle blending images with external graphics
	swapChainCreateInfo.clipped = VK_TRUE;


	// Get queue family indices
	QueueFamilyIndices indices = GetQueueFamilies(m_MainDevice.PhysicalDevice);

	// if graphics and presentation families are different, then swapchain
	// must let images be shared between families
	if (indices.GraphicsFamily != indices.PresentationFamily)
	{
		uint32_t queueFamiliyIndices[] = {
			(uint32_t)indices.GraphicsFamily,
			(uint32_t)indices.PresentationFamily,
		};

		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // image share handling
		swapChainCreateInfo.queueFamilyIndexCount = 2;					// number of queues to share images between
		swapChainCreateInfo.pQueueFamilyIndices = queueFamiliyIndices;	// array of queue to share between
	}
	else
	{
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // image share handling
		swapChainCreateInfo.queueFamilyIndexCount = 0;					// number of queues to share images between
		swapChainCreateInfo.pQueueFamilyIndices = nullptr;	// array of queue to share between
	}

	// If old swapchain been destroyed and this one replaces it, then link old one to quicckly hand over responsabilities
	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	// Create Swapchain
	VkResult result = vkCreateSwapchainKHR(m_MainDevice.LogicalDevice, &swapChainCreateInfo, nullptr, &m_Swapchain);

	// Check
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a swapchain!");
	}

	// Store for later reference
	m_SwapchainImageFormat = surfaceFormat.format;
	m_SwapchainExtent = extent;

	// Get swapchain images
	uint32_t swapChainImageCount = 0;
	vkGetSwapchainImagesKHR(m_MainDevice.LogicalDevice, m_Swapchain, &swapChainImageCount, nullptr);
	std::vector<VkImage> images(swapChainImageCount);
	vkGetSwapchainImagesKHR(m_MainDevice.LogicalDevice, m_Swapchain, &swapChainImageCount, images.data());

	for (VkImage image : images)
	{
		// Store image handle
		SwapChainImage swapChainImage = {};
		swapChainImage.Image = image;
		swapChainImage.ImageView = CreateImageView(image, m_SwapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

		// Add to swapchain image list
		m_SwapChainImages.push_back(swapChainImage);
	}
}

void VulkanRenderer::CreateRenderPass()
{
	// Array of our subpasses
	std::array<VkSubpassDescription, 2> subpassDescriptions = {};

	// ATTACHMENTS
	// -----------------------------------------------------------------
	// SUBPASS 1 ATTACHMENTS + REFERENCES (INPUT ATTACHMENTS)
	// -----------------------------------------------------------------
	// Color attachment (input)
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = ChooseSupportedFormat(
		{ VK_FORMAT_R8G8B8A8_UNORM },		// Formats
		VK_IMAGE_TILING_OPTIMAL,			// tiling
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL // featureFlags
	);
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// depth attachment (input)
	// Depth attachment of render pass
	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = ChooseSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


	// Corlor Attachment (input) Reference
	VkAttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.attachment = 1;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Depth attachment (input) Reference
	VkAttachmentReference depthAttachmentReference = {};
	depthAttachmentReference.attachment = 2;
	depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// Set up subpass 1 (input)
	subpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescriptions[0].colorAttachmentCount = 1;
	subpassDescriptions[0].pColorAttachments = &colorAttachmentReference;
	subpassDescriptions[0].pDepthStencilAttachment = &depthAttachmentReference;


	// -----------------------------------------------------------------
	// SUBPASS 2 ATTACHMENTS + REFERENCES 
	// -----------------------------------------------------------------
	// Swapchain color attachment of render pass
	VkAttachmentDescription swapchainColorAttachment = {};
	swapchainColorAttachment.format = m_SwapchainImageFormat;
	swapchainColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;	// number of samples to write for multisampling
	swapchainColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Describes what do with attachment before rendering
	swapchainColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;	// It describes what do with attachment after rendering
	swapchainColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	swapchainColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// Framebuffer data will be store as image, but image can be given
	// different data layout to give optimal use for certain operations
	swapchainColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;		// image data layout before render pass starts
	swapchainColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;	// Image data layout after render pass (to change to)

	// Attachment reference uses an attachment index that refers to index in the attachment list passed to renderPassCreateInfo
	VkAttachmentReference swapchainColorAttachmentReference = {};
	swapchainColorAttachmentReference.attachment = 0;
	swapchainColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// reference to attachments that subpass will take input from
	std::array<VkAttachmentReference, 2> inputReferences;
	inputReferences[0].attachment = 1;
	inputReferences[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	inputReferences[1].attachment = 2;
	inputReferences[1].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// Set up subpass 2 
	subpassDescriptions[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescriptions[1].colorAttachmentCount = 1;
	subpassDescriptions[1].pColorAttachments = &swapchainColorAttachmentReference;
	subpassDescriptions[1].inputAttachmentCount = static_cast<uint32_t>(inputReferences.size());
	subpassDescriptions[1].pInputAttachments = inputReferences.data();



	// -----------------------------------------------------------------
	//SUBPASSES DEPENDENCIES
	// -----------------------------------------------------------------
		
	// Need to determine when layout transition occur using subpass dependencies
	std::array<VkSubpassDependency, 3> subpassDependencies;
	// Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	// Transition must happen after...
	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL; // Subpass index (VK_SUBPASS_EXTERNAL = Special value meaning outside of render pass)
	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // Pipeline stage
	subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT; // Stage access mask (memory access)
	
	// But must happen before
	subpassDependencies[0].dstSubpass = 0;
	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[0].dependencyFlags = 0;

	// Subpass 1 layout (color/depth) to subpass 2 layout (shader read)
	subpassDependencies[1].srcSubpass = 0;
	subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	//
	subpassDependencies[1].dstSubpass = 1;
	subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	subpassDependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	subpassDependencies[1].dependencyFlags = 0;


	// Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	// Transition must happen after...
	subpassDependencies[2].srcSubpass = 0; // Subpass index (VK_SUBPASS_EXTERNAL = Special value meaning outside of render pass)
	subpassDependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Pipeline stage
	subpassDependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;; // Stage access mask (memory access)
	// But must happen before
	subpassDependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependencies[2].dependencyFlags = 0;

	std::array<VkAttachmentDescription, 3> renderPassAttachments = { swapchainColorAttachment,colorAttachment, depthAttachment };

	// Create info for Render pass
	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(renderPassAttachments.size());
	renderPassCreateInfo.pAttachments = renderPassAttachments.data();
	renderPassCreateInfo.subpassCount = static_cast<uint32_t>(subpassDescriptions.size());
	renderPassCreateInfo.pSubpasses = subpassDescriptions.data();
	renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
	renderPassCreateInfo.pDependencies = subpassDependencies.data();

	VkResult result = vkCreateRenderPass(m_MainDevice.LogicalDevice, &renderPassCreateInfo, nullptr, &m_RenderPass);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Render Pass");
	}
}

void VulkanRenderer::CreateDescriptorSetLayout()
{
	// UNIFORM VALUES DESCRIPTOR SET LAYOUT
	// View-Projection binding info
	VkDescriptorSetLayoutBinding vpLayoutBinding = {};
	vpLayoutBinding.binding = 0; // binding point in shader
	vpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // type of descriptor
	vpLayoutBinding.descriptorCount = 1;		// number of descriptor for binding
	vpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;		// Shader stage to bind to
	vpLayoutBinding.pImmutableSamplers = nullptr;			// Fopr texture: can make sampler data unchangeable (imutable) by specifying in layout

	// Model layout binding
	VkDescriptorSetLayoutBinding modelLayoutBinding = {};
	modelLayoutBinding.binding = 1;
	modelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	modelLayoutBinding.descriptorCount = 1;
	modelLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	modelLayoutBinding.pImmutableSamplers = nullptr;

	// List of descriptor set layout bindings
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings = { vpLayoutBinding , modelLayoutBinding };


	// Create descriptor set layout with given bindings
	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());		// number of binding info	
	layoutCreateInfo.pBindings = layoutBindings.data();		// array of binding infos


	// Create descriptor set layour
	VkResult result = vkCreateDescriptorSetLayout(m_MainDevice.LogicalDevice, &layoutCreateInfo, nullptr, &m_DescriptorSetLayout);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Descriptor Set Layout!");
	}

	// CREATE TEXTURE SAMPLER DESCRIPTOR SET LAYOUT
	// Texture binding info
	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 0;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;


	VkDescriptorSetLayoutCreateInfo textureLayoutCreateInfo = {};
	textureLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	textureLayoutCreateInfo.bindingCount = 1;
	textureLayoutCreateInfo.pBindings = &samplerLayoutBinding;

	// Create descriptor set layout
	result = vkCreateDescriptorSetLayout(m_MainDevice.LogicalDevice, &textureLayoutCreateInfo, nullptr, &m_SamplerDescriptorSetLayout);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Sampler Descriptor Set Layout!");
	}


	// CREATE INPUT ATTACHMENT IMAGE DESCRIPTOR SET LAYOUT
	// Color input binding
	VkDescriptorSetLayoutBinding colorInputLayoutBinding = {};
	colorInputLayoutBinding.binding = 0;
	colorInputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	colorInputLayoutBinding.descriptorCount = 1;
	colorInputLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	// Color input binding
	VkDescriptorSetLayoutBinding depthInputLayoutBinding = {};
	depthInputLayoutBinding.binding = 1;
	depthInputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	depthInputLayoutBinding.descriptorCount = 1;
	depthInputLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	// Arrays of input attachment bindings
	std::array< VkDescriptorSetLayoutBinding, 2> inputBindings = { colorInputLayoutBinding , depthInputLayoutBinding };

	// Create descriptor set layout for input attachments
	VkDescriptorSetLayoutCreateInfo inputLayoutCreateInfo = {};
	inputLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	inputLayoutCreateInfo.bindingCount = static_cast<uint32_t>(inputBindings.size());
	inputLayoutCreateInfo.pBindings = inputBindings.data();

	// Create descriptor set layout
	result = vkCreateDescriptorSetLayout(m_MainDevice.LogicalDevice, &inputLayoutCreateInfo, nullptr, &m_InputDescriptorSetLayout);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Input Descriptor Set Layout!");
	}

}

void VulkanRenderer::CreateGraphicsPipeline()
{
	// Read in SPIR-V code 
	auto vertexShaderCode = readSPVFile("src/Shaders/vert.spv");
	auto fragmentShaderCode = readSPVFile("src/Shaders/frag.spv");

	// Build Shader Module to link to graphics pipeline
	VkShaderModule vertexShaderModule = CreateShaderModule(vertexShaderCode);
	VkShaderModule fragmentShaderModule = CreateShaderModule(fragmentShaderCode);

	// shader state creation information
	// vertex stage create information
	VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
	vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderCreateInfo.module = vertexShaderModule;
	vertexShaderCreateInfo.pName = "main";

	// fragment stage create information
	VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
	fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderCreateInfo.module = fragmentShaderModule;
	fragmentShaderCreateInfo.pName = "main";

	// put shader state creation info in to array
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreateInfo , fragmentShaderCreateInfo };

	// How the data for a single vertex (including info such as position, color, texture coordinates, normals, etc) is as in whole
	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0;			// Can bind multiple streams of data, this defines which one
	bindingDescription.stride = sizeof(Vertex);		// size of a single vertex object
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;	// How to move between data after each vertex
																// VK_VERTEX_INPUT_RATE_INSTANCE: move to a vertex for next 

	// How the data for an attribute is defined within a vertex
	std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions;

	// Position attribute
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;			// location in shader wherre data will be read from
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;  // format the data will take (also it helps define size opf data)
	attributeDescriptions[0].offset = offsetof(Vertex, Position); // where this attribute is defined in the data for a single vertex

	// Color attribute 
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;			// location in shader wherre data will be read from
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;  // format the data will take (also it helps define size opf data)
	attributeDescriptions[1].offset = offsetof(Vertex, Color); // where this attribute is defined in the data for a single vertex
	
	// Texture attribute
	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;			// location in shader wherre data will be read from
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;  // format the data will take (also it helps define size opf data)
	attributeDescriptions[2].offset = offsetof(Vertex, TextureCoords); // where this attribute is defined in the data for a single vertex

	// Create PIPELINE
	// -- Vertex Input
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription; // list of binding description (data spacing, stride info)
	vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); // list of vertex attribite descriptions(data format and where to binding)


	// -- Input Assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
	inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;		// Allow overrinding of `strip` topology to start new primitives

	// -- Viewport & Scissor
	// Create a viewport info struct
	VkViewport viewport = {};
	viewport.x = 0.0f; viewport.y = 0.0f;  // start coordinates
	viewport.width = (float)m_SwapchainExtent.width;  // viewport width and height
	viewport.height = (float)m_SwapchainExtent.height;
	viewport.minDepth = 0.0f; viewport.maxDepth = 1.0f;

	// Create a scissor info struct
	VkRect2D scissor = {};
	scissor.offset = { 0,0 };		// offset to use region from
	scissor.extent = m_SwapchainExtent; // extent to describe region to use, starting at offset

	//
	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

	// -- Dynamic States
	// Dynamic states to enable
	std::vector<VkDynamicState> dynamicStateEnables;
	// dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT); // dynamic viewport: can resize in command buffer with vkCmdSetViewport(commandbuffer,0, 1, &viewport)
	// dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR); // dynamic scissor: can resize in command buffer scissor vkCmdSetScissor(commandbuffer, 0, 1, &scissor)

	// dynamic state create info
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
	dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();

	// -- Rasterizer
	// Convert primitive into fragments
	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
	rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerCreateInfo.depthClampEnable = VK_FALSE;		// Change if fragments beyond near/far planes are clipped (default) or clamped to plane (To use we need to set device feature of GPU)
	rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE; //Whether to discard data and skip rasterizer. Never creates framents, only suitable for pipeline without framebuffer output
	rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL; // how to handle filling points between vertices
	rasterizerCreateInfo.lineWidth = 1.0f;		// how thick line should be when drawn (for > 1.0f it is need to enable on gpu)
	rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;	// which face of a triangle to cull
	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // winding to determine which side is front
	rasterizerCreateInfo.depthBiasEnable = VK_FALSE;		// Whether to add depth bias to fragments (good for stopping `shadow acne` in shadow mapping)

	// -- Multisampling
	VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {};
	multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;
	multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;	// number of samples to use per fragment

	// -- Blending

	// Blending attachment state (how blending is handled)
	VkPipelineColorBlendAttachmentState colorState = {};
	colorState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
								|VK_COLOR_COMPONENT_B_BIT  | VK_COLOR_COMPONENT_A_BIT;  // colors to apply blending to
	colorState.blendEnable = VK_TRUE;	
	// Blending uses equation: (srcColorBlendFactor * new color) colorBlendOp(dstColorBlendFactor * old color)
	colorState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorState.colorBlendOp = VK_BLEND_OP_ADD;
	colorState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorState.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo = {};
	colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendingCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendingCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendingCreateInfo.attachmentCount = 1;
	colorBlendingCreateInfo.pAttachments = &colorState;

	// -- Pipeline layout 
	std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = { m_DescriptorSetLayout, m_SamplerDescriptorSetLayout };
	
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	// Create pipeline layout
	VkResult result = vkCreatePipelineLayout(m_MainDevice.LogicalDevice, &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create pipeline layout!");
	}

	// -- Depth Stencil Testing
	// Depth stencil testing
	VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
	depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilCreateInfo.depthTestEnable = VK_TRUE;		// Enable checking depth to determine fragment write
	depthStencilCreateInfo.depthWriteEnable = VK_TRUE;	// Enable writing to depth buffer (to replace old values)
	depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;	// Comparison operation that allows an overwrite (it is in front)
	depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;		// Depth bound test: does the depth value exist between bounds
	depthStencilCreateInfo.stencilTestEnable = VK_FALSE;


	// -- Graphics pipeline creation
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stageCount = 2;			// number of shader stages
	pipelineCreateInfo.pStages = shaderStages;
	pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;		// All the fixed function pipeline states
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
	pipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
	pipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
	pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
	pipelineCreateInfo.layout = m_PipelineLayout;		// pipeline layout pipeline should iuse
	pipelineCreateInfo.renderPass = m_RenderPass;		// Render pass description the pipeline is compatible with
	pipelineCreateInfo.subpass = 0;						// subspass of render pass to use with pipeline


	// Pipeline derivatives: can create multiple pipelines that derive from one another for optimization
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;	// Existing pipeline to derive from...
	pipelineCreateInfo.basePipelineIndex = -1;		// or index of pipeline being created to derive from (in case creating multiple at once)

	// Create graphics pipeline
	result = vkCreateGraphicsPipelines(m_MainDevice.LogicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_GraphicsPipeline);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a graphics pipeline!");
	}

	// Destroy shader modules
	vkDestroyShaderModule(m_MainDevice.LogicalDevice, fragmentShaderModule, nullptr);
	vkDestroyShaderModule(m_MainDevice.LogicalDevice, vertexShaderModule, nullptr);


	// TO DO: CREATE A GENERAL FUNCTION TO CREATE PIPELINE
	// ------------------------------------------------------------
	// CREATE SECOND PASS PIPELINE
	// Second pass shaders
	auto secondVertexShaderCode = readSPVFile("src/Shaders/second_vert.spv");
	auto secondFragmentShaderCode = readSPVFile("src/Shaders/second_frag.spv");

	// Build shaders
	VkShaderModule secondVertexShaderModule = CreateShaderModule(secondVertexShaderCode);
	VkShaderModule secondFragmentShaderModule = CreateShaderModule(secondFragmentShaderCode);

	// Set new shaders
	vertexShaderCreateInfo.module = secondVertexShaderModule;
	fragmentShaderCreateInfo.module = secondFragmentShaderModule;

	VkPipelineShaderStageCreateInfo secondShaderStages[] = { vertexShaderCreateInfo , fragmentShaderCreateInfo };

	// No vertex data for second pass
	vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
	vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
	vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
	vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;

	// Disable depth buffer (Dont want to write to depth buffer)
	depthStencilCreateInfo.depthWriteEnable = VK_FALSE;

	// Create new pipeline layout
	VkPipelineLayoutCreateInfo secondPipelineLayoutCreateInfo = {};
	secondPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	secondPipelineLayoutCreateInfo.setLayoutCount = 1;
	secondPipelineLayoutCreateInfo.pSetLayouts = &m_InputDescriptorSetLayout;
	secondPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	secondPipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	result = vkCreatePipelineLayout(m_MainDevice.LogicalDevice, &secondPipelineLayoutCreateInfo, nullptr, &m_SecondPipelineLayout);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create second pipeline layout!");
	}

	pipelineCreateInfo.pStages = secondShaderStages;	// update second shader stage list
	pipelineCreateInfo.layout = m_SecondPipelineLayout;	// change pipeline for input attachment descriptor sets
	pipelineCreateInfo.subpass = 1;	// use second subpass

	// Create second pipeline
	result = vkCreateGraphicsPipelines(m_MainDevice.LogicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_SecondPipeline);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create second graphiscs pipeline!");
	}

	// Destroy second shader modules
	vkDestroyShaderModule(m_MainDevice.LogicalDevice, secondFragmentShaderModule, nullptr);
	vkDestroyShaderModule(m_MainDevice.LogicalDevice, secondVertexShaderModule, nullptr);
}

void VulkanRenderer::CreateDepthBufferImage()
{
	// 1 depth buffer image to each swapchain
	m_DepthBufferImage.resize(m_SwapChainImages.size());
	m_DepthBufferImageMemory.resize(m_SwapChainImages.size());
	m_DepthBufferImageView.resize(m_SwapChainImages.size());

	// Get supported format for depth buffer
	m_DepthBufferFormat = ChooseSupportedFormat(
		{VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT},
		VK_IMAGE_TILING_OPTIMAL, 
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);

	// Create buffer image amd image view
	for (size_t i = 0; i < m_SwapChainImages.size(); i++)
	{
		// Create depth buffer image
		m_DepthBufferImage[i] = CreateImage(m_SwapchainExtent.width, m_SwapchainExtent.height, m_DepthBufferFormat, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_DepthBufferImageMemory[i]);

		// Create depth buffer image view
		m_DepthBufferImageView[i] = CreateImageView(m_DepthBufferImage[i], m_DepthBufferFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
	}
	
}

void VulkanRenderer::CreateColorBufferImage()
{
	// Resize supported format for color attachment
	// Resize the color buffer image (1 buffer for each swapchain)
	m_ColorBufferImage.resize(m_SwapChainImages.size());
	m_ColorBufferImageMemory.resize(m_SwapChainImages.size());
	m_ColorBufferImageView.resize(m_SwapChainImages.size());

	// Get supported format for color buffer
	VkFormat colorBufferFormat = ChooseSupportedFormat(
		{ VK_FORMAT_R8G8B8A8_UNORM},		// Formats
		VK_IMAGE_TILING_OPTIMAL,			// tiling
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL // featureFlags
	);

	// Create buffer image amd image view
	for (size_t i = 0; i < m_SwapChainImages.size(); i++)
	{
		// Create depth buffer image
		m_ColorBufferImage[i] = CreateImage(m_SwapchainExtent.width, m_SwapchainExtent.height, colorBufferFormat, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_ColorBufferImageMemory[i]);

		// Create depth buffer image view
		m_ColorBufferImageView[i] = CreateImageView(m_ColorBufferImage[i], colorBufferFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

void VulkanRenderer::CreateFramebuffers()
{
	// Resize framebuffer count to equal the swapchain images count
	m_SwapChainFramebuffers.resize(m_SwapChainImages.size());

	// Create a framebuffer for each swapchain image
	for (size_t i = 0; i < m_SwapChainFramebuffers.size(); i++)
	{
		std::array<VkImageView, 3> attachments = {
			m_SwapChainImages[i].ImageView,
			m_ColorBufferImageView[i],
			m_DepthBufferImageView[i]
		};

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = m_RenderPass;			//Render pass layout that framebuffer will be used with
		framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size()); 
		framebufferCreateInfo.pAttachments = attachments.data();  // list of attachments (1:1 with render pass)
		framebufferCreateInfo.width = m_SwapchainExtent.width;
		framebufferCreateInfo.height = m_SwapchainExtent.height;
		framebufferCreateInfo.layers = 1;

		VkResult result = vkCreateFramebuffer(m_MainDevice.LogicalDevice, &framebufferCreateInfo, nullptr, &m_SwapChainFramebuffers[i]);
		
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a framebuffer!");
		}
	}
}

void VulkanRenderer::CreateCommandPool()
{
	// Get indices of queue families from device
	QueueFamilyIndices queueFamilyIndices = GetQueueFamilies(m_MainDevice.PhysicalDevice);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.GraphicsFamily;	//Queue family type that buffers from this command pool will use

	// Create graphics queue family command pool
	VkResult result = vkCreateCommandPool(m_MainDevice.LogicalDevice, &poolInfo, nullptr, &m_GraphicsCommandPool);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create command pool!");
	}
}

void VulkanRenderer::CreateCommandBuffers()
{
	// Resize command buffer count to have one for each framebuffer
	m_CommandBuffers.resize(m_SwapChainFramebuffers.size());

	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = m_GraphicsCommandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // VK_COMMAND_BUFFER_LEVEL_PRIMARY : buffer you submit directly to queue. Cant be called by other buffer.
																	   // VK_COMMAND_BUFFER_LEVEL_SECONDARY : buffer cant be called directly. Can be called from other buffers via vkCmdExecuteCommand when recording commands in primary buffer
	commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());
	
	// Allocate command buffer and place handles in array of buffers
	VkResult result = vkAllocateCommandBuffers(m_MainDevice.LogicalDevice, &commandBufferAllocateInfo, m_CommandBuffers.data());
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate Command Buffers!");
	}
}

void VulkanRenderer::CreateSynchronization()
{
	m_SemaphoresImageAvailable.resize(MAX_FRAME_DRAWS);
	m_SemaphoresRenderFinished.resize(MAX_FRAME_DRAWS);
	m_DrawFences.resize(MAX_FRAME_DRAWS);

	// Semaphore creation information
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	// Fence creation info
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
	{
		if (vkCreateSemaphore(m_MainDevice.LogicalDevice, &semaphoreCreateInfo, nullptr, &m_SemaphoresImageAvailable[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_MainDevice.LogicalDevice, &semaphoreCreateInfo, nullptr, &m_SemaphoresRenderFinished[i]) != VK_SUCCESS || 
			vkCreateFence(m_MainDevice.LogicalDevice, &fenceCreateInfo, nullptr, &m_DrawFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create semaphore and/or fence!");
		}
	}
}

void VulkanRenderer::CreateTextureSampler()
{
	// Sampler create info
	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;						// How to render when image is magnified on screen
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;						// How to render when image is minified on screen
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;	// How to handle texture wrap in U(x) direction
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;	// How to handle texture wrap in V(y) direction
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;	// How to handle texture wrap in U(z) direction
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;	// Border beyond texture (only works for border clamp)
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;				// Normalized texture coordinates in range [0, 1]
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;		// Mipmap interpolation mode
	samplerCreateInfo.mipLodBias = 0.0f;								// level of details bias for mip level
	samplerCreateInfo.minLod = 0.0f;									// minimum level of detail to pick mip level
	samplerCreateInfo.maxLod = 0.0f;									// maximum level of detail to pick mip level
	samplerCreateInfo.anisotropyEnable = VK_TRUE;						// enable anisotropy
	samplerCreateInfo.maxAnisotropy = 16;								// Anisotropy sample level

	// Create sampler
	VkResult result = vkCreateSampler(m_MainDevice.LogicalDevice, &samplerCreateInfo, nullptr, &m_TextureSampler);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create texture sampler!");
	}
}

void VulkanRenderer::CreateUniformBuffers()
{
	// Buffer size of view-projection
	VkDeviceSize bufferSize = sizeof(Camera);

	// Dynamic uniform buffer size (model buffer)
	VkDeviceSize modelBufferSize = m_ModelUniformAlignment * MAX_OBJECTS;

	// One uniform buffer for each image (and by extension, command buffer)
	m_UniformBuffers.resize(m_SwapChainImages.size());
	m_UniformBufferMemory.resize(m_SwapChainImages.size());

	m_UniformDynamicBuffers.resize(m_SwapChainImages.size());
	m_UniformDynamicBufferMemory.resize(m_SwapChainImages.size());

	// Create uniform buffers
	for (size_t i = 0; i < m_SwapChainImages.size(); i++)
	{
		CreateBuffer(m_MainDevice.PhysicalDevice, m_MainDevice.LogicalDevice, bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&m_UniformBuffers[i], &m_UniformBufferMemory[i]);

		CreateBuffer(m_MainDevice.PhysicalDevice, m_MainDevice.LogicalDevice, modelBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&m_UniformDynamicBuffers[i], &m_UniformDynamicBufferMemory[i]);
	}
}

void VulkanRenderer::CreateDescriptorPool()
{
	// CREATE UNIFORM DESCRIPTOR POOL
	// Type of descriptor
	VkDescriptorPoolSize poolSize = {};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = static_cast<uint32_t>(m_UniformBuffers.size());

	// Model pool size 
	VkDescriptorPoolSize dynamicPoolSize = {};
	dynamicPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	dynamicPoolSize.descriptorCount = static_cast<uint32_t>(m_UniformDynamicBuffers.size());

	// list of pool sizes
	std::vector<VkDescriptorPoolSize> descriptorPoolSizeList = { poolSize, dynamicPoolSize };

	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.maxSets = static_cast<uint32_t>(m_SwapChainImages.size());
	poolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizeList.size());		// Amount of pool size
	poolCreateInfo.pPoolSizes = descriptorPoolSizeList.data();

	// Create descriptor pool
	VkResult result = vkCreateDescriptorPool(m_MainDevice.LogicalDevice, &poolCreateInfo, nullptr, &m_DescriptorPool);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Descriptor Pool!");
	}

	// CREATE SAMPLER DESCRIPTOR POOL
	VkDescriptorPoolSize samplerPooSize = {};
	samplerPooSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerPooSize.descriptorCount = MAX_OBJECTS; // Assuming 1 texture per object

	VkDescriptorPoolCreateInfo samplerPoolCreateInfo = {};
	samplerPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	samplerPoolCreateInfo.maxSets = MAX_OBJECTS;
	samplerPoolCreateInfo.poolSizeCount = 1;
	samplerPoolCreateInfo.pPoolSizes = &samplerPooSize;

	result = vkCreateDescriptorPool(m_MainDevice.LogicalDevice, &samplerPoolCreateInfo, nullptr, &m_SamplerDescriptorPool);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Sampler Descriptor Pool!");
	}



	// CREATE INPUT ATTACHMENT DESCRIPTOR POOL
	// Color attachment pool size
	VkDescriptorPoolSize colorInputPoolSize = {};
	colorInputPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	colorInputPoolSize.descriptorCount = static_cast<uint32_t>(m_ColorBufferImageView.size());

	// Depth attachment pool size
	VkDescriptorPoolSize depthInputPoolSize = {};
	depthInputPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	depthInputPoolSize.descriptorCount = static_cast<uint32_t>(m_ColorBufferImageView.size());

	std::array<VkDescriptorPoolSize, 2> inputPoolSizes = { colorInputPoolSize , depthInputPoolSize };

	// Create input attachment pool
	VkDescriptorPoolCreateInfo inputPoolCreateInfo = {};
	inputPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	inputPoolCreateInfo.maxSets = m_SwapChainImages.size();
	inputPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(inputPoolSizes.size());
	inputPoolCreateInfo.pPoolSizes = inputPoolSizes.data();

	result = vkCreateDescriptorPool(m_MainDevice.LogicalDevice, &inputPoolCreateInfo, nullptr, &m_InputDescriptorPool);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Input Descriptor Pool!");
	}
}

void VulkanRenderer::CreateDescriptorSets()
{
	// Resize descriptor sets, so we have one for each buffer
	m_DescriptorSets.resize(m_SwapChainImages.size());

	std::vector<VkDescriptorSetLayout> setLayouts(m_SwapChainImages.size(), m_DescriptorSetLayout);

	VkDescriptorSetAllocateInfo setAllocateInfo = {};
	setAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocateInfo.descriptorPool = m_DescriptorPool;					// Pool to allocate descriptor set from
	setAllocateInfo.descriptorSetCount = static_cast<uint32_t>(m_SwapChainImages.size());	// number of set to allocate
	setAllocateInfo.pSetLayouts = setLayouts.data();	// layout to use to allocate sets

	// Allocate descriptor sets (multiple)
	VkResult result = vkAllocateDescriptorSets(m_MainDevice.LogicalDevice,
		&setAllocateInfo, m_DescriptorSets.data());

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate descriptor sets!");
	}

	// update all of descriptor set buffer bindings
	for (size_t i = 0; i < m_SwapChainImages.size(); i++)
	{

		// UNIFORM BUFFER (VIEW-PROJECTION)
		// buffer info and data offset info
		VkDescriptorBufferInfo vpBufferInfo = {};
		vpBufferInfo.buffer = m_UniformBuffers[i];	// buffer to get data from
		vpBufferInfo.offset = 0;					// Position of start of data
		vpBufferInfo.range = sizeof(Camera);		// Size of data


		// Data about connection between binding and buffer
		VkWriteDescriptorSet vpSetWrite = {};
		vpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vpSetWrite.dstSet = m_DescriptorSets[i];		// Descriptor set to update
		vpSetWrite.dstBinding = 0;						// binding to update (mathces with binding on layout/shader)
		vpSetWrite.dstArrayElement = 0;			// index in array to update
		vpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		vpSetWrite.descriptorCount = 1;
		vpSetWrite.pBufferInfo = &vpBufferInfo;		// info about buffer data to bind


		// UNIFORM DYNAMIC (MODEL)
		VkDescriptorBufferInfo modelBufferInfo = {};
		modelBufferInfo.buffer = m_UniformDynamicBuffers[i];	// buffer to get data from
		modelBufferInfo.offset = 0;					// Position of start of data
		modelBufferInfo.range = m_ModelUniformAlignment;		// Size of data

		VkWriteDescriptorSet modelSetWrite = {};
		modelSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		modelSetWrite.dstSet = m_DescriptorSets[i];
		modelSetWrite.dstBinding = 1;
		modelSetWrite.dstArrayElement = 0;
		modelSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		modelSetWrite.descriptorCount = 1;
		modelSetWrite.pBufferInfo = &modelBufferInfo;


		// list of descriptor set writes
		std::vector<VkWriteDescriptorSet> writeDescriptorSetLists = { vpSetWrite , modelSetWrite };

		// Update the descriptor sets with new buffer/binding info
		vkUpdateDescriptorSets(m_MainDevice.LogicalDevice, 
			static_cast<uint32_t>(writeDescriptorSetLists.size()), writeDescriptorSetLists.data(), 0, nullptr);
	}
}

void VulkanRenderer::CreateInputDescriptorSets()
{
	// Resize array to hold descriptor set for each swap chain image
	m_InputDescriptorSets.resize(m_SwapChainImages.size());

	// Fill array of layout ready for set creation
	std::vector<VkDescriptorSetLayout> setLayouts(m_SwapChainImages.size(), m_InputDescriptorSetLayout);

	// input attachment descriptor set allocation info
	VkDescriptorSetAllocateInfo setAllocateInfo = {};
	setAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocateInfo.descriptorPool = m_InputDescriptorPool;
	setAllocateInfo.descriptorSetCount = static_cast<uint32_t>(m_SwapChainImages.size());
	setAllocateInfo.pSetLayouts = setLayouts.data();

	// allocate descriptor sets
	VkResult result = vkAllocateDescriptorSets(m_MainDevice.LogicalDevice, &setAllocateInfo, m_InputDescriptorSets.data());
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate input attachment descriptor sets!");
	}

	// Update each descriptor set with input attachment
	for (size_t i = 0; i < m_SwapChainImages.size(); i++)
	{
		// color attachment descriptor
		VkDescriptorImageInfo colorAttachmentDescriptor = {};
		colorAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		colorAttachmentDescriptor.imageView = m_ColorBufferImageView[i];
		colorAttachmentDescriptor.sampler = VK_NULL_HANDLE;

		// Color attachment descriptor write
		VkWriteDescriptorSet colorWrite = {};
		colorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		colorWrite.dstSet = m_InputDescriptorSets[i];
		colorWrite.dstBinding = 0;
		colorWrite.dstArrayElement = 0;
		colorWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		colorWrite.descriptorCount = 1;
		colorWrite.pImageInfo = &colorAttachmentDescriptor;

		// depth attachment descriptor
		VkDescriptorImageInfo depthAttachmentDescriptor = {};
		depthAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		depthAttachmentDescriptor.imageView = m_DepthBufferImageView[i];
		depthAttachmentDescriptor.sampler = VK_NULL_HANDLE;

		// Depth attachment descriptor write
		VkWriteDescriptorSet depthWrite = {};
		depthWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		depthWrite.dstSet = m_InputDescriptorSets[i];
		depthWrite.dstBinding = 1;
		depthWrite.dstArrayElement = 0;
		depthWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		depthWrite.descriptorCount = 1;
		depthWrite.pImageInfo = &depthAttachmentDescriptor;

		// List of input descriptor set writes
		std::vector<VkWriteDescriptorSet> setWrites = { colorWrite , depthWrite };

		// Update descriptor sets
		vkUpdateDescriptorSets(m_MainDevice.LogicalDevice, static_cast<uint32_t>(setWrites.size()),
			setWrites.data(), 0, nullptr);
	}
}

void VulkanRenderer::UpdateUniformBuffers(uint32_t imageIndex)
{

	// Copy uniform buffer (view-projection matrix)
	void* data;
	vkMapMemory(m_MainDevice.LogicalDevice, m_UniformBufferMemory[imageIndex], 0,
		sizeof(Camera), 0, &data);
	memcpy(data, &m_Camera, sizeof(Camera));
	vkUnmapMemory(m_MainDevice.LogicalDevice, m_UniformBufferMemory[imageIndex]);

	// copy model data (dynamic uniform buffer)
	size_t Count = 0;
#if 0
	for (size_t i = 0; i < m_ModelList.size(); i++)
	{
		//const glm::mat4& currModel = m_ModelList[i].GetModel();
		// get the address and applied an offset

		for (size_t j = 0; j < m_ModelList[i].GetMeshCount(); j++)
		{
			auto mesh = m_ModelList[i].GetMesh(j);
			
			UniformBufferObjectModel* thisModel = (UniformBufferObjectModel*)((uint64_t)m_ModelTransferSpace + (j * m_ModelUniformAlignment));
			*thisModel = mesh.GetUniformBufferModel();
			Count++;
		} 

	}
#endif

	for (size_t i = 0; i < m_ModelList.size(); i++)
	{
		//const glm::mat4& currModel = m_ModelList[i].GetModel();
		// get the address and applied an offset
		glm::mat4* thisModel = (glm::mat4*)((uint64_t)m_ModelTransferSpace + (i * m_ModelUniformAlignment));
		*thisModel = m_ModelList[i].GetModel();
		Count++;
	}

	
	// Map list of dynamic uniform buffer data (model data)
	vkMapMemory(m_MainDevice.LogicalDevice, m_UniformDynamicBufferMemory[imageIndex], 0,
		m_ModelUniformAlignment * Count, 0, &data);
	memcpy(data, m_ModelTransferSpace, m_ModelUniformAlignment * Count);
	vkUnmapMemory(m_MainDevice.LogicalDevice, m_UniformDynamicBufferMemory[imageIndex]);

}

void VulkanRenderer::RecordCommands(uint32_t currentImageIndex)
{
	// Info about how to begin each command buffer
	VkCommandBufferBeginInfo bufferBeginInfo = { };
	bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	// bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;  // buffer can be resubmitted when it has already been submitted and is awaiting
	
	// Info about how to beging a render pass (only need for graphical applications)
	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = m_RenderPass;		// Render pass to begin
	renderPassBeginInfo.renderArea.offset = { 0, 0 };	// start point of render pass in pixel
	renderPassBeginInfo.renderArea.extent = m_SwapchainExtent;	// size of region to run render pass on (starting at offset)
	
	std::array<VkClearValue, 3> clearValues = {};
	clearValues[0].color = { 0.1f, 0.0f, 0.7f, 1.0f };
	clearValues[1].color = { 0.20f, 0.10f, 0.40f, 1.0f };
	clearValues[2].depthStencil.depth = 1.0f;
	renderPassBeginInfo.pClearValues = clearValues.data();			// list of clear values 
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());

	renderPassBeginInfo.framebuffer = m_SwapChainFramebuffers[currentImageIndex];

	// Start recording commands to command buffer
	VkResult result = vkBeginCommandBuffer(m_CommandBuffers[currentImageIndex], &bufferBeginInfo);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to start recording a Command buffer!");

	// Begin Render Pass
	vkCmdBeginRenderPass(m_CommandBuffers[currentImageIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Start first pipeline (Draw)
	{
		// Bind pipeline to be used in render pass
		vkCmdBindPipeline(m_CommandBuffers[currentImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);
		size_t meshCount = 0;
		// Draw model
		for (auto& model : m_ModelList)
		{
			// Dynamic offset amount
			uint32_t dynamicOffset = static_cast<uint32_t>(m_ModelUniformAlignment * meshCount);
			meshCount++;

			for (size_t k = 0; k < model.GetMeshCount(); k++)
			{
				auto currentMeshPart = model.GetMesh(k);
				VkBuffer vertexBuffers[] = { currentMeshPart.GetVertexBuffer() };	// Buffer to bind
				VkDeviceSize offsets[] = { 0 };		// offsets into buffers being bound
				vkCmdBindVertexBuffers(m_CommandBuffers[currentImageIndex], 0, 1, vertexBuffers, offsets);	// Command to bind vertex buffer before drawing with them

				vkCmdBindIndexBuffer(m_CommandBuffers[currentImageIndex], currentMeshPart.GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

				// dynamic offset

				std::array<VkDescriptorSet, 2> descriptorSetGroup = { m_DescriptorSets[currentImageIndex],
																	m_SamplerDescriptorSets[currentMeshPart.GetTextureID()] };

				// Bind Descriptor Sets (uniform, uniform_dynamic)
				vkCmdBindDescriptorSets(m_CommandBuffers[currentImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS,
					m_PipelineLayout, 0, static_cast<uint32_t>(descriptorSetGroup.size()),
					descriptorSetGroup.data(), 1, &dynamicOffset);

				// Execute pipeline
				vkCmdDrawIndexed(m_CommandBuffers[currentImageIndex], currentMeshPart.GetIndexCount(), 1, 0, 0, 0);
			}
		}

		
	}

	//  Start second subpass
	{
		vkCmdNextSubpass(m_CommandBuffers[currentImageIndex], VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(m_CommandBuffers[currentImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, m_SecondPipeline);
		vkCmdBindDescriptorSets(m_CommandBuffers[currentImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, m_SecondPipelineLayout,
			0, 1, &m_InputDescriptorSets[currentImageIndex], 0, nullptr);

		vkCmdDraw(m_CommandBuffers[currentImageIndex], 3, 1, 0, 0);
	}

	// End Render Pass
	vkCmdEndRenderPass(m_CommandBuffers[currentImageIndex]);

	// Stop recording commands to command buffer
	result = vkEndCommandBuffer(m_CommandBuffers[currentImageIndex]);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to stop recording a Command buffer!");




	// vkBeginCommandBuffer();
}

bool VulkanRenderer::CheckInstanceExtensionSupport(std::vector<const char*>* checkExtensions)
{

	// need to get bumber of extensions to create array of correct size to hold extensions
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	// Create a list of VkExtensionProperties using extensionCount
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	// check if given extensions are in list of availavle extensions
	for (const auto& checkExtension : *checkExtensions)
	{
		bool hasExtension = false;
		for (const auto& extension : extensions)
		{
			if (strcmp(checkExtension, extension.extensionName))
			{
				hasExtension = true;
				break;
			}
		}

		if (!hasExtension)
		{
			return false;
		}
	}

	return true;
}

bool VulkanRenderer::CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
	// Get device extension count
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	if (extensionCount == 0)
		return false;

	// Populate list of extensions
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

	for (const auto& deviceExtension : s_DeviceExtensions)
	{
		bool hasExtension = false;
		for (const auto& extension : extensions)
		{
			if (strcmp(deviceExtension, extension.extensionName) == 0)
			{
				hasExtension = true;
				break;
			}
		}

		if (!hasExtension)
		{
			return false;
		}
	}

	return true;
}

bool VulkanRenderer::CheckDeviceSuitable(VkPhysicalDevice device)
{
#if 0
	// Information about device itself(ID, name, type, vendor, etc)
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	// Information about what the device can do (geo shader, tess shader, wide lines, etc)
	//VkPhysicalDeviceFeatures deviceFeatures;
	//vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
#endif

	// Information about what the device can do (geo shader, tess shader, wide lines, etc)
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	QueueFamilyIndices indices = GetQueueFamilies(device);

	bool hasExtensionsSupported = CheckDeviceExtensionSupport(device);

	bool swapChainValid = false;
	if (hasExtensionsSupported)
	{
		SwapChainDetails  swapChainDetails = GetSwapChainDetails(device);
		swapChainValid = !swapChainDetails.PresentationMode.empty() && !swapChainDetails.Formats.empty();
	}

	//deviceFeatures.samplerAnisotropy
	return indices.IsValid() && hasExtensionsSupported && swapChainValid && deviceFeatures.samplerAnisotropy;
}

QueueFamilyIndices VulkanRenderer::GetQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());

	// Go through each queue family and check if it has at least 1 of the required types of queue
	int index = 0;
	for (const auto& queueFamily : queueFamilyList)
	{
		// First check if queue family has at least 1 queue in that family
		// Queue can be multiple types through bitfield. Need to bitwise AND with VK_QUEUE_*_BIT 
		// to check if the required type
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.GraphicsFamily = index;

		// Check the presentation support
		VkBool32 hasPresentationSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, index, m_Surface, &hasPresentationSupport);
		if (queueFamily.queueCount > 0 && hasPresentationSupport)
			indices.PresentationFamily = index;


		if (indices.IsValid())
			break;

		index++;
	}

	return indices;
}

SwapChainDetails VulkanRenderer::GetSwapChainDetails(VkPhysicalDevice device)
{
	SwapChainDetails swapChainDetails;

	// get surface capabilities for device and m_Surface
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &swapChainDetails.SurfaceCapabilities);

	// Get format
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);
	
	if (formatCount != 0)
	{
		swapChainDetails.Formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, swapChainDetails.Formats.data());
	}
	
	// Get presentation modes
	uint32_t presentationModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentationModeCount, nullptr);
	if (presentationModeCount != 0)
	{
		swapChainDetails.PresentationMode.resize(presentationModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, 
			&presentationModeCount, swapChainDetails.PresentationMode.data());

	}

	return swapChainDetails;
}
// best format is subjective, but ours will be:
// Format : VK_FORMAT_R8G8B8A8_UNORM
// colorSpace : VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
VkSurfaceFormatKHR VulkanRenderer::ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
	if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
	{
		return { VK_FORMAT_R8G8B8A8_UNORM , VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto& format : formats)
	{
		if ((format.format == VK_FORMAT_R8G8B8A8_UNORM
			|| format.format == VK_FORMAT_B8G8R8A8_UNORM)
			&& format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}

	return formats[0];
}
// Mailbox
VkPresentModeKHR VulkanRenderer::ChooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes)
{
	// Look for VK_PRESENT_MODE_MAILBOX_KHR
	for (const auto& presentationMode : presentationModes)
	{
		if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return presentationMode;
		}
	}


	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
{
	if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return surfaceCapabilities.currentExtent;
	}
	else
	{
		int width, height;
		glfwGetFramebufferSize(m_Window, &width, &height);
		
		// Create new extent
		VkExtent2D newExtent = {};
		newExtent.width = (uint32_t)width;
		newExtent.height = (uint32_t)height;

		// Surface also defined max and min and make sure within boundaries by clamping value
		newExtent.width = std::max(surfaceCapabilities.minImageExtent.width,
			std::min(surfaceCapabilities.maxImageExtent.width, newExtent.width));
		newExtent.height = std::max(surfaceCapabilities.minImageExtent.height,
			std::min(surfaceCapabilities.maxImageExtent.height, newExtent.height));


		return newExtent;
	}

}

VkFormat VulkanRenderer::ChooseSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
{
	// Loop through optiong and find compatible one
	for (VkFormat format : formats)
	{
		// Get properties for give format
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(m_MainDevice.PhysicalDevice, format, &properties);

		// Depending on tiling choice, need to check for different bit flag
		if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & featureFlags) == featureFlags)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & featureFlags) == featureFlags)
		{
			return format;
		}

	}

	throw std::runtime_error("Failed to find a matching format");

	return VkFormat();
}

bool VulkanRenderer::CheckValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	// Get the list of the available layers
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	// check if all of the layer in s_validationLayers exist in the availableLayers list
	for (const char* layerName : s_ValidationLayers)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
			return false;
	}

	return true;
}

VkImage VulkanRenderer::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory* imageMemory)
{
	// Create image
	// Image creation info
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;			// image type (1D, 2D, 3D)
	imageCreateInfo.extent.width = width;					// image extents
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;						// depth of image extent (just 1, no 3D aspect)
	imageCreateInfo.mipLevels = 1;							// number of mipmap levels
	imageCreateInfo.arrayLayers = 1;						// number of levels in image array
	imageCreateInfo.format = format;						// format of image (VkFormat)
	imageCreateInfo.tiling = tiling;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;		// layout of image data on creation
	imageCreateInfo.usage = usageFlags;								// Bit flags defining what image will be used
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;				// Number of samples for multi-sampling
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;		// Whether image can be shared between queues

	// Create image (like image header/ The concept of image is created here, but the memory still needs to be allocated)
	VkImage image;
	VkResult result = vkCreateImage(m_MainDevice.LogicalDevice, &imageCreateInfo, nullptr, &image);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create image!");
	}

	// Create memory for image
	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(m_MainDevice.LogicalDevice, image, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = FindMemoryTypeIndex(m_MainDevice.PhysicalDevice, memoryRequirements.memoryTypeBits, propFlags);


	result = vkAllocateMemory(m_MainDevice.LogicalDevice, &memoryAllocateInfo, nullptr, imageMemory);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate memory for image!");
	}

	// Bind image to a memory (connect memory to image)
	vkBindImageMemory(m_MainDevice.LogicalDevice, image, *imageMemory, 0);

	return image;
}

VkImageView VulkanRenderer::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.image = image;	
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;		// type of image (1d, 2d, 3d, cube, etc)
	viewCreateInfo.format = format;
	viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;	// Allow remapping opf rgba components
	viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	// Subresources allow the view to view only a part of an image
	viewCreateInfo.subresourceRange.aspectMask = aspectFlags; // which aspect of iamge to view
	viewCreateInfo.subresourceRange.baseMipLevel = 0;			// Start mipmap level to view from
	viewCreateInfo.subresourceRange.levelCount = 1;				// number of mipmap levels to view
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;			// start array level to view from
	viewCreateInfo.subresourceRange.layerCount = 1;

	// Create image view and return it
	VkImageView imageView;
	VkResult result = vkCreateImageView(m_MainDevice.LogicalDevice, &viewCreateInfo, nullptr, &imageView);

	// Check
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a image view!");
	}

	return imageView;
}

VkShaderModule VulkanRenderer::CreateShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = code.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	VkResult result = vkCreateShaderModule(m_MainDevice.LogicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create shader module!");
	}

	return shaderModule;
}

int VulkanRenderer::CreateTextureImage(const std::string& filepath)
{
	// Load image file
	int width, height;
	VkDeviceSize imageSize;

	stbi_uc* imageData = LoadTextureFile(filepath, &width, &height, &imageSize);

	// Create staging buffer to hold loaded data, ready to copy to device
	VkBuffer imageStagingBuffer;
	VkDeviceMemory imageStagingBufferMemory;
	CreateBuffer(m_MainDevice.PhysicalDevice, m_MainDevice.LogicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&imageStagingBuffer, &imageStagingBufferMemory);


	// Copy image data to staging buffer
	void* data;
	vkMapMemory(m_MainDevice.LogicalDevice, imageStagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, imageData, static_cast<size_t>(imageSize));
	vkUnmapMemory(m_MainDevice.LogicalDevice, imageStagingBufferMemory);

	// Free original image data
	stbi_image_free(imageData);

	// Create image to hold final texture
	VkImage texImage;
	VkDeviceMemory texImageMemory;
	texImage = CreateImage(width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&texImageMemory);


	// COPY DATA TO IMAGE
	// transition image to be dst for copy operation
	TransitionImageLayout(m_MainDevice.LogicalDevice, m_GraphicsQueue, m_GraphicsCommandPool, texImage,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// copy image data
	CopyImageBuffer(m_MainDevice.LogicalDevice, m_GraphicsQueue, m_GraphicsCommandPool, imageStagingBuffer,
		texImage, width, height);
	
	// Transition image to be shader readble for shader usage
	TransitionImageLayout(m_MainDevice.LogicalDevice, m_GraphicsQueue, m_GraphicsCommandPool, texImage,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Add texture data to vector for reference
	m_TextureImages.push_back(texImage);
	m_TextureImageMemory.push_back(texImageMemory);

	// Destroy staging buffer
	vkDestroyBuffer(m_MainDevice.LogicalDevice, imageStagingBuffer, nullptr);
	vkFreeMemory(m_MainDevice.LogicalDevice, imageStagingBufferMemory, nullptr);

	// Return index of new texture image
	return m_TextureImages.size() - 1;
}

int VulkanRenderer::CreateTexture(const std::string& filepath)
{
	// Create texture image and get is location in array
	int textureImageLoc = CreateTextureImage(filepath);

	// Create image view and add to list
	VkImageView imageView = CreateImageView(m_TextureImages[textureImageLoc], VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
	m_TextureImageViews.push_back(imageView);

	// Create descriptor set here
	int descriptorLoc = CreateTextureDescriptor(imageView);

	// Return location of set with texture
	return descriptorLoc;
}

int VulkanRenderer::CreateTextureDescriptor(VkImageView textureImage)
{
	VkDescriptorSet descriptorSet;

	// Descriptor set allocation info
	VkDescriptorSetAllocateInfo setAllocateInfo = {};
	setAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocateInfo.descriptorPool = m_SamplerDescriptorPool;
	setAllocateInfo.descriptorSetCount = 1;
	setAllocateInfo.pSetLayouts = &m_SamplerDescriptorSetLayout;

	// ALlocate descriptor sets
	VkResult result = vkAllocateDescriptorSets(m_MainDevice.LogicalDevice, &setAllocateInfo, &descriptorSet);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate texture descriptor sets!");
	}

	// Texture image info
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;		// image layout when in use
	imageInfo.imageView = textureImage;										// image to bind to set
	imageInfo.sampler = m_TextureSampler;									// sampler to use for set


	// Descriptor write info
	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;


	// Update new descriptor set
	vkUpdateDescriptorSets(m_MainDevice.LogicalDevice, 1, &descriptorWrite, 0, nullptr);

	// Add descriptor set to list
	m_SamplerDescriptorSets.push_back(descriptorSet);

	// return descriptor set location
	return m_SamplerDescriptorSets.size() - 1;

}

void VulkanRenderer::CreateMeshModel(const std::string& filepath)
{
	// Import model 'scene'
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filepath,
		aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);

	if (!scene)
	{
		throw std::runtime_error("Failed to load model: " + filepath);
	}

	// Get the directory of model
	std::string directoryPath;
	const size_t last_slash_idx = filepath.rfind('//');
	if (std::string::npos != last_slash_idx)
	{
		directoryPath = filepath.substr(0, last_slash_idx);
	}


	// Get vector of all material with 1:1 ID placement
	std::vector<std::string> textureNames = MeshModel::LoadMaterials(scene);

	// Conversion from the materials lists IDS to our Descriptor Array IDS
	std::vector<int> materialToTextures(textureNames.size());

	// Loop over textureNames and create textures for them
	for (size_t i = 0; i < textureNames.size(); i++)
	{
		// If material has no texture, set `0` to indicate no texture, texture 0 will be reserved for a default texture
		if (textureNames[i].empty())
		{
			materialToTextures[i] = 0;
		}
		else
		{
			// Otherwise, create texture and set value to index of new texture
			std::string texturePath = directoryPath + "/" + textureNames[i];
			materialToTextures[i] = CreateTexture(texturePath);
		}
	}

	// Load in all our meshes
	std::vector<Mesh> modelMeshes = MeshModel::LoadNode(m_MainDevice.PhysicalDevice, m_MainDevice.LogicalDevice, m_GraphicsQueue,
		m_GraphicsCommandPool, scene->mRootNode, scene, materialToTextures);

	// Create mesh model and add to list
	MeshModel meshModel = MeshModel(modelMeshes);
	m_ModelList.push_back(meshModel);
}

stbi_uc* VulkanRenderer::LoadTextureFile(const std::string& fileName, int* width, int* height, VkDeviceSize* imageSize)
{
	// number of channels image uses
	int channels;

	// load pixel data 
	stbi_uc* image = stbi_load(fileName.c_str(), width, height, &channels, STBI_rgb_alpha);

	if (!image)
	{
		throw std::runtime_error("Failed to load a texture file: " + fileName);
	}

	// Calculaate image size using given and known data
	*imageSize = (*width) * (*height) * 4;

	return image;
}

void VulkanRenderer::GetPhysicalDevice()
{
	// Enumerate physical devices the vkInstance can acess
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);

	// Check the available devices
	if (deviceCount == 0)
	{
		throw std::runtime_error("No physical device! GPU with no Vulkan support!");
	}
	// Get list of physical devices
	std::vector<VkPhysicalDevice> physicalDeviceList(deviceCount);
	vkEnumeratePhysicalDevices(m_Instance, &deviceCount, physicalDeviceList.data());

	// Temp: Just picking the first device
	for (const auto& device : physicalDeviceList)
	{
		if (CheckDeviceSuitable(device))
		{
			m_MainDevice.PhysicalDevice = device;
			break;
		}
	}


	// Get properties of our physical device
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(m_MainDevice.PhysicalDevice, &deviceProperties);

	m_MinUniformBufferOffset = deviceProperties.limits.minUniformBufferOffsetAlignment;


	
}

void VulkanRenderer::AllocateDynamicBufferTransferSpace()
{
	// Calculate alignment of model data
	m_ModelUniformAlignment = (sizeof(UniformBufferObjectModel)+ m_MinUniformBufferOffset - 1) 
							  & ~(m_MinUniformBufferOffset - 1);

	// Create a piece of memory to allocate the dynamic buffer
	m_ModelTransferSpace = (UniformBufferObjectModel*)_aligned_malloc(m_ModelUniformAlignment * MAX_OBJECTS,
		m_ModelUniformAlignment);
}
