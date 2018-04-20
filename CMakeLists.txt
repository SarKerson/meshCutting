cmake_minimum_required(VERSION 3.2)
project(meshCutting)
add_compile_options(-DTETLIBRARY -std=c++11 -g)


find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

message( STATUS "    version: ${VTK_VERSION}" )
message( STATUS "    libraries: ${VTK_LIBS}" )
message( STATUS "    include path: ${VTK_INCLUDE_DIRS}" )

# add subdirecories
add_subdirectory(console)
add_subdirectory(core)
# add_subdirectory(gui)
