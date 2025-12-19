#include "common.h"
#include <cstring>
#include <functional>
#include <set>
#include <stack>

using ReleaseFunc = std::function<void(void*)>;
struct ReleaseNode
{
	void*		pData;
	ReleaseFunc deleter;
};

const std::vector<const char*> requiredLayers = {
	"VK_LAYER_KHRONOS_validation",
};

const std::vector<const char*> requiredExtensions = {
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
	VK_KHR_SURFACE_EXTENSION_NAME,
};

const std::vector<const char*> requiredDeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

const std::vector<const char*> requiredDeviceLayers = {};

struct InstanceContext
{
	VkInstance					  instance;
	std::vector<VkPhysicalDevice> physicalDevices;
	VkSurfaceKHR				  surface;
};

struct QueueFamily
{
	bool existed;
	u32	 index;
};

struct QueueFamilies
{
	QueueFamily graphics;
	QueueFamily present;
	QueueFamily compute;
	QueueFamily transfer;
};

struct FlightSyncObjects
{
	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
	VkFence		inFlightFence;
};

struct DeviceContext
{
	GLFWwindow*		 pWindow;
	VkPhysicalDevice physicalDevice;
	VkDevice		 device;
	QueueFamilies	 queueFamilies;

	VkSwapchainKHR			 swapchain;
	VkFormat				 swapchainImageFormat;
	VkExtent2D				 swapchainExtent;
	VkPresentModeKHR		 swapchainPresentMode;
	u32						 swapchainImagesCount;
	std::vector<VkImage>	 swapchainImages;
	std::vector<VkImageView> swapchainImageViews;

	VkCommandPool graphicsCommandPool;
	VkCommandPool presentCommandPool;

	VkCommandBuffer graphicsBuffer;
	VkCommandBuffer presentBuffer;

	std::vector<FlightSyncObjects> flightSyncObjects;

	std::stack<ReleaseNode> releaseStack;
};

static InstanceContext		   instanceContext = {};
static std::stack<ReleaseNode> releaseStack;

static void createInstance();
static void getPhysicalDevices();
static void createSurface(GLFWwindow* pWindow);

using EvaluatePhysicalDeviceFunc = std::function<u32(VkPhysicalDevice)>;

using ChooseFormatFunc		= std::function<VkFormat(const std::vector<VkFormat>&)>;
using ChoosePresentModeFunc = std::function<VkPresentModeKHR(const std::vector<VkPresentModeKHR>&)>;
using ChooseImageCountFunc	= std::function<u32(VkSurfaceCapabilitiesKHR)>;
using ChooseExtentFunc		= std::function<VkExtent2D(const VkSurfaceCapabilitiesKHR&)>;

static u32				evaluatePhysicalDevice(VkPhysicalDevice physicalDevice);
static VkFormat			chooseSwapchainFormat(const std::vector<VkFormat>& availableFormats);
static VkPresentModeKHR chooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
static u32				chooseSwapchainImageCount(VkSurfaceCapabilitiesKHR surfaceCapabilities);
static VkExtent2D		chooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);

static DeviceContext createDevice(GLFWwindow*				 pWindow,
								  EvaluatePhysicalDeviceFunc evaluateFunc = evaluatePhysicalDevice);
static void			 destroyDevice(DeviceContext& deviceContext);

#define CLEANUP(releaseStack)                                                                                          \
	do                                                                                                                 \
	{                                                                                                                  \
		while (!(releaseStack).empty())                                                                                \
		{                                                                                                              \
			ReleaseNode node = (releaseStack).top();                                                                   \
			(releaseStack).pop();                                                                                      \
			node.deleter(node.pData);                                                                                  \
		}                                                                                                              \
	} while (0)

