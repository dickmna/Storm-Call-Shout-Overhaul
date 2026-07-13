cmake_minimum_required(VERSION 3.24)

project(
	SCSOProjectileBounds
	VERSION 0.1.0
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SKSE_SUPPORT_XBYAK OFF CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_SE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_AE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_VR OFF CACHE BOOL "" FORCE)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/CommonLibSSE-NG/CMakeLists.txt")
	message(FATAL_ERROR "CommonLibSSE-NG submodule is missing. Run: git submodule update --init --recursive")
endif()

add_subdirectory("external/CommonLibSSE-NG" CommonLibSSE-NG EXCLUDE_FROM_ALL)

add_library(${PROJECT_NAME} SHARED
	src/skse/main.cpp
)

target_compile_definitions(${PROJECT_NAME}
	PRIVATE
		WIN32_LEAN_AND_MEAN
		NOMINMAX
)

target_link_libraries(${PROJECT_NAME}
	PRIVATE
		CommonLibSSE::CommonLibSSE
		delayimp.lib
)

target_link_options(${PROJECT_NAME}
	PRIVATE
		"/DELAYLOAD:fmt.dll"
		"/DELAYLOAD:spdlog.dll"
)

target_precompile_headers(${PROJECT_NAME}
	PRIVATE
		<RE/Skyrim.h>
		<SKSE/SKSE.h>
)

set_target_properties(${PROJECT_NAME}
	PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Data/SKSE/Plugins"
		LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Data/SKSE/Plugins"
		ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

foreach(CONFIG_TYPE DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
	set_target_properties(${PROJECT_NAME}
		PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE} "${CMAKE_BINARY_DIR}/Data/SKSE/Plugins"
			LIBRARY_OUTPUT_DIRECTORY_${CONFIG_TYPE} "${CMAKE_BINARY_DIR}/Data/SKSE/Plugins"
			ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_TYPE} "${CMAKE_BINARY_DIR}/lib"
	)
endforeach()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${CMAKE_CURRENT_SOURCE_DIR}/package/SKSE/Plugins/SCSOProjectileBounds.ini"
		"$<TARGET_FILE_DIR:${PROJECT_NAME}>/SCSOProjectileBounds.ini"
)
