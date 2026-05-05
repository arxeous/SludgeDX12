cmake_minimum_required(VERSION 3.20)
project(SludgeDX12)

file(GLOB AGILITY_SDK_DIR 
    "${CMAKE_SOURCE_DIR}/libs/DX12AgilitySDK"
)

find_program(DXC_EXECUTABLE
    NAMES dxc dxc.exe
    PATHS
        "${CMAKE_SOURCE_DIR}/libs/dxc/bin/x64"
        "$ENV{WindowsSdkDir}bin/$ENV{WindowsSDKVersion}x64"
        "C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64"
    DOC "DirectX Shader Compiler"
)
if(NOT DXC_EXECUTABLE)
    message(FATAL_ERROR "DXC not found! Please install Windows SDK or download DXC.")
endif()

message(STATUS "Found DXC: ${DXC_EXECUTABLE}")

function(compile_shader TARGET SHADER)
    set(options "")
    set(oneValueArgs ENTRY_POINT OUTPUT_NAME)
    set(multiValueArgs DEFINES)
    cmake_parse_arguments(SHADER_ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT SHADER_ARG_ENTRY_POINT)
        set(SHADER_ARG_ENTRY_POINT "main")
    endif()

    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    get_filename_component(SHADER_EXT ${SHADER} EXT)
    
    # Determine shader profile based on extension
    if(SHADER_EXT STREQUAL ".vs.hlsl" OR SHADER_EXT STREQUAL ".vert")
        set(PROFILE "vs_6_6")
    elseif(SHADER_EXT STREQUAL ".ps.hlsl" OR SHADER_EXT STREQUAL ".frag")
        set(PROFILE "ps_6_6")
    elseif(SHADER_EXT STREQUAL ".cs.hlsl" OR SHADER_EXT STREQUAL ".comp")
        set(PROFILE "cs_6_6")
    else()
        message(WARNING "Unknown shader type: ${SHADER}")
        return()
    endif()
    
        # Use custom output name if provided, otherwise generate one
    if(SHADER_ARG_OUTPUT_NAME)
        set(OUTPUT_FILE "${CMAKE_SOURCE_DIR}/src/shaders/cso/${SHADER_ARG_OUTPUT_NAME}.cso")
    else()
        set(OUTPUT_FILE "${CMAKE_SOURCE_DIR}/src/shaders/cso/${SHADER_NAME}.cso")
    endif()
    
    set(DEFINE_ARGS "")
    foreach(DEFINE ${SHADER_ARG_DEFINES})
        list(APPEND DEFINE_ARGS "-D${DEFINE}")
    endforeach()

    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/src/shaders"
        COMMAND ${DXC_EXECUTABLE}
            -T ${PROFILE}
            -E ${SHADER_ARG_ENTRY_POINT}
            ${DEFINE_ARGS}
            -Fo ${OUTPUT_FILE}
            ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}
                ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/Utils/Utils.hlsl
                ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/Utils/LightingUtils.hlsl
        COMMENT "Compiling shader: ${SHADER} with defines: ${SHADER_ARG_DEFINES}"
        VERBATIM
    )
    
    target_sources(${TARGET} PRIVATE ${OUTPUT_FILE})
endfunction()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "")
set(ASSIMP_BUILD_DRACO OFF CACHE BOOL "")
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "")
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "")
set(ASSIMP_INSTALL_PDB OFF CACHE BOOL "")
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "")
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "")
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "")
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "")
add_subdirectory(libs/assimp)

include_directories(.)
include_directories(libs)
include_directories(libs/imgui/)
include_directories(libs/ImGuizmo/)

set_property(TARGET assimp          PROPERTY FOLDER "third-party")
if(WIN32)
  if(TARGET zlibstatic)
    set_property(TARGET zlibstatic  PROPERTY FOLDER "third-party")
  endif()
  if(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs)
    set_property(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTY FOLDER "third-party")
  endif()
endif()
set_property(TARGET uninstall       PROPERTY FOLDER "third-party")
add_definitions(-D_UNICODE -DUNICODE)

if(MSVC)
	target_compile_options(assimp PRIVATE /wd4267)
endif()

