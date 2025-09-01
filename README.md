# Point-Cloud Renderer
3D, cross-platform, point-cloud, Vulkan renderer.

## Dependencies
All dependencies are included in the repo EXCEPT for the necessary Vulkan SDK headers and libs. Download v1.4.321.1 from https://www.lunarg.com/vulkan-sdk/. Place the necessary dependencies in the source code with the following structure:

```
- Point-Cloud Renderer
-- Core
--- external
---- vulkan
----- Include
------ * (Include folder contents from vulkan sdk)
----- Libs
----- * (Libs folder contents from vulkan sdk)
----- License.txt
--- ...
-- ...
```

## Installation
If you did not already acquire the necessary dependencies, do so before proceeding.

You will also need to acquire the sample model. I used the monkey from the Blender Foundation, Suzanne, but you can use any .obj file. Name this object "Suzanne.obj" and place it in the "Sample\objects\" subdirectory.

Point-Cloud Renderer uses premake as its build system. To generate VS2022 project files, run Scripts/Setup-Windows.bat.

### Shader Compilation
SPIR-V shaders are not pre-compiled in this repo. You must first generate compiled .spv files from each of the shaders in Sample/Shaders/. It is recommended that you use glslangValidator (included with Vulkan SDK) to do so. Open git bash in the root directory, and enter the following commands:

```
glslangValidator -V pointcloud.comp -o pointcloud.comp.spv

glslangValidator -V pointcloud.frag -o pointcloud.frag.spv

glslangValidator -V pointcloud.vert -o pointcloud.vert.spv
```

If bash is unable to locate glslangValidator, you made need to use the absolute file path to glslangValidator.exe. Note that this is not in the directories that we added to the project files. You can find glslandValidator in the following subdirectory within the Vulkan SDK:

```
Bin\glslangValidator.exe
```

## Compilation
From your IDE or from the command line, build and compile the solution with "Sample" as the startup project. The generated binaries can be found in
```
bin\{platform}-{architecture}\{config}\
```

i.e. for a Windows, x64, debug build, you will find the binaries in
```
bin\windows-x86_64\Debug\
```

To run the sample app, first copy the "objects" and "shaders" folders from the "Sample" subdirectory of the root directory, and paste them into the "Sample" subdirectory of the compiled binaries.

Everything in the "Sample" bin directory must remain alongside the executable. Run "Sample.exe" to view the rendered sample model.
