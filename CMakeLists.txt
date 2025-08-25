# CMakeList.txt : CMake project for BG3TouchInput, include source and define
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

project ("BG3TouchInput")

# Add source to this project's executable.
add_library (BG3TouchInput SHARED
	"src/dllmain.cpp"
	"src/setup_thread.cpp"
	"src/custom_wnd_proc.cpp"
	"src/interaction_context.cpp"
	"src/hooks.cpp"
 "include/pending_camera_updates.h")

target_include_directories(BG3TouchInput PRIVATE "include/")

# Neded for Interaction Context
# See Requirements section in https://learn.microsoft.com/en-us/windows/win32/api/interactioncontext/nf-interactioncontext-createinteractioncontext
target_link_libraries(BG3TouchInput PRIVATE ninput)

# Add MinHook using FetchContent
include(FetchContent)
FetchContent_Declare(
  minhook
  GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
  GIT_TAG        master 
)
FetchContent_MakeAvailable(minhook)
target_link_libraries(BG3TouchInput PRIVATE minhook)