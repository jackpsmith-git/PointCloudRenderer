
// ---------------------------------------------------------------------------
// ------ Point-Cloud Renderer Sample Application ----------------------------
// ---------------------------------------------------------------------------


// PCR
#include "Core/App.h"
#include "Core/Renderer.h"

int main(int argc, char* argv[])
{
	App* app = new App();

	auto window = app->GetWindow();

	Renderer renderer(window);
	renderer.LoadMesh("objects/Suzanne.obj");
	renderer.SetParticleCount(10000);			// Optional - Defaults to 10,000
	renderer.SetRotationSpeed(10.0f);			// Optional - Defaults to 10 degrees per second
	renderer.Init();

	bool running = true;
	while (running)
	{
		running = window->PollEvents();
		renderer.Run();
	}

	renderer.Shutdown();

	delete app;
	return 0;
}