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
	}
	catch (const std::runtime_error& e)
	{
		std::cout << "ERROR: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	
	return 0;
}

void VulkanRenderer::CleanUp()
{
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
	deviceCreateInfo.enabledExtensionCount = 0;		// number of enabled logical device extensions
	deviceCreateInfo.ppEnabledExtensionNames = nullptr; // list of enabled logical device extensions

	VkPhysicalDeviceFeatures deviceFeature = {};

	deviceCreateInfo.pEnabledFeatures = &deviceFeature;  // physical device feature will use
	
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

bool VulkanRenderer::CheckDeviceSuitable(VkPhysicalDevice device)
{
#if 0
	// Information about device itself(ID, name, type, vendor, etc)
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	// Information about what the device can do (geo shader, tess shader, wide lines, etc)
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
#endif

	QueueFamilyIndices& indices = GetQueueFamilies(device);
	return indices.IsValid();
}

QueueFamilyIndices& VulkanRenderer::GetQueueFamilies(VkPhysicalDevice device)
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
	
}