int main()
{
	ASSERT(glfwInit());
	releaseStack.push({nullptr, [](void*) { glfwTerminate(); }});

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	createInstance();
	getPhysicalDevices();

	GLFWwindow* pWindow = glfwCreateWindow(800, 600, "Graphical Learning - Vulkan", nullptr, nullptr);
	releaseStack.push({pWindow, [](void* p) { glfwDestroyWindow((GLFWwindow*)p); }});

	createSurface(pWindow);
	DeviceContext device = createDevice(pWindow, evaluatePhysicalDevice);
	releaseStack.push({&device, [](void* p) { destroyDevice(*(DeviceContext*)p); }});

	while (!glfwWindowShouldClose(pWindow))
	{
		glfwPollEvents();
	}

	CLEANUP(releaseStack);

	return 0;
}

static void destroyInstance(void* pData);
static void createInstance()
{
	VkApplicationInfo appInfo  = {};
	appInfo.sType			   = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName   = "Graphical Learning - Vulkan";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName		   = "No Engine";
	appInfo.engineVersion	   = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion		   = VK_API_VERSION_1_1;

	std::vector<const char*> finalExtensions	= requiredExtensions;
	u32						 glfwExtensionCount = 0;
	const char**			 glfwExtensions		= glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	for (u32 i = 0; i < glfwExtensionCount; ++i)
	{
		finalExtensions.push_back(glfwExtensions[i]);
	}

	VkInstanceCreateInfo instanceInfo	 = {};
	instanceInfo.sType					 = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pApplicationInfo		 = &appInfo;
	instanceInfo.enabledLayerCount		 = u32(requiredLayers.size());
	instanceInfo.ppEnabledLayerNames	 = requiredLayers.data();
	instanceInfo.enabledExtensionCount	 = u32(finalExtensions.size());
	instanceInfo.ppEnabledExtensionNames = finalExtensions.data();

	VK_ASSERT(vkCreateInstance(&instanceInfo, nullptr, &instanceContext.instance));
	releaseStack.push({&instanceContext.instance, destroyInstance});
}

static void destroyInstance(void* pData)
{
	vkDestroyInstance(*(VkInstance*)pData, nullptr);
}

static void getPhysicalDevices()
{
	u32 physicalDevicesCount = 0;
	vkEnumeratePhysicalDevices(instanceContext.instance, &physicalDevicesCount, nullptr);

	instanceContext.physicalDevices.resize(physicalDevicesCount);
	vkEnumeratePhysicalDevices(instanceContext.instance, &physicalDevicesCount, instanceContext.physicalDevices.data());
}

static void createSurface(GLFWwindow* pWindow)
{
	VkSurfaceKHR surface;
	VK_ASSERT(glfwCreateWindowSurface(instanceContext.instance, pWindow, nullptr, &instanceContext.surface));
	releaseStack.push({&instanceContext.surface, [](void* p) {
						   VkSurfaceKHR surface = *(VkSurfaceKHR*)p;
						   vkDestroySurfaceKHR(instanceContext.instance, surface, nullptr);
					   }});
}

static void choosePhysicalDevice(DeviceContext& deviceContext, EvaluatePhysicalDeviceFunc evaluateFunc);
static void findQueueFamilies(DeviceContext& deviceContext);
static void createDevice(DeviceContext& deviceContext);
static void createSwapchain(DeviceContext&		  deviceContext,
							ChooseFormatFunc	  chooseFormat		= chooseSwapchainFormat,
							ChoosePresentModeFunc choosePresentMode = chooseSwapchainPresentMode,
							ChooseImageCountFunc  chooseImageCount	= chooseSwapchainImageCount,
							ChooseExtentFunc	  chooseExtent		= chooseSwapchainExtent);
static void aquireSwapchainImages(DeviceContext& deviceContext);
static void createSwapchainImagesViews(DeviceContext& deviceContext);
static void createCommandPools(DeviceContext& deviceContext);
static void createCommandBuffers(DeviceContext& deviceContext);
static void createFlightSyncObjects(DeviceContext& deviceContext);

