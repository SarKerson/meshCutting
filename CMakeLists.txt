cmake_minimum_required(VERSION 2.8)

PROJECT(demo)
aux_source_directory(${PROJECT_BINARY_DIR}/../ SRCS)

find_package(VTK REQUIRED)
find_package( OpenCV REQUIRED )
include_directories(${OpenCV_INCLUDE_DIRS})
include(${VTK_USE_FILE})

add_executable(demo MACOSX_BUNDLE ${SRCS} )

target_link_libraries(demo ${VTK_LIBRARIES} ${OpenCV_LIBS})