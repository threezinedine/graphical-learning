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

struct DeviceContext
{
	GLFWwindow*		 pWindow;
	VkPhysicalDevice physicalDevice;
	VkDevice		 device;
	QueueFamilies	 queueFamilies;

	std::stack<ReleaseNode> releaseStack;
};

static InstanceContext		   instanceContext = {};
static std::stack<ReleaseNode> releaseStack;

static void createInstance();
static void getPhysicalDevices();
static void createSurface(GLFWwindow* pWindow);

using EvaluatePhysicalDeviceFunc = std::function<u32(VkPhysicalDevice)>;
static u32 evaluatePhysicalDevice(VkPhysicalDevice physicalDevice);

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
static void createSwapchain(DeviceContext& deviceContext);

static DeviceContext createDevice(GLFWwindow* pWindow, EvaluatePhysicalDeviceFunc evaluateFunc)
{
	DeviceContext deviceContext = {};
	deviceContext.pWindow		= pWindow;

	choosePhysicalDevice(deviceContext, evaluateFunc);
	findQueueFamilies(deviceContext);
	createDevice(deviceContext);
	createSwapchain(deviceContext);

	return deviceContext;
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

static void createSwapchain(DeviceContext& deviceContext)
{
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