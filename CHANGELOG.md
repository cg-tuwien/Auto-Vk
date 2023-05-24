# Auto-Vk Versions

Notable _Auto-Vk_ versions in date-wise decending order:

### v0.98.1

**Date:** 24.05.2023

Major changes:
- Ray tracing buffers memory alignment to device-demanded offsets (https://github.com/cg-tuwien/Auto-Vk/commit/f831cec59d58113e0c87971756aa0c8531fc77cf)
- Awareness of official `VK_EXT_mesh_shader` mesh shader stages (https://github.com/cg-tuwien/Auto-Vk/commit/54d561170db952f44264569d0cd3c6fc94967467)
- Added some mesh shader commands: `draw_mesh_tasks_nv` and `draw_mesh_tasks_ext` (https://github.com/cg-tuwien/Auto-Vk/commit/d7fe3b09f809484b773e3e2577c63873653b6cbf)
- _Auto-Vk_ includes are now using quotes instead of angle brackets (https://github.com/cg-tuwien/Auto-Vk/commit/ff0a882c613b64dd176ed445c43601428fc5330b)
- Added a compiler switch between using Synchronization2 API functions (`2KHR`) and core functions (`2`) (https://github.com/cg-tuwien/Auto-Vk/commit/f88cac14fe412773d40e89668163c8c877b43f73)
- Reworked and fixed query pool usage (https://github.com/cg-tuwien/Auto-Vk/commit/e7767662c0a51142a4fac1a9b9a511e60f90e0f2)
    - **Breaking change:** Parameter order and meaning of `create_query_pool_for_pipeline_statistics_queries` method have changed.
- VMA is no longer part of the _Auto-Vk_ repository, but in intended to be included from the Vulkan SDK henceforth. (https://github.com/cg-tuwien/Auto-Vk/commit/c5c7266c3d06d5b2acb1abb4f1be8f2d9744e3ae)

### v0.98

**Date:** 27.07.2022          
First version number.

Major changes:
- Synchronization refactoring, introducing `avk::sync::sync_hint`, `avk::sync::sync_type_command`, and related functionality.
- Commands refactoring, introducing `avk::command::action_type_command`, `avk::command::state_type_command`, and related functionality.
