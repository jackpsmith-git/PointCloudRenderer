#include "EntryPoint.h"

// CORE
#include "Renderer.h"

int PCR::EntryPoint(const char* modelPath, uint32_t particleCount, float rotateSpeed)
{
	Renderer renderer(modelPath, particleCount, rotateSpeed);
	renderer.Init();

	while (renderer.IsRunning())
		renderer.Run();

	renderer.Shutdown();

	return renderer.IsRunning() ? 1 : 0;
}
