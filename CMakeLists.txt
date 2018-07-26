cmake_minimum_required (VERSION 3.8)

project (DOMIPLAN
         LANGUAGES CXX C
         VERSION 0.0.1)

list (INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/CMake)
include (GNUInstallDirs)
include (cotire OPTIONAL)
add_library (Domiplan INTERFACE)
target_include_directories (Domiplan INTERFACE
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                            $<INSTALL_INTERFACE:include>)

enable_testing ()

include_directories(ColorSystem/include)
include_directories(stb)

add_subdirectory (test)
add_subdirectory (ColorSystem)
add_subdirectory (lenstest)

install (DIRECTORY include DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
