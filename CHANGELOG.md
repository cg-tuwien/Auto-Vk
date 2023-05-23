# Auto-Vk Versions

Notable _Auto-Vk_ versions in date-wise decending order:

### v0.98.1

**Date:** 23.05.2023

Major changes:
- Ray tracing buffers memory alignment to device-demanded offsets (f831cec59d58113e0c87971756aa0c8531fc77cf)
- Awareness of official `VK_EXT_mesh_shader` mesh shader stages (54d561170db952f44264569d0cd3c6fc94967467)
- _Auto-Vk_ includes are now using quotes instead of angle brackets (ff0a882c613b64dd176ed445c43601428fc5330b)
- Added a compiler switch between using Synchronization2 API functions (`2KHR`) and core functions (`2`) (f88cac14fe412773d40e89668163c8c877b43f73)
- Reworked and fixed query pool usage (e7767662c0a51142a4fac1a9b9a511e60f90e0f2)
    - _Breaking change:_ Parameter order and meaning of `create_query_pool_for_pipeline_statistics_queries` method have changed.
- VMA is no longer part of the _Auto-Vk_ repository, but in intended to be included from the Vulkan SDK henceforth.

### v0.98

**Date:** 27.07.2022          
First version number.

Major changes:
- Synchronization refactoring, introducing `avk::sync::sync_hint`, `avk::sync::sync_type_command`, and related functionality.
- Commands refactoring, introducing `avk::command::action_type_command`, `avk::command::state_type_command`, and related functionality.
