# Shared opendrt_ofx target (included from OpenDRTApple/Windows/Linux.cmake).

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OFX_SDK_PATH "${CMAKE_SOURCE_DIR}/openfx-sdk" CACHE PATH "Path to OFX SDK (include + Support)")

if(NOT EXISTS "${OFX_SDK_PATH}/include/ofxCore.h")
  message(FATAL_ERROR "OFX_SDK_PATH must contain include/ofxCore.h (got '${OFX_SDK_PATH}')")
endif()

set(OPENDRT_OFX_SUPPORT_SRCS
  "${OFX_SDK_PATH}/Support/Library/ofxsCore.cpp"
  "${OFX_SDK_PATH}/Support/Library/ofxsImageEffect.cpp"
  "${OFX_SDK_PATH}/Support/Library/ofxsInteract.cpp"
  "${OFX_SDK_PATH}/Support/Library/ofxsLog.cpp"
  "${OFX_SDK_PATH}/Support/Library/ofxsMultiThread.cpp"
  "${OFX_SDK_PATH}/Support/Library/ofxsParams.cpp"
  "${OFX_SDK_PATH}/Support/Library/ofxsProperty.cpp"
  "${OFX_SDK_PATH}/Support/Library/ofxsPropertyValidation.cpp"
)

set(OPENDRT_PLUGIN_CORE_SRCS
  "${CMAKE_SOURCE_DIR}/plugin/core/OpenDRT.cpp"
  "${CMAKE_SOURCE_DIR}/plugin/core/OpenDRTEffect.cpp"
  "${CMAKE_SOURCE_DIR}/plugin/core/OpenDRTRuntimeEnv.cpp"
  "${CMAKE_SOURCE_DIR}/plugin/core/OpenDRTDescribe.cpp"
  "${CMAKE_SOURCE_DIR}/plugin/core/OpenDRTInterop.cpp"
  "${CMAKE_SOURCE_DIR}/plugin/core/OpenDRTLookSections.cpp"
  "${CMAKE_SOURCE_DIR}/plugin/core/OpenDRTParamRouter.cpp"
  "${CMAKE_SOURCE_DIR}/plugin/core/OpenDRTPresetMenus.cpp"
  "${CMAKE_SOURCE_DIR}/plugin/core/OpenDRTPlatform.cpp"
  "${CMAKE_SOURCE_DIR}/plugin/core/OpenDRTRender.cpp"
  "${CMAKE_SOURCE_DIR}/plugin/core/OpenDRTUiState.cpp"
)

set(OPENDRT_INCLUDE_DIRS
  "${CMAKE_SOURCE_DIR}/plugin"
  "${CMAKE_SOURCE_DIR}/plugin/core"
  "${OFX_SDK_PATH}/include"
  "${OFX_SDK_PATH}/Support/include"
  "${OFX_SDK_PATH}/Support/Library"
  "${CMAKE_SOURCE_DIR}/plugin/metal"
  "${CMAKE_SOURCE_DIR}/plugin/cuda"
  "${CMAKE_SOURCE_DIR}/plugin/opencl"
)

file(MAKE_DIRECTORY "${OPENDRT_BIN_DIR}")

add_library(opendrt_ofx SHARED
  ${OPENDRT_PLUGIN_CORE_SRCS}
  ${OPENDRT_OFX_SUPPORT_SRCS}
  ${OPENDRT_PLATFORM_EXTRA_SRCS}
)

target_include_directories(opendrt_ofx PRIVATE ${OPENDRT_INCLUDE_DIRS} ${OPENDRT_PLATFORM_INCLUDE_DIRS})
target_compile_features(opendrt_ofx PRIVATE cxx_std_17)
target_compile_options(opendrt_ofx PRIVATE ${OPENDRT_COMPILE_OPTIONS})
target_compile_definitions(opendrt_ofx PRIVATE
  OFX_SUPPORTS_OPENCLRENDER
  _CRT_SECURE_NO_WARNINGS
  ${OPENDRT_COMPILE_DEFINITIONS}
)
if(OPENDRT_LINK_LIBS)
  target_link_libraries(opendrt_ofx PRIVATE ${OPENDRT_LINK_LIBS})
endif()
target_link_options(opendrt_ofx PRIVATE ${OPENDRT_LINK_OPTIONS})

set_target_properties(opendrt_ofx PROPERTIES
  PREFIX ""
  SUFFIX ".ofx"
  OUTPUT_NAME "${OPENDRT_OFX_BUNDLE_STEM}"
  LIBRARY_OUTPUT_DIRECTORY "${OPENDRT_BIN_DIR}"
)

add_dependencies(opendrt_ofx opendrt_gen_version)
if(OPENDRT_EXTRA_TARGET_DEPS)
  add_dependencies(opendrt_ofx ${OPENDRT_EXTRA_TARGET_DEPS})
endif()

function(opendrt_assemble_bundle_postbuild p_PlatformSubdir)
  set(_bundle_ofx "${OPENDRT_OFX_BUNDLE}/Contents/${p_PlatformSubdir}/${OPENDRT_OFX_EXECUTABLE_NAME}")
  add_custom_command(TARGET opendrt_ofx POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${OPENDRT_DIST_PLATFORM_DIR}"
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${OPENDRT_OFX_BUNDLE}/Contents"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${OPENDRT_OFX_BUNDLE}/Contents/${p_PlatformSubdir}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${OPENDRT_OFX_BUNDLE}/Contents/Resources"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${OPENDRT_INFO_PLIST}" "${OPENDRT_OFX_BUNDLE}/Contents/Info.plist"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:opendrt_ofx>" "${_bundle_ofx}"
    COMMENT "Assemble ${OPENDRT_OFX_BUNDLE_STEM}.ofx.bundle (${p_PlatformSubdir}) -> ${OPENDRT_DIST_PLATFORM_DIR}"
  )
endfunction()

add_custom_target(opendrt_all DEPENDS opendrt_ofx)