static DeviceContext createDevice(GLFWwindow* pWindow, EvaluatePhysicalDeviceFunc evaluateFunc)
{
	DeviceContext deviceContext = {};
	deviceContext.pWindow		= pWindow;

	choosePhysicalDevice(deviceContext, evaluateFunc);
	findQueueFamilies(deviceContext);
	createDevice(deviceContext);
	createSwapchain(deviceContext);
	aquireSwapchainImages(deviceContext);
	createSwapchainImagesViews(deviceContext);
	createCommandPools(deviceContext);
	createCommandBuffers(deviceContext);
	createFlightSyncObjects(deviceContext);

	return deviceContext;
}

static void createSemaphore(VkDevice device, VkSemaphore* pSemaphore);
static void createFence(VkDevice device, VkFence* pFence);
static void createFlightSyncObjects(DeviceContext& deviceContext)
{
	deviceContext.flightSyncObjects.resize(deviceContext.swapchainImagesCount);
	memset(deviceContext.flightSyncObjects.data(), 0, sizeof(FlightSyncObjects) * deviceContext.swapchainImagesCount);

	for (u32 flightIndex = 0u; flightIndex < deviceContext.swapchainImagesCount; ++flightIndex)
	{
		FlightSyncObjects& syncObjects = deviceContext.flightSyncObjects[flightIndex];

		createSemaphore(deviceContext.device, &syncObjects.imageAvailableSemaphore);
		createSemaphore(deviceContext.device, &syncObjects.renderFinishedSemaphore);
		createFence(deviceContext.device, &syncObjects.inFlightFence);
	}

	deviceContext.releaseStack.push(
		{&deviceContext, [](void* p) {
			 DeviceContext& deviceContext = *(DeviceContext*)p;
			 for (FlightSyncObjects& syncObjects : deviceContext.flightSyncObjects)
			 {
				 vkDestroySemaphore(deviceContext.device, syncObjects.imageAvailableSemaphore, nullptr);
				 vkDestroySemaphore(deviceContext.device, syncObjects.renderFinishedSemaphore, nullptr);
				 vkDestroyFence(deviceContext.device, syncObjects.inFlightFence, nullptr);
			 }
		 }});
}

static void createSemaphore(VkDevice device, VkSemaphore* pSemaphore)
{
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType					= VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VK_ASSERT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, pSemaphore));
}

static void createFence(VkDevice device, VkFence* pFence)
{
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType				= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags				= VK_FENCE_CREATE_SIGNALED_BIT;
	VK_ASSERT(vkCreateFence(device, &fenceInfo, nullptr, pFence));
}

static void createCommandBuffers(DeviceContext& deviceContext)
{
	VkCommandBufferAllocateInfo graphicsAllocInfo = {};
	graphicsAllocInfo.sType						  = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	graphicsAllocInfo.commandPool				  = deviceContext.graphicsCommandPool;
	graphicsAllocInfo.level						  = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	graphicsAllocInfo.commandBufferCount		  = 1;
	VK_ASSERT(vkAllocateCommandBuffers(deviceContext.device, &graphicsAllocInfo, &deviceContext.graphicsBuffer));
	deviceContext.releaseStack.push({&deviceContext, [](void* p) {
										 DeviceContext& deviceContext = *(DeviceContext*)p;
										 vkFreeCommandBuffers(deviceContext.device,
															  deviceContext.graphicsCommandPool,
															  1,
															  &deviceContext.graphicsBuffer);
									 }});

	VkCommandBufferAllocateInfo presentAllocInfo = {};
	presentAllocInfo.sType						 = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	presentAllocInfo.commandPool				 = deviceContext.presentCommandPool;
	presentAllocInfo.level						 = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	presentAllocInfo.commandBufferCount			 = 1;
	VK_ASSERT(vkAllocateCommandBuffers(deviceContext.device, &presentAllocInfo, &deviceContext.presentBuffer));

	deviceContext.releaseStack.push({&deviceContext, [](void* p) {
										 DeviceContext& deviceContext = *(DeviceContext*)p;
										 vkFreeCommandBuffers(deviceContext.device,
															  deviceContext.presentCommandPool,
															  1,
															  &deviceContext.presentBuffer);
									 }});
}

