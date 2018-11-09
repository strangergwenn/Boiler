cmake_minimum_required (VERSION 3.0)
project (Boiler)

# Setup a target
macro(setup_executable TARGET FILES)

	# Create executable
	add_executable(${TARGET} ${FILES})
	set_property(TARGET ${TARGET} PROPERTY CXX_STANDARD 11)
	set_property(TARGET ${TARGET} PROPERTY CXX_STANDARD_REQUIRED ON)
	
	# C++11 trick for older CMake
	if (NOT WIN32)
		set(CMAKE_CXX_FLAGS "-std=c++11")
	endif()

	# Add includes
	target_include_directories(${TARGET} PUBLIC sources)
	target_include_directories(${TARGET} PUBLIC external/jsoncpp)
	
	# Link with sockets, threads
	if(WIN32)
  		target_link_libraries(${TARGET} wsock32 ws2_32)
	else()
  		target_link_libraries(${TARGET} pthread)
	endif()

	# Make it a console application for easier debugging
	if(WIN32)
		set_target_properties(${TARGET} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
		set_target_properties(${TARGET} PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
		set_target_properties(${TARGET} PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
		set_target_properties(${TARGET} PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "_CONSOLE")
		set_target_properties(${TARGET} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
		set_target_properties(${TARGET} PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")
	endif(WIN32)

endmacro()

# JSON files
set (JSON_FILES
	external/jsoncpp/json/json.h
	external/jsoncpp/json/json-forwards.h
	external/jsoncpp/jsoncpp.cpp
)

# General files
set (MAIN_FILES
	sources/inputparams.h
	sources/main.cpp
)

# Organize solution
source_group("JSON" FILES ${JSON_FILES})
source_group("Main" FILES ${MAIN_FILES})

# Test files
set (CLIENT_PROJECT_FILES
	${JSON_FILES}
	${MAIN_FILES}
)
setup_executable(Boiler "${CLIENT_PROJECT_FILES}")
