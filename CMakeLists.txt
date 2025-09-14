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