static void createCommandPools(DeviceContext& deviceContext)
{
	VkCommandPoolCreateInfo graphicsPoolInfo = {};
	graphicsPoolInfo.sType					 = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	graphicsPoolInfo.queueFamilyIndex		 = deviceContext.queueFamilies.graphics.index;
	graphicsPoolInfo.flags					 = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VK_ASSERT(
		vkCreateCommandPool(deviceContext.device, &graphicsPoolInfo, nullptr, &deviceContext.graphicsCommandPool));
	deviceContext.releaseStack.push({&deviceContext, [](void* p) {
										 DeviceContext& deviceContext = *(DeviceContext*)p;
										 vkDestroyCommandPool(
											 deviceContext.device, deviceContext.graphicsCommandPool, nullptr);
									 }});

	if (deviceContext.queueFamilies.present.index != deviceContext.queueFamilies.graphics.index)
	{
		VkCommandPoolCreateInfo presentPoolInfo = {};
		presentPoolInfo.sType					= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		presentPoolInfo.queueFamilyIndex		= deviceContext.queueFamilies.present.index;
		presentPoolInfo.flags					= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VK_ASSERT(
			vkCreateCommandPool(deviceContext.device, &presentPoolInfo, nullptr, &deviceContext.presentCommandPool));
		deviceContext.releaseStack.push({&deviceContext, [](void* p) {
											 DeviceContext& deviceContext = *(DeviceContext*)p;
											 vkDestroyCommandPool(
												 deviceContext.device, deviceContext.presentCommandPool, nullptr);
										 }});
	}
	else
	{
		deviceContext.presentCommandPool = deviceContext.graphicsCommandPool;
	}
}

static void createSwapchainImagesViews(DeviceContext& deviceContext)
{
	deviceContext.swapchainImageViews.resize(deviceContext.swapchainImagesCount);

	for (u32 imageIndex = 0u; imageIndex < deviceContext.swapchainImagesCount; ++imageIndex)
	{
		VkImage&	 swapchainImage		= deviceContext.swapchainImages[imageIndex];
		VkImageView& swapchainImageView = deviceContext.swapchainImageViews[imageIndex];

		VkImageViewCreateInfo imageViewInfo			  = {};
		imageViewInfo.sType							  = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewInfo.image							  = swapchainImage;
		imageViewInfo.viewType						  = VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.format						  = deviceContext.swapchainImageFormat;
		imageViewInfo.components.r					  = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewInfo.components.g					  = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewInfo.components.b					  = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewInfo.components.a					  = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewInfo.subresourceRange.aspectMask	  = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewInfo.subresourceRange.baseMipLevel	  = 0;
		imageViewInfo.subresourceRange.levelCount	  = 1;
		imageViewInfo.subresourceRange.baseArrayLayer = 0;
		imageViewInfo.subresourceRange.layerCount	  = 1;

		VK_ASSERT(vkCreateImageView(deviceContext.device, &imageViewInfo, nullptr, &swapchainImageView));
	}

	deviceContext.releaseStack.push({&deviceContext, [](void* p) {
										 DeviceContext& deviceContext = *(DeviceContext*)p;
										 for (VkImageView imageView : deviceContext.swapchainImageViews)
										 {
											 vkDestroyImageView(deviceContext.device, imageView, nullptr);
										 }
									 }});
}

