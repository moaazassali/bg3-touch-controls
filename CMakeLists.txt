# CMakeList.txt : CMake project for BG3TouchControls, include source and define
# project specific logic here.
#
# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

cmake_minimum_required (VERSION 3.31.6)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project ("BG3TouchControls")

# Add source to this project's executable.
add_library (BG3TouchControls SHARED
	"src/dllmain.cpp"
	"src/setup_thread.cpp"
	"src/custom_wnd_proc.cpp"
	"src/interaction_context.cpp"
	"src/Hooks/update_camera_hook.cpp"
	"src/Hooks/calculate_pitch_hook.cpp"
	"src/Hooks/update_camera_pitch_hook.cpp"
	"src/Hooks/process_input_hook.cpp"
	"src/reverse/action_map.cpp"
	"src/game_bindings.cpp"
	"src/sdl_bindings/sdl_create_window_hook.cpp"
	"src/sdl_bindings/sdl_bindings.cpp"
	"src/sdl_bindings/send_sdl_events.cpp"
	"src/reverse/action_sequences/action_sequences.cpp"
	"src/reverse/action_sequences/swipe_up_sequence.cpp"
	"src/reverse/action_sequences/swipe_down_sequence.cpp"
	"src/Hooks/unk_consumes_filtered_actions_queue_hook.cpp"
	"src/asm_patches/minimum_hotbar_rows_patch.cpp"
	"src/utils.cpp"
	"src/game_patches/register_undo_multi_target_command.cpp"
	"src/Hooks/debug_hooks.cpp"
	"src/reverse/action_sequences/world_swipe_right_sequence.cpp"
	"src/reverse/action_sequences/world_swipe_left_sequence.cpp"
	"src/reverse/action_sequences/world_swipe_toggle_sequence.cpp"
	"src/joystick/dx11_present_hook.cpp"
)

target_include_directories(BG3TouchControls PRIVATE "include/")

# Neded for Interaction Context
# See Requirements section in https://learn.microsoft.com/en-us/windows/win32/api/interactioncontext/nf-interactioncontext-createinteractioncontext
target_link_libraries(BG3TouchControls PRIVATE ninput)

# Add MinHook using FetchContent
include(FetchContent)
FetchContent_Declare(
  minhook
  GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
  GIT_TAG        master 
)
FetchContent_MakeAvailable(minhook)
target_link_libraries(BG3TouchControls PRIVATE minhook)

# Add SDL2 headers (we are cloning the entire repo just for the headers... should maybe fix)
FetchContent_Populate(
	sdl2
	GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
	GIT_TAG        ba2f78a0069118a6c583f1fbf1420144ffa35bad
	GIT_SHALLOW    TRUE
)
# Your existing target_include_directories call will work perfectly with this
target_include_directories(BG3TouchControls PUBLIC "${sdl2_SOURCE_DIR}/include")

# === ImGui Integration (LLM generated; not carefully checked but it works) ===
# Add ImGui repo
FetchContent_Declare(
	imgui
	GIT_REPOSITORY https://github.com/ocornut/imgui.git
	GIT_TAG        docking # 'docking' branch is standard for mods/overlays, 'master' is fine too
)
FetchContent_MakeAvailable(imgui)
# Create a static library for ImGui so we don't clutter the main target
# We include the Core files AND the DX11/Win32 Backends
add_library(imgui STATIC
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp"
)
# Allow ImGui to find its own headers and the backend headers
target_include_directories(imgui PUBLIC 
    "${imgui_SOURCE_DIR}" 
    "${imgui_SOURCE_DIR}/backends"
)
# ImGui DX11 backend requires these system libraries
target_link_libraries(imgui PRIVATE d3d11)
target_link_libraries(BG3TouchControls PRIVATE imgui)