#include "common.h"
#include <functional>
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

struct InstanceContext
{
	VkInstance					  instance;
	std::vector<VkPhysicalDevice> physicalDevices;
	VkSurfaceKHR				  surface;
};

struct DeviceContext
{
	GLFWwindow*		 pWindow;
	VkPhysicalDevice physicalDevice;
	VkDevice		 device;

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

static DeviceContext createDevice(GLFWwindow* pWindow, EvaluatePhysicalDeviceFunc evaluateFunc)
{
	DeviceContext deviceContext = {};
	deviceContext.pWindow		= pWindow;

	choosePhysicalDevice(deviceContext, evaluateFunc);

	return deviceContext;
}

static void destroyDevice(DeviceContext& deviceContext)
{
	CLEANUP(deviceContext.releaseStack);
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