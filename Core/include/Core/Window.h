#pragma once

// STD
#include <string>

// SDL
#include <SDL2/SDL.h>

class Window
{
public:
	Window(const std::string& title, int width, int height);
	~Window();

	bool PollEvents();

	SDL_Window* Get() const { return m_window; }

	uint32_t GetWidth() const { return m_width; }
	uint32_t GetHeight() const { return m_height; }
public:
	bool FramebufferResized = false;

	float CameraDistance = 3.0f;
	float ZoomSpeed = 0.1f;
private:
	uint32_t m_width = 1208;
	uint32_t m_height = 720;
	SDL_Window* m_window;
};