cmake_minimum_required(VERSION 2.8)
add_compile_options(-fpermissive -std=c++11 -g)

PROJECT(console)
aux_source_directory(. SRCS)

find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

message( STATUS "    version: ${VTK_VERSION}" )
message( STATUS "    libraries: ${VTK_LIBS}" )
message( STATUS "    include path: ${VTK_INCLUDE_DIRS}" )

add_executable(console MACOSX_BUNDLE ${SRCS} )
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

target_link_libraries(console ${VTK_LIBRARIES})
