set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

# -----VTK-------------
# message( STATUS "    searching VTK..." )
# find_package(VTK REQUIRED)
# include(${VTK_USE_FILE})
# message( STATUS "    version: ${VTK_VERSION}" )
# message( STATUS "    libraries: ${VTK_LIBS}" )
# message( STATUS "    include path: ${VTK_INCLUDE_DIRS}" )


# ------MY LIBS---------
INCLUDE_DIRECTORIES(./strutil/include ./cellPicker/include ./tetgen/include)
AUX_SOURCE_DIRECTORY(./strutil/src STRUTIL_LIB_CPP)
AUX_SOURCE_DIRECTORY(./cellPicker/src  CELLPICKER_LIB_CPP)
AUX_SOURCE_DIRECTORY(./tetgen/src  TETGEN_LIB_CPP)
# list(APPEND LIB_CPP ${HELLO_LIB_CPP} ${SQRT_LIB_CPP})    # merge two or more single lib into one
message(STATUS "STRUTIL_LIB_CPP = ${STRUTIL_LIB_CPP}")
ADD_LIBRARY(strutil SHARED ${STRUTIL_LIB_CPP})
message(STATUS "CELLPICKER_LIB_CPP = ${CELLPICKER_LIB_CPP}")
ADD_LIBRARY(cellPicker SHARED ${CELLPICKER_LIB_CPP})
message(STATUS "TETGEN_LIB_CPP = ${TETGEN_LIB_CPP}")
ADD_LIBRARY(tetgen SHARED ${TETGEN_LIB_CPP})

# ------produce demos----
add_subdirectory(./gui)

