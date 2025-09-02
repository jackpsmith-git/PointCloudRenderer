#include "Core/EntryPoint.h"

int WinMain()
{
	const char* modelPath = "objects/Suzanne.obj";
	uint32_t particleCount = 100000;
	float rotateSpeed = 10.0f;

	bool result = PCR::EntryPoint(modelPath, particleCount, rotateSpeed);
	return result;
}