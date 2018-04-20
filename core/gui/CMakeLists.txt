PROJECT(meshcutting-gui)

cmake_minimum_required(VERSION 2.8)
 
if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()

if(${VTK_VERSION} VERSION_GREATER "6" AND VTK_QT_VERSION VERSION_GREATER "4")
  # Instruct CMake to run moc automatically when needed.
  set(CMAKE_AUTOMOC ON)
  find_package(Qt5Widgets REQUIRED QUIET)
else()
  find_package(Qt4 REQUIRED)
  include(${QT_USE_FILE})
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} )

file(GLOB UI_FILES *.ui)
file(GLOB QT_WRAP *.h)
file(GLOB CXX_FILES *.cpp)

if(${VTK_VERSION} VERSION_GREATER "6" AND VTK_QT_VERSION VERSION_GREATER "4")
  qt5_wrap_ui(UISrcs ${UI_FILES} )
  # CMAKE_AUTOMOC in ON so the MOC headers will be automatically wrapped.
  add_executable(meshcutting-gui MACOSX_BUNDLE
    ${CXX_FILES} ${UISrcs} ${QT_WRAP})
  qt5_use_modules(meshcutting-gui Core Gui)
  LINK_DIRECTORIES(${LIBRARY_OUTPUT_PATH})   # libs dir
  message(STATUS "GUI!!! ${LIBRARY_OUTPUT_PATH}")
  target_link_libraries(meshcutting-gui tetgen ${VTK_LIBRARIES} cellPicker )
else()
  QT4_WRAP_UI(UISrcs ${UI_FILES})
  QT4_WRAP_CPP(MOCSrcs ${QT_WRAP})
  add_executable(meshcutting-gui MACOSX_BUNDLE ${CXX_FILES} ${UISrcs} ${MOCSrcs})
  set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

  LINK_DIRECTORIES(${LIBRARY_OUTPUT_PATH})   # libs dir
  message(STATUS "GUI!!! ${LIBRARY_OUTPUT_PATH}")

  target_link_libraries(meshcutting-gui tetgen ${VTK_LIBRARIES} cellPicker )
endif()