static void aquireSwapchainImages(DeviceContext& deviceContext)
{
	deviceContext.swapchainImages.resize(deviceContext.swapchainImagesCount);

	for (u32 imageIndex = 0u; imageIndex < deviceContext.swapchainImagesCount; ++imageIndex)
	{
		VK_ASSERT(vkGetSwapchainImagesKHR(deviceContext.device,
										  deviceContext.swapchain,
										  &deviceContext.swapchainImagesCount,
										  deviceContext.swapchainImages.data()));
	}
}

static void destroyDevice(DeviceContext& deviceContext)
{
	CLEANUP(deviceContext.releaseStack);
}

static void createDevice(DeviceContext& deviceContext)
{
	ASSERT(deviceContext.physicalDevice != VK_NULL_HANDLE);

	std::set<u32> uniqueQueueFamilies;
	uniqueQueueFamilies.insert(deviceContext.queueFamilies.graphics.index);
	uniqueQueueFamilies.insert(deviceContext.queueFamilies.present.index);
	if (deviceContext.queueFamilies.compute.existed)
	{
		uniqueQueueFamilies.insert(deviceContext.queueFamilies.compute.index);
	}
	if (deviceContext.queueFamilies.transfer.existed)
	{
		uniqueQueueFamilies.insert(deviceContext.queueFamilies.transfer.index);
	}

	u32									 uniqueFamiliesCount = u32(uniqueQueueFamilies.size());
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(uniqueFamiliesCount);
	memset(queueCreateInfos.data(), 0, sizeof(VkDeviceQueueCreateInfo) * uniqueFamiliesCount);

	f32 graphicsPriority = 1.0f;
	f32 computePriority	 = 0.5f;

	for (u32 uniqueFamilyIndex = 0u; uniqueFamilyIndex < uniqueFamiliesCount; ++uniqueFamilyIndex)
	{
		u32	 index		   = *(std::next(uniqueQueueFamilies.begin(), uniqueFamilyIndex));
		f32* queuePriority = index == deviceContext.queueFamilies.graphics.index ? &graphicsPriority : &computePriority;

		VkDeviceQueueCreateInfo& queueCreateInfo = queueCreateInfos[uniqueFamilyIndex];
		queueCreateInfo.sType					 = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex		 = index;
		queueCreateInfo.queueCount				 = 1;
		queueCreateInfo.pQueuePriorities		 = queuePriority;
	}

	VkDeviceCreateInfo deviceInfo	   = {};
	deviceInfo.sType				   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.queueCreateInfoCount	   = uniqueFamiliesCount;
	deviceInfo.pQueueCreateInfos	   = queueCreateInfos.data();
	deviceInfo.enabledExtensionCount   = u32(requiredDeviceExtensions.size());
	deviceInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();
	deviceInfo.enabledLayerCount	   = u32(requiredDeviceLayers.size());
	deviceInfo.ppEnabledLayerNames	   = requiredDeviceLayers.data();

	VK_ASSERT(vkCreateDevice(deviceContext.physicalDevice, &deviceInfo, nullptr, &deviceContext.device));
	deviceContext.releaseStack.push({&deviceContext.device, [](void* p) {
										 VkDevice device = *(VkDevice*)p;
										 vkDestroyDevice(device, nullptr);
									 }});

	printf("Logical device created.\n");
}

