# LSP - Simple Open DRT OFX — unified CMake (macOS + Windows + Linux)
cmake_minimum_required(VERSION 3.22)

option(OPENDRT_OFX_FAT_ARCHS "Link universal Mach-O (arm64 + x86_64)" ON)
option(SIMPLE_OPENDRT_LINUX_CUDA "Enable CUDA backend in the Linux plugin build" ON)

if(CMAKE_HOST_APPLE)
  set(OPENDRT_MACOSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Minimum macOS deployment target")
  set(CMAKE_OSX_DEPLOYMENT_TARGET "${OPENDRT_MACOSX_DEPLOYMENT_TARGET}" CACHE STRING "" FORCE)
  if(OPENDRT_OFX_FAT_ARCHS)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS build architectures" FORCE)
  endif()
elseif(NOT CMAKE_HOST_WIN32 AND NOT CMAKE_HOST_UNIX)
  message(FATAL_ERROR "LSP Simple Open DRT CMake supports macOS, Windows, and Linux only.")
endif()

if(CMAKE_HOST_APPLE)
  project(LSP_Simple_Open_DRT
    VERSION 0.0.0
    DESCRIPTION "LSP - Simple Open DRT OFX"
    LANGUAGES C CXX OBJCXX
  )
elseif(CMAKE_HOST_WIN32)
  project(LSP_Simple_Open_DRT
    VERSION 0.0.0
    DESCRIPTION "LSP - Simple Open DRT OFX"
    LANGUAGES C CXX
  )
else()
  project(LSP_Simple_Open_DRT
    VERSION 0.0.0
    DESCRIPTION "LSP - Simple Open DRT OFX"
    LANGUAGES C CXX
  )
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/OpenDRTVersion.cmake")

if(APPLE)
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/OpenDRTApple.cmake")
elseif(WIN32)
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/OpenDRTWindows.cmake")
elseif(UNIX)
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/OpenDRTLinux.cmake")
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/scripts/archive_version.sh")
  add_custom_target(opendrt_archive
    COMMAND bash "${CMAKE_SOURCE_DIR}/scripts/archive_version.sh"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    USES_TERMINAL
    COMMENT "Archive current version, bump VERSION, rebuild"
  )
endif()
