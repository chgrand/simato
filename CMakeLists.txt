cmake_minimum_required(VERSION 2.8)

PROJECT(simato)

option(INSTALL_SCRIPTS "Set to ON to also install the scripts" ON)
set(PYTHON_VERSION "2.7" CACHE STRING "Python version. Default is 2.7")

ADD_DEFINITIONS(-std=c++11)

# QT4 configuration
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
ADD_DEFINITIONS(${QT_DEFINITIONS})


# Binary simato
SET(simato_HEADERS 
  src/main_window.h
  src/map_viewer.h
  src/mission_viewer.h
  src/mission_dialog.h
  src/agent_dialog.h
  src/wp_group_dialog.h
)

#QT specific builds
QT4_WRAP_CPP(simato_HEADERS_MOC ${simato_HEADERS})


ADD_EXECUTABLE(simato
  src/colors.cpp 
  src/main.cpp
  src/main_window.cpp
  src/map_viewer.cpp
  src/mission_model.cpp
  src/mission_viewer.cpp
  src/mission_dialog.cpp
  src/agent_dialog.cpp
  src/wp_group_dialog.cpp
  ${simato_HEADERS_MOC}
  ${simato_HEADERS}
)
TARGET_LINK_LIBRARIES(simato ${QT_LIBRARIES})

################################
# Installation
################################
INSTALL(TARGETS simato DESTINATION bin)

IF(INSTALL_SCRIPTS)

  if (${PYTHON_VERSION} VERSION_GREATER 3)
      set(Python_ADDITIONAL_VERSIONS 3.3 3.4)
  endif()
  find_package(PythonInterp ${PYTHON_VERSION} REQUIRED)

  IF(NOT DEFINED PYTHON_INSTDIR)
    EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())" OUTPUT_VARIABLE PYTHON_INSTDIR OUTPUT_STRIP_TRAILING_WHITESPACE)
  ENDIF()
    
  INSTALL(PROGRAMS scripts/actionGenerator.py DESTINATION bin RENAME actionGenerator)
  INSTALL(FILES scripts/pddl.py scripts/actionvisu.py DESTINATION ${PYTHON_INSTDIR})
ENDIF()









