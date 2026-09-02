# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "E:/dev/dx12RE/Sludge/build/_deps/meshoptimizer-src")
  file(MAKE_DIRECTORY "E:/dev/dx12RE/Sludge/build/_deps/meshoptimizer-src")
endif()
file(MAKE_DIRECTORY
  "E:/dev/dx12RE/Sludge/build/_deps/meshoptimizer-build"
  "E:/dev/dx12RE/Sludge/build/_deps/meshoptimizer-subbuild/meshoptimizer-populate-prefix"
  "E:/dev/dx12RE/Sludge/build/_deps/meshoptimizer-subbuild/meshoptimizer-populate-prefix/tmp"
  "E:/dev/dx12RE/Sludge/build/_deps/meshoptimizer-subbuild/meshoptimizer-populate-prefix/src/meshoptimizer-populate-stamp"
  "E:/dev/dx12RE/Sludge/build/_deps/meshoptimizer-subbuild/meshoptimizer-populate-prefix/src"
  "E:/dev/dx12RE/Sludge/build/_deps/meshoptimizer-subbuild/meshoptimizer-populate-prefix/src/meshoptimizer-populate-stamp"
)

set(configSubDirs Debug)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "E:/dev/dx12RE/Sludge/build/_deps/meshoptimizer-subbuild/meshoptimizer-populate-prefix/src/meshoptimizer-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "E:/dev/dx12RE/Sludge/build/_deps/meshoptimizer-subbuild/meshoptimizer-populate-prefix/src/meshoptimizer-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
