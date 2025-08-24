# CMakeList.txt : CMake project for BG3TouchInput, include source and define
# project specific logic here.
#
# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

cmake_minimum_required (VERSION 3.31.6)

project ("BG3TouchInput")

# Add source to this project's executable.
add_executable (BG3TouchInput "BG3TouchInput.cpp" "BG3TouchInput.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET BG3TouchInput PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