file(GLOB_RECURSE SRC_FILES
    "src/*.h"
    "src/*.cpp"
    "src/*.c"
    "src/*.hpp"
)


set(ENTRY_FILES
    entry/main.cpp
)

set(IMGUI_SRC
    libs/imgui/imgui.h
    libs/imgui/imgui.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_widgets.cpp
    libs/imgui/imgui_demo.cpp
    libs/imgui/backends/imgui_impl_win32.h
    libs/imgui/backends/imgui_impl_win32.cpp
    libs/imgui/backends/imgui_impl_dx12.h
    libs/imgui/backends/imgui_impl_dx12.cpp
)



# Create the executable
add_executable(${PROJECT_NAME} WIN32
    ${ENTRY_FILES}
    ${SRC_FILES}
    ${IMGUI_SRC}
)

compile_shader(${PROJECT_NAME} src/shaders/PBR/VertexShader.vs.hlsl
OUTPUT_NAME "vs"
DEFINES "")
compile_shader(${PROJECT_NAME} src/shaders/PBR/PixelShader.ps.hlsl
OUTPUT_NAME "ps"
DEFINES "")

compile_shader(${PROJECT_NAME} src/shaders/Offscreen/OSVertex.vs.hlsl
OUTPUT_NAME "osVS"
DEFINES "")
compile_shader(${PROJECT_NAME} src/shaders/Offscreen/OSPixel.ps.hlsl
OUTPUT_NAME "osPS"
DEFINES "")


compile_shader(${PROJECT_NAME} src/shaders/PBR/SkyBoxVS.vs.hlsl
OUTPUT_NAME "sbVS"
DEFINES "")
compile_shader(${PROJECT_NAME} src/shaders/PBR/SkyBoxPS.ps.hlsl
OUTPUT_NAME "sbPS"
DEFINES "")

compile_shader(${PROJECT_NAME} src/shaders/Compute/Equirect2cube.cs.hlsl
OUTPUT_NAME "skyboxCompute"
DEFINES "")

compile_shader(${PROJECT_NAME} src/shaders/Compute/IrradianceConvolve.cs.hlsl
OUTPUT_NAME "irradianceCompute"
DEFINES "")

compile_shader(${PROJECT_NAME} src/shaders/Compute/PrefilteredEnv.cs.hlsl
OUTPUT_NAME "prefilterCompute"
DEFINES "")

compile_shader(${PROJECT_NAME} src/shaders/Compute/IntegrateBRDF.cs.hlsl
OUTPUT_NAME "lutCompute"
DEFINES "")



if(AGILITY_SDK_DIR)
    # Get the first match (in case multiple versions exist)
    list(GET AGILITY_SDK_DIR 0 AGILITY_SDK_PATH)
    
    message(STATUS "Found Agility SDK at: ${AGILITY_SDK_PATH}")
    
    # Copy DLLs to output directory after build
    add_custom_command(TARGET SludgeDX12 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory 
            "$<TARGET_FILE_DIR:SludgeDX12>/D3D12"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${AGILITY_SDK_PATH}/build/native/bin/x64/D3D12Core.dll"
            "$<TARGET_FILE_DIR:SludgeDX12>/D3D12/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${AGILITY_SDK_PATH}/build/native/bin/x64/d3d12SDKLayers.dll"
            "$<TARGET_FILE_DIR:SludgeDX12>/D3D12/"
        COMMENT "Copying Agility SDK DLLs from NuGet package"
    )
else()
    message(WARNING "Agility SDK NuGet package not found in packages folder")
endif()

include_directories(.)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_sources(${PROJECT_NAME} PRIVATE
    libs/ImGuizmo/ImGuizmo.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    libs/ImGuizmo
)
source_group("entry" FILES ${ENTRY_FILES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src PREFIX "src" FILES ${SRC_FILES})

# Windows subsystem settings
if(MSVC)
    # Set as Windows application (not console)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup"
    )
    
    # Disable some annoying MSVC warnings
    add_compile_options(/wd4996)  # Disable deprecated warnings
    add_compile_options(/W4)      # Warning level 4
endif()

# Link Windows libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    d3d12.lib
    dxgi.lib
    dxguid.lib
    d3dcompiler.lib
    assimp
)
