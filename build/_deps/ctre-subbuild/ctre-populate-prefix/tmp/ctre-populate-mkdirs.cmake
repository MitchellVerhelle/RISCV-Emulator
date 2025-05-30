# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/mitchellverhelle/C++ Class/Emulator/build/_deps/ctre-src")
  file(MAKE_DIRECTORY "/Users/mitchellverhelle/C++ Class/Emulator/build/_deps/ctre-src")
endif()
file(MAKE_DIRECTORY
  "/Users/mitchellverhelle/C++ Class/Emulator/build/_deps/ctre-build"
  "/Users/mitchellverhelle/C++ Class/Emulator/build/_deps/ctre-subbuild/ctre-populate-prefix"
  "/Users/mitchellverhelle/C++ Class/Emulator/build/_deps/ctre-subbuild/ctre-populate-prefix/tmp"
  "/Users/mitchellverhelle/C++ Class/Emulator/build/_deps/ctre-subbuild/ctre-populate-prefix/src/ctre-populate-stamp"
  "/Users/mitchellverhelle/C++ Class/Emulator/build/_deps/ctre-subbuild/ctre-populate-prefix/src"
  "/Users/mitchellverhelle/C++ Class/Emulator/build/_deps/ctre-subbuild/ctre-populate-prefix/src/ctre-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/mitchellverhelle/C++ Class/Emulator/build/_deps/ctre-subbuild/ctre-populate-prefix/src/ctre-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/mitchellverhelle/C++ Class/Emulator/build/_deps/ctre-subbuild/ctre-populate-prefix/src/ctre-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
