# Install script for directory: E:/dev/dx12RE/Sludge/libs/assimp/code

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files/SludgeDX12")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "libassimp6.0.2-dev" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "E:/dev/dx12RE/Sludge/build/libs/assimp/lib/Debug/assimp-vc143-mtd.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "E:/dev/dx12RE/Sludge/build/libs/assimp/lib/Release/assimp-vc143-mt.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "E:/dev/dx12RE/Sludge/build/libs/assimp/lib/MinSizeRel/assimp-vc143-mt.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "E:/dev/dx12RE/Sludge/build/libs/assimp/lib/RelWithDebInfo/assimp-vc143-mt.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "assimp-dev" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/assimp" TYPE FILE FILES
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/anim.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/aabb.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/ai_assert.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/camera.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/color4.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/color4.inl"
    "E:/dev/dx12RE/Sludge/build/libs/assimp/code/../include/assimp/config.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/ColladaMetaData.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/commonMetaData.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/defs.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/cfileio.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/light.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/material.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/material.inl"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/matrix3x3.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/matrix3x3.inl"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/matrix4x4.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/matrix4x4.inl"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/mesh.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/ObjMaterial.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/pbrmaterial.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/GltfMaterial.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/postprocess.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/quaternion.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/quaternion.inl"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/scene.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/metadata.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/texture.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/types.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/vector2.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/vector2.inl"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/vector3.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/vector3.inl"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/version.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/cimport.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/AssertHandler.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/importerdesc.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/Importer.hpp"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/DefaultLogger.hpp"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/ProgressHandler.hpp"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/IOStream.hpp"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/IOSystem.hpp"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/Logger.hpp"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/LogStream.hpp"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/NullLogger.hpp"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/cexport.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/Exporter.hpp"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/DefaultIOStream.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/DefaultIOSystem.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/ZipArchiveIOSystem.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/SceneCombiner.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/fast_atof.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/qnan.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/BaseImporter.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/Hash.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/MemoryIOWrapper.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/ParsingUtils.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/StreamReader.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/StreamWriter.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/StringComparison.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/StringUtils.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/SGSpatialSort.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/GenericProperty.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/SpatialSort.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/SkeletonMeshBuilder.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/SmallVector.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/SmoothingGroups.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/SmoothingGroups.inl"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/StandardShapes.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/RemoveComments.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/Subdivision.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/Vertex.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/LineSplitter.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/TinyFormatter.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/Profiler.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/LogAux.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/Bitmap.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/XMLTools.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/IOStreamBuffer.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/CreateAnimMesh.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/XmlParser.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/BlobIOSystem.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/MathFunctions.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/Exceptional.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/ByteSwapper.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/Base64.hpp"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "assimp-dev" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/assimp/Compiler" TYPE FILE FILES
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/Compiler/pushpack1.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/Compiler/poppack1.h"
    "E:/dev/dx12RE/Sludge/libs/assimp/code/../include/assimp/Compiler/pstdint.h"
    )
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "E:/dev/dx12RE/Sludge/build/libs/assimp/code/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
