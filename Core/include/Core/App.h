#pragma once

// CORE
#include "Window.h"

// STD
#include <memory>

class App
{
public:
	App();
	~App() = default;

	std::shared_ptr<Window> GetWindow() const { return m_window; }

private:
	std::shared_ptr<Window> m_window;
};