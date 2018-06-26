PROJECT(Trigger)

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

#where to find local cmake modules
FILE(TO_CMAKE_PATH "$ENV{SISODIR5}" SISODIR5)
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_INSTALL_PREFIX}/cmake/modules" "${SISODIR5}/cmake/modules")

INCLUDE(SisoPlatform)
FIND_PACKAGE(FgLib5 REQUIRED)
FIND_PACKAGE(SisoDisplayLib REQUIRED)

if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

INCLUDE_DIRECTORIES(
	${FGLIB5_INCLUDE_DIR}
	${SISODISPLAYLIB_INCLUDE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

ADD_EXECUTABLE(AreaExternal
	AreaExternal.cpp)
TARGET_LINK_LIBRARIES(AreaExternal
	${FGLIB5_LIBRARIES}
	${SISODISPLAYLIB_LIBRARIES}
)

ADD_EXECUTABLE(AreaFreeRun
	AreaFreeRun.cpp)
TARGET_LINK_LIBRARIES(AreaFreeRun
	${FGLIB5_LIBRARIES}
	${SISODISPLAYLIB_LIBRARIES}
)

ADD_EXECUTABLE(AreaGrabberControl
	AreaGrabberControl.cpp)
TARGET_LINK_LIBRARIES(AreaGrabberControl
	${FGLIB5_LIBRARIES}
	${SISODISPLAYLIB_LIBRARIES}
)

ADD_EXECUTABLE(AreaSoftware
	AreaSoftware.cpp)
TARGET_LINK_LIBRARIES(AreaSoftware
	${FGLIB5_LIBRARIES}
	${SISODISPLAYLIB_LIBRARIES}
)

ADD_EXECUTABLE(LineFreeRunImg
	LineFreeRunImg.cpp)
TARGET_LINK_LIBRARIES(LineFreeRunImg
	${FGLIB5_LIBRARIES}
	${SISODISPLAYLIB_LIBRARIES}
)

ADD_EXECUTABLE(LineGatedAsyncTriggerLine
	LineGatedAsyncTriggerLine.cpp)
TARGET_LINK_LIBRARIES(LineGatedAsyncTriggerLine
	${FGLIB5_LIBRARIES}
	${SISODISPLAYLIB_LIBRARIES}
)

ADD_EXECUTABLE(LineGatedGrabberControlledLine
	LineGatedGrabberControlledLine.cpp)
TARGET_LINK_LIBRARIES(LineGatedGrabberControlledLine
	${FGLIB5_LIBRARIES}
	${SISODISPLAYLIB_LIBRARIES}
)

ADD_EXECUTABLE(LineGrabberControlledLine
	LineGrabberControlledLine.cpp)
TARGET_LINK_LIBRARIES(LineGrabberControlledLine
	${FGLIB5_LIBRARIES}
	${SISODISPLAYLIB_LIBRARIES}
)

ADD_EXECUTABLE(LineStartTriggerImg
	LineStartTriggerImg.cpp)
TARGET_LINK_LIBRARIES(LineStartTriggerImg
	${FGLIB5_LIBRARIES}
	${SISODISPLAYLIB_LIBRARIES}
)

ADD_EXECUTABLE(LineStartTriggerLine
	LineStartTriggerLine.cpp)
TARGET_LINK_LIBRARIES(LineStartTriggerLine
	${FGLIB5_LIBRARIES}
	${SISODISPLAYLIB_LIBRARIES}
)

ADD_EXECUTABLE(LineTriggerGatedImg
	LineTriggerGatedImg.cpp)
TARGET_LINK_LIBRARIES(LineTriggerGatedImg
	${FGLIB5_LIBRARIES}
	${SISODISPLAYLIB_LIBRARIES}
)

ADD_EXECUTABLE(LineTriggerGatedMultiBufferImg
	LineTriggerGatedMultiBufferImg.cpp)
TARGET_LINK_LIBRARIES(LineTriggerGatedMultiBufferImg
	${FGLIB5_LIBRARIES}
	${SISODISPLAYLIB_LIBRARIES}
)

INSTALL(TARGETS AreaExternal
	AreaFreeRun
	AreaGrabberControl
	AreaSoftware
	LineFreeRunImg
	LineGatedAsyncTriggerLine
	LineGatedGrabberControlledLine
	LineGrabberControlledLine
	LineStartTriggerImg
	LineStartTriggerLine
	LineTriggerGatedImg
	LineTriggerGatedMultiBufferImg
	DESTINATION bin/examples)
