#pragma once

// STD
#include <vector>
#include <fstream>
#include <string>
#include <locale>
#include <codecvt>

// VULKAN
#include <vulkan/vulkan.h>

namespace Utils
{
    static std::vector<char> ReadFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open())
            throw std::runtime_error("Failed to open file: " + filename);

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }

    static VkShaderModule CreateShaderModule(VkDevice device, const std::vector<char>& code)
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
            throw std::runtime_error("Failed to create shader module!");

        return shaderModule;
    }

    static void ThrowFatalError(const char* message)
    {
        std::wstring wmsg = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().from_bytes(message);

        int result = -1;
        while (result != IDOK && result != IDCANCEL)
        {
            result = MessageBox(
                nullptr,
                wmsg.c_str(),
                L"Fatal Error!",
                MB_OK | MB_ICONERROR);
        }

        exit(EXIT_FAILURE);
    }
}