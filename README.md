# Auto-Vk

_Auto-Vk_ is a low-level convenience and productivity layer atop [Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp). 

# Requirements and Integration

_Auto-Vk_ requires
* A Vulkan 1.2 SDK
* [Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp)

_Auto-Vk_ consists of multiple C++ include files and two (soon: one) C++ source file
* Add [`include/`](include/) to the include paths so that your compiler can find include files under paths `avk/*`
* Add [`src/avk.cpp`](src/avk.cpp) (and currently also [`src/sync.cpp`](src/sync.cpp)) as a compiled C++ source code file


