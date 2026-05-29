set(CMAKE_OBJCXX_STANDARD 17)
set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)

set(OPENDRT_BUILD_ROOT "${CMAKE_SOURCE_DIR}/build/macos" CACHE PATH "macOS build intermediates")
set(OPENDRT_DIST_PLATFORM_DIR "${CMAKE_SOURCE_DIR}/release/${OPENDRT_RELEASE_FOLDER_MACOS}")
set(OPENDRT_BIN_DIR "${OPENDRT_BUILD_ROOT}/bin")
set(OPENDRT_INFO_PLIST "${OPENDRT_BUILD_ROOT}/Info.plist")
set(OPENDRT_OFX_BUNDLE "${OPENDRT_DIST_PLATFORM_DIR}/${OPENDRT_OFX_BUNDLE_STEM}.ofx.bundle")

file(MAKE_DIRECTORY "${OPENDRT_BUILD_ROOT}")
configure_file(
  "${CMAKE_SOURCE_DIR}/Info.plist.in"
  "${OPENDRT_INFO_PLIST}"
  @ONLY
)
add_custom_target(opendrt_gen_plist DEPENDS "${OPENDRT_INFO_PLIST}")

set(OPENDRT_PLATFORM_EXTRA_SRCS "${CMAKE_SOURCE_DIR}/plugin/metal/OpenDRTMetal.mm")
set(OPENDRT_PLATFORM_INCLUDE_DIRS "")
set(OPENDRT_LIBRARY_TYPE MODULE)

set(OPENDRT_COMPILE_OPTIONS
  -O2
  -Wno-dynamic-exception-spec
  -fvisibility=hidden
)
set(OPENDRT_COMPILE_DEFINITIONS "")

find_program(XCRUN_EXECUTABLE xcrun REQUIRED)
set(METAL_SRC "${CMAKE_SOURCE_DIR}/plugin/metal/OpenDRT.metal")
set(METAL_AIR "${OPENDRT_BUILD_ROOT}/OpenDRT.air")
set(METAL_LIB "${OPENDRT_BUILD_ROOT}/OpenDRT.metallib")
if(CMAKE_OSX_DEPLOYMENT_TARGET)
  set(METAL_MIN_OS "${CMAKE_OSX_DEPLOYMENT_TARGET}")
else()
  set(METAL_MIN_OS "13.0")
endif()

add_custom_command(
  OUTPUT "${METAL_AIR}"
  COMMAND "${XCRUN_EXECUTABLE}" -sdk macosx metal -std=macos-metal2.4 -mmacosx-version-min=${METAL_MIN_OS} -c "${METAL_SRC}" -o "${METAL_AIR}"
  DEPENDS "${METAL_SRC}" "${CMAKE_SOURCE_DIR}/plugin/core/OpenDRTParams.h"
  VERBATIM
)

add_custom_command(
  OUTPUT "${METAL_LIB}"
  COMMAND "${XCRUN_EXECUTABLE}" -sdk macosx metallib "${METAL_AIR}" -o "${METAL_LIB}"
  DEPENDS "${METAL_AIR}"
  VERBATIM
)

add_custom_target(OpenDRTMetalLib ALL DEPENDS "${METAL_LIB}")
set(OPENDRT_EXTRA_TARGET_DEPS OpenDRTMetalLib opendrt_gen_plist)

find_library(OPENDRT_FRAMEWORK_METAL NAMES Metal REQUIRED)
find_library(OPENDRT_FRAMEWORK_FOUNDATION NAMES Foundation REQUIRED)
find_library(OPENDRT_FRAMEWORK_COREFOUNDATION NAMES CoreFoundation REQUIRED)
set(OPENDRT_LINK_LIBS
  ${OPENDRT_FRAMEWORK_METAL}
  ${OPENDRT_FRAMEWORK_FOUNDATION}
  ${OPENDRT_FRAMEWORK_COREFOUNDATION}
)
set(OPENDRT_LINK_OPTIONS
  "-bundle"
  "-fvisibility=hidden"
  "-Wl,-rpath,@loader_path"
)

include("${CMAKE_CURRENT_LIST_DIR}/OpenDRTCommon.cmake")

target_compile_options(opendrt_ofx PRIVATE
  "$<$<COMPILE_LANGUAGE:OBJCXX>:-fblocks>"
)

add_custom_command(TARGET opendrt_ofx POST_BUILD
  COMMAND strip -x "$<TARGET_FILE:opendrt_ofx>"
  COMMENT "strip -x opendrt_ofx"
)

opendrt_assemble_bundle_postbuild("MacOS")

add_custom_command(TARGET opendrt_ofx POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "${METAL_LIB}" "${OPENDRT_OFX_BUNDLE}/Contents/Resources/OpenDRT.metallib"
)

message(STATUS "[OpenDRT] macOS bundle: ${OPENDRT_OFX_BUNDLE}")
if(OPENDRT_OFX_FAT_ARCHS)
  message(STATUS "[OpenDRT] macOS architectures: ${CMAKE_OSX_ARCHITECTURES}")
else()
  message(STATUS "[OpenDRT] macOS architectures: host default")
endif()
