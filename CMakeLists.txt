cmake_minimum_required (VERSION 3.0)
project (Boiler)

# Setup Steam SDK path
set(STEAM_SDK_PATH "${CMAKE_SOURCE_DIR}/external/sdk")
set(STEAM_LIBRARY_PATH "${STEAM_SDK_PATH}/redistributable_bin/win64")

# JSON files
set (JSON_FILES
	"external/jsoncpp/json/json.h"
	"external/jsoncpp/json/json-forwards.h"
	"external/jsoncpp/jsoncpp.cpp"
)

# Boiler library
set (BOILER_FILES
	"sources/boiler/boiler.h"
	"sources/boiler/boiler.cpp"
)

# General files
set (MAIN_FILES
	"sources/inputparams.h"
	"sources/main.cpp"
)

# Organize solution
source_group("JSON" FILES ${JSON_FILES})
source_group("Boiler" FILES ${BOILER_FILES})
source_group("Main" FILES ${MAIN_FILES})

# Add sources
set (CLIENT_PROJECT_FILES
	${JSON_FILES}
	${BOILER_FILES}
	${MAIN_FILES}
)

macro (setup_executable TARGET FILES)

	# Create executable
	add_executable(${TARGET} ${CLIENT_PROJECT_FILES})
	set_property(TARGET ${TARGET} PROPERTY CXX_STANDARD 17)
	set_property(TARGET ${TARGET} PROPERTY CXX_STANDARD_REQUIRED ON)
	set_target_properties(${TARGET} PROPERTIES LINKER_LANGUAGE CXX)

	# Additional definitions
	set(CMAKE_CXX_FLAGS_RELEASE "/MT")
	set(CMAKE_CXX_FLAGS_DEBUG "/MTd")
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	
	# Add includes
	target_include_directories(${TARGET} PUBLIC "sources")
	target_include_directories(${TARGET} PUBLIC "external/jsoncpp")
	target_include_directories(${TARGET} PUBLIC "${STEAM_SDK_PATH}/public")

	# Add Steam library link
	target_link_libraries(${TARGET} PUBLIC "${STEAM_LIBRARY_PATH}/steam_api64.lib")

	# Add Steam DLL copy
	add_custom_command(TARGET ${TARGET} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${STEAM_LIBRARY_PATH}/steam_api64.dll"
		"$<TARGET_FILE_DIR:Boiler>")
	
endmacro()

# Add executables
setup_executable(Boiler ${CLIENT_PROJECT_FILES})
setup_executable(HeliumRainLauncher ${CLIENT_PROJECT_FILES})

# Console application
if (WIN32)
	set_target_properties(HeliumRainLauncher PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS")
	set_target_properties(HeliumRainLauncher PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS")
	set_target_properties(HeliumRainLauncher PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
	set_target_properties(HeliumRainLauncher PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")
endif()
