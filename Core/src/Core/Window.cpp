#include "Window.h"

// PCR
#include "Utils.h"

// SDL
#include <SDL2/SDL_vulkan.h>

Window::Window(const std::string& title, int width, int height)
	: m_width(width), m_height(height)
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0) 
	{
		Utils::ThrowFatalError("Failed to initialize SDL");
	}

	m_window = SDL_CreateWindow(
		title.c_str(),
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		width,
		height,
		SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
	);

	if (!m_window)
	{
		Utils::ThrowFatalError("Failed to create SDL window");
	}
}

Window::~Window()
{
	if (m_window)
	{
		SDL_DestroyWindow(m_window);
	}

	SDL_Quit();
}

bool Window::PollEvents()
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) 
	{
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