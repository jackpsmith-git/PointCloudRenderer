project "Sample"
   kind "WindowedApp"
   language "C++"
   cppdialect "C++20"
   targetdir "bin/%{cfg.buildcfg}"
   staticruntime "off"

   files 
   { 
      "include/**.h", 
      "src/**.cpp" 
    }

   includedirs
   {
      "src",
      "include",

	  -- Include Core
	  "../Core/include",

      "../Core/external/vulkan/Include"
   }

    libdirs 
    {
        "../Core/external/vulkan/Lib"
    }

   links
   {
      "Core",
      "SDL2",
      "SDL2main",
      "vulkan-1"
   }

   targetdir ("../bin/" .. OutputDir .. "/%{prj.name}")
   objdir ("../bin/int/" .. OutputDir .. "/%{prj.name}")

   filter "system:windows"
       systemversion "latest"
       defines { "WINDOWS" }

   filter "configurations:Debug"
       defines { "DEBUG" }
       runtime "Debug"
       symbols "On"

   filter "configurations:Release"
       defines { "RELEASE" }
       runtime "Release"
       optimize "On"
       symbols "On"

   filter "configurations:Dist"
       defines { "DIST" }
       runtime "Release"
       optimize "On"
       symbols "Off"