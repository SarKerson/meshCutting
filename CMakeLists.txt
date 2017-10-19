cmake_minimum_required(VERSION 2.8)
add_compile_options(-fpermissive -std=c++11 -g)

PROJECT(demo)
aux_source_directory(${PROJECT_BINARY_DIR}/../ SRCS)

find_package(VTK REQUIRED)
find_package( OpenCV REQUIRED )
include_directories(${OpenCV_INCLUDE_DIRS})
include(${VTK_USE_FILE})

message( STATUS "OpenCV library status:" )              # 输出一下得到的变量
message( STATUS "    version: ${VTK_VERSION}" )
message( STATUS "    libraries: ${VTK_LIBS}" )
message( STATUS "    include path: ${VTK_INCLUDE_DIRS}" )



add_executable(demo MACOSX_BUNDLE ${SRCS} )

target_link_libraries(demo ${VTK_LIBRARIES} ${OpenCV_LIBS})