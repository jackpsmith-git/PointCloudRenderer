#pragma once
#include <cstdint>

namespace PCR
{
	int EntryPoint(const char* modelPath, uint32_t particleCount = 100000, float rotateSpeed = 10.0);
}