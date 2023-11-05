# Vulkan PBR Renderer

A simple PBR renderer written in CPP using Vulkan. May expand into something more in the future, or just be used as boilerplate for other projects. Still in its early stages.

## Dependencies

- ImGui \[User Interface] - included as a submodule. Compiled and linked together with the project.
- TinyObjLoader \[Model Loading] - header is included in the `src/include` folder.
- stb_image \[Texture Loading] - same as above.
- GLFW \[Window Manager] - must be compiled and installed. Place headers into a top level `include` directory next to the `src` folder. Lib file can be placed in a top level `lib` folder.
- Vulkan \[Graphics API] - installed through the VulkanSDK installer.
- Vulkan Memory Allocator \[Vulkan Memory Management] - Can be installed seperately or through the VulkanSDK.
- GLM \[Matrix Math] - headers can be downloaded and placed in a top level `include/GLM` directory next to `src`. Or you can download these as a part of the VulkanSDK.

## How to Compile

The project is structured to use CMake. You can use CMake to configure and create build files for whatever toolchain you like. You can find a tutorial on how to use CMake online.

If you want to enable validation layers, you may enable the `ENABLE_VALIDATION_LAYERS` cmake option. The validation layer settings can then be configured in the `vk_layer_settings.txt` file. I plan to add more CMake options to control debug output, along with a better system for logging custom debug output.

I use CMake's `find_package` to search for GLFW, Vukan, and VMA library and header files. This command searches differently on different platforms so make sure to figure that out if you have errors. I use `set(CMAKE_PREFIX_PATH path)` in the `CMakeLists.txt` file to specify where the `glfw3Config.cmake` file can be found which in turn gives CMake the directions to find GLFW's headers and library files. This `set` command can be edited to search elsewhere if you want.

## Todo List
- [ ] add to do list
