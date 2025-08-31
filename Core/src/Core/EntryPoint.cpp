#include "EntryPoint.h"

#include "Window.h"
#include "Instance.h"

#include <vulkan/vulkan.h>

namespace Core {

	int EntryPoint()
	{
		Window window("Point-CLoud Renderer", 1280, 720);
		Instance instance(window.GetSDLWindow());

		VkSurfaceKHR surface = instance.CreateVulkanSurface(window.GetSDLWindow());

		while (window.PollEvents())
		{
			// Main Loop
		}

		return 0;
	}
}