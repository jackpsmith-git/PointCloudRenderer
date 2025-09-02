#include "Window.h"

// STD
#include <stdexcept>

// SDL
#include <SDL2/SDL_vulkan.h>

Window::Window(const std::string& title, int width, int height)
	: m_Wdith(width), m_Height(height)
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0) 
	{
		throw std::runtime_error("Failed to initialize SDL");
	}

	m_Window = SDL_CreateWindow(
		title.c_str(),
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		width,
		height,
		SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
	);

	if (!m_Window)
	{
		throw std::runtime_error("Failed to create SDL window");
	}
}

Window::~Window()
{
	if (m_Window)
	{
		SDL_DestroyWindow(m_Window);
	}

	SDL_Quit();
}

bool Window::PollEvents()
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) 
		{
			return false;
		}

		if (event.type == SDL_WINDOWEVENT &&
			event.window.event == SDL_WINDOWEVENT_RESIZED)
		{
			FramebufferResized = true;
		}

		if (event.type == SDL_MOUSEWHEEL)
		{
			CameraDistance -= event.wheel.y * ZoomSpeed;
			if (CameraDistance < 0.1f) CameraDistance = 0.1f;
			if (CameraDistance > 20.0f) CameraDistance = 20.0f;
		}
	}
	return true;
}