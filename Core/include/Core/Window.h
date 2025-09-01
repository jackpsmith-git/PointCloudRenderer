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

	SDL_Window* GetSDLWindow() const { return m_Window; }

	uint32_t GetWidth() const { return m_Wdith; }
	uint32_t GetHeight() const { return m_Height; }
private:
	uint32_t m_Wdith = 1208;
	uint32_t m_Height = 720;
	SDL_Window* m_Window;
};