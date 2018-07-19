cmake_minimum_required (VERSION 3.8)

set (HEADER_FILES common.hpp TestUtilities.hpp)

set (SOURCE_FILES random.cpp
                  lens.cpp
    )

add_library (domiplan_test_objs OBJECT ${SOURCE_FILES} ${HEADER_FILES})
target_compile_features (domiplan_test_objs PUBLIC cxx_std_14)
target_include_directories (domiplan_test_objs PUBLIC $<TARGET_PROPERTY:Domiplan,INTERFACE_INCLUDE_DIRECTORIES>
                                                  ${DOMIPLAN_SOURCE_DIR}/ext)
set_target_properties (domiplan_test_objs PROPERTIES
                       COTIRE_PREFIX_HEADER_INIT common.hpp
                       COTIRE_PREFIX_HEADER_INCLUDE_PATH ${DOMIPLAN_SOURCE_DIR}/ext)

if (COMMAND cotire)
    cotire (domiplan_test_objs)
endif ()

add_executable (domiplan_test main.cpp $<TARGET_OBJECTS:domiplan_test_objs>)
target_link_libraries (domiplan_test PRIVATE Domiplan)
target_include_directories (domiplan_test PRIVATE ${DOMIPLAN_SOURCE_DIR}/ext)
target_compile_features (domiplan_test PRIVATE cxx_std_14)

add_test (NAME domiplan_test
          COMMAND domiplan_test
          WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
