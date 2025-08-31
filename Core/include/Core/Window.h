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
private:
	SDL_Window* m_Window;
};