static void findQueueFamilies(DeviceContext& deviceContext)
{
	ASSERT(deviceContext.physicalDevice != VK_NULL_HANDLE);

	u32 queueFamiliesCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(deviceContext.physicalDevice, &queueFamiliesCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamiliesCount);
	vkGetPhysicalDeviceQueueFamilyProperties(deviceContext.physicalDevice, &queueFamiliesCount, queueFamilies.data());

	VkSurfaceKHR surface = instanceContext.surface;
	ASSERT(surface != VK_NULL_HANDLE);

	bool foundGraphicsAndPresent = false;
	bool foundCompute			 = false;
	bool foundTransfer			 = false;

	for (u32 familyIndex = 0u; familyIndex < queueFamiliesCount; ++familyIndex)
	{
		const VkQueueFamilyProperties& properties	   = queueFamilies[familyIndex];
		VkBool32					   supportsPresent = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(deviceContext.physicalDevice, familyIndex, surface, &supportsPresent);

		if (properties.queueFlags & VK_QUEUE_GRAPHICS_BIT && !foundGraphicsAndPresent)
		{
			deviceContext.queueFamilies.graphics.existed = true;
			deviceContext.queueFamilies.graphics.index	 = familyIndex;
		}

		if (supportsPresent && !foundGraphicsAndPresent)
		{
			deviceContext.queueFamilies.present.existed = true;
			deviceContext.queueFamilies.present.index	= familyIndex;

			if (deviceContext.queueFamilies.graphics.index == familyIndex)
			{
				foundGraphicsAndPresent = true;
			}
		}
	}

	for (u32 familyIndex = 0u; familyIndex < queueFamiliesCount; ++familyIndex)
	{
		const VkQueueFamilyProperties& properties = queueFamilies[familyIndex];

		if (properties.queueFlags & VK_QUEUE_COMPUTE_BIT && !foundCompute)
		{
			deviceContext.queueFamilies.compute.existed = true;
			deviceContext.queueFamilies.compute.index	= familyIndex;

			if (deviceContext.queueFamilies.graphics.index != familyIndex)
			{
				foundCompute = true;
			}
		}
	}

	for (u32 familyIndex = 0u; familyIndex < queueFamiliesCount; ++familyIndex)
	{
		const VkQueueFamilyProperties& properties = queueFamilies[familyIndex];
		if (properties.queueFlags & VK_QUEUE_TRANSFER_BIT)
		{
			deviceContext.queueFamilies.transfer.existed = true;
			deviceContext.queueFamilies.transfer.index	 = familyIndex;

			if (deviceContext.queueFamilies.graphics.index != familyIndex &&
				deviceContext.queueFamilies.compute.index != familyIndex)
			{
				foundTransfer = true;
			}
		}
	}

	printf("Selected Queue Families:\n");
	printf(" Graphics: %d\n", deviceContext.queueFamilies.graphics.index);
	printf(" Present:  %d\n", deviceContext.queueFamilies.present.index);
	printf(" Compute:  %d\n", deviceContext.queueFamilies.compute.index);
	printf(" Transfer: %d\n", deviceContext.queueFamilies.transfer.index);
}

static void choosePhysicalDevice(DeviceContext& deviceContext, EvaluatePhysicalDeviceFunc evaluateFunc)
{
	u32 bestScore = 0;
	i32 bestIndex = -1; // must be >= 0 after selection

	u32 physicalDevicesCount = u32(instanceContext.physicalDevices.size());

	for (u32 physicalDeviceIndex = 0; physicalDeviceIndex < physicalDevicesCount; ++physicalDeviceIndex)
	{
		VkPhysicalDevice physicalDevice = instanceContext.physicalDevices[physicalDeviceIndex];
		u32				 score			= evaluateFunc(physicalDevice);

		if (score > bestScore)
		{
			bestScore = score;
			bestIndex = physicalDeviceIndex;
		}
	}

	ASSERT(bestIndex >= 0);
	deviceContext.physicalDevice = instanceContext.physicalDevices[bestIndex];
}

