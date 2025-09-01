#include "EntryPoint.h"

// CORE
#include "Renderer.h"

int Core::EntryPoint()
{
	Renderer renderer;
	renderer.Init();

	while (renderer.IsRunning())
		renderer.Run();

	renderer.Shutdown();

	return 0;
}
