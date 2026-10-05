cmake_minimum_required(VERSION 3.24)

project(
	SCSOProjectileBounds
	VERSION 2.1.2
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SKSE_SUPPORT_XBYAK OFF CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_SE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_AE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_VR OFF CACHE BOOL "" FORCE)

# Dependencies restored from the corresponding published release source.
set(COMMONLIBSSE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/CommonLibSSE-NG-10.0.1" CACHE PATH "Matching CommonLibSSE-NG source")
set(COMMONLIBSSE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/build/commonlib-installed" CACHE PATH "Matching source-built dependencies")
list(PREPEND CMAKE_PREFIX_PATH "${COMMONLIBSSE_INSTALL_PREFIX}")

find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_path(RAPIDCSV_INCLUDE_DIR rapidcsv.h REQUIRED)
find_package(directxtk CONFIG REQUIRED)
find_package(CommonLibSSE 10.0.1 CONFIG REQUIRED)

add_library(${PROJECT_NAME} SHARED
	src/skse/main.cpp
)

target_compile_definitions(${PROJECT_NAME}
	PRIVATE
		WIN32_LEAN_AND_MEAN
		NOMINMAX
)
target_include_directories(${PROJECT_NAME} PRIVATE ${RAPIDCSV_INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME}
	PRIVATE
		CommonLibSSE::CommonLibSSE
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