static void createSwapchain(DeviceContext&		  deviceContext,
							ChooseFormatFunc	  chooseFormat,
							ChoosePresentModeFunc choosePresentMode,
							ChooseImageCountFunc  chooseImageCount,
							ChooseExtentFunc	  chooseExtent)
{
	{
		u32 presentModeCount = 0;
		VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(
			deviceContext.physicalDevice, instanceContext.surface, &presentModeCount, nullptr));

		std::vector<VkPresentModeKHR> availablePresentModes(presentModeCount);
		VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(
			deviceContext.physicalDevice, instanceContext.surface, &presentModeCount, availablePresentModes.data()));

		deviceContext.swapchainPresentMode = choosePresentMode(availablePresentModes);
	}

	{
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		VK_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
			deviceContext.physicalDevice, instanceContext.surface, &surfaceCapabilities));

		deviceContext.swapchainImagesCount = chooseImageCount(surfaceCapabilities);
		deviceContext.swapchainExtent	   = chooseExtent(surfaceCapabilities);
	}

	{
		u32 formatCount = 0;
		VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(
			deviceContext.physicalDevice, instanceContext.surface, &formatCount, nullptr));

		std::vector<VkSurfaceFormatKHR> availableFormats(formatCount);
		VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(
			deviceContext.physicalDevice, instanceContext.surface, &formatCount, availableFormats.data()));

		std::vector<VkFormat> formats(formatCount);
		for (u32 i = 0; i < formatCount; ++i)
		{
			formats[i] = availableFormats[i].format;
		}

		deviceContext.swapchainImageFormat = chooseFormat(formats);
	}

	VkSwapchainCreateInfoKHR swapchainInfo = {};
	swapchainInfo.sType					   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainInfo.surface				   = instanceContext.surface;
	swapchainInfo.minImageCount			   = deviceContext.swapchainImagesCount;
	swapchainInfo.imageFormat			   = deviceContext.swapchainImageFormat;
	swapchainInfo.imageColorSpace		   = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	swapchainInfo.imageExtent			   = deviceContext.swapchainExtent;
	swapchainInfo.imageArrayLayers		   = 1;
	swapchainInfo.imageSharingMode		   = VK_SHARING_MODE_EXCLUSIVE;
	swapchainInfo.queueFamilyIndexCount	   = 0;
	swapchainInfo.imageUsage			   = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainInfo.preTransform			   = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapchainInfo.compositeAlpha		   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainInfo.clipped				   = VK_TRUE;
	swapchainInfo.oldSwapchain			   = VK_NULL_HANDLE;

	VK_ASSERT(vkCreateSwapchainKHR(deviceContext.device, &swapchainInfo, nullptr, &deviceContext.swapchain));
	deviceContext.releaseStack.push({&deviceContext, [](void* p) {
										 DeviceContext& deviceContext = *(DeviceContext*)p;
										 vkDestroySwapchainKHR(deviceContext.device, deviceContext.swapchain, nullptr);
									 }});

	printf("Swapchain created.\n");
}

static u32 evaluatePhysicalDevice(VkPhysicalDevice physicalDevice)
{
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

	u32 score = 0;

	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
	{
		score += 1000;
	}
	else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
	{
		score += 500;
	}
	else
	{
		return 0;
	}

	score += deviceProperties.limits.maxImageDimension2D;

	return score;
}

static VkFormat chooseSwapchainFormat(const std::vector<VkFormat>& availableFormats)
{
	for (const VkFormat& format : availableFormats)
	{
		if (format == VK_FORMAT_B8G8R8A8_SRGB)
		{
			return format;
		}
	}

	return availableFormats[0]; // fallback
}

static VkPresentModeKHR chooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const VkPresentModeKHR& presentMode : availablePresentModes)
	{
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return presentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR; // guaranteed to be available
}

static u32 chooseSwapchainImageCount(VkSurfaceCapabilitiesKHR surfaceCapabilities)
{
	u32 imageCount = surfaceCapabilities.minImageCount + 1;

	if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount)
	{
		imageCount = surfaceCapabilities.maxImageCount;
	}

	return imageCount;
}

static VkExtent2D chooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
{
	ASSERT(surfaceCapabilities.currentExtent.width != UINT32_MAX);

	if (surfaceCapabilities.currentExtent.width != UINT32_MAX)
	{
		return surfaceCapabilities.currentExtent;
	}

	ASSERT(false); // For simplicity, we ignore the case where we have to choose the extent ourselves.
	return VkExtent2D{800, 600};
}