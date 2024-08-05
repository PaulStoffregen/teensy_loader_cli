# Copyright (C) 2024 Michael Fabian 'Xaymar' Dirks
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# We require 3.30 for now, as there's zero reasons for you to be using older versions.
cmake_minimum_required(VERSION 3.30...4.0)
project(
	teensy_loader_cli
	DESCRIPTION "Command Line Interface for flashing Teensy boards."
	HOMEPAGE_URL https://github.com/Xaymar/teensy_loader_cli
	LANGUAGES C CXX ASM
)
list(APPEND CMAKE_MESSAGE_INDENT "[${PROJECT_NAME}]" " ")

################################################################################
# Modules
################################################################################

# Search Paths
set(CMAKE_MODULE_PATH
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake"
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/clang"
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/version"
)

# Include required Modules
include("CheckIPOSupported")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/version/version.cmake")
	include("version")
	set(HAVE_VERSION_CMAKE ON)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/clang/Clang.cmake")
	include("Clang")
	set(HAVE_CLANG_CMAKE ON)
endif()

################################################################################
# Versioning
################################################################################
if(HAVE_VERSION_CMAKE)
	version(GENERATE _VERSION COMPRESSED MAJOR 0 MINOR 0 PATCH 0 TWEAK 0 REQUIRE "PATCH;")
	version(PARSE _VERSION "${_VERSION}" REQUIRE "PATCH;TWEAK")

	# If possible, automatically generate versions from git.
	if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/.git")
		find_program(GIT
			NAMES
				git
				git.exe
		)

		if(EXISTS "${GIT}")
			# Try and calculate the exist version using git.
			execute_process(COMMAND "${GIT}" describe --tags --long --abbrev=8 "--match=[0-9].[0-9].[0-9]" HEAD WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR} RESULT_VARIABLE GIT_RESULT OUTPUT_VARIABLE GIT_OUTPUT ERROR_VARIABLE GIT_ERROR OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
			if((GIT_RESULT EQUAL 0) AND (GIT_OUTPUT MATCHES "([0-9]+)\.([0-9]+)\.([0-9]+)(.*)"))
				# Result will be MAJOR.MINOR.PATCH-TWEAK-gHASH
				string(REPLACE "-" ";" GIT_OUTPUT "${GIT_OUTPUT}")
				string(REPLACE "." ";" GIT_OUTPUT "${GIT_OUTPUT}")

				# Split into components
				list(GET GIT_OUTPUT 0 GIT_OUTPUT_MAJOR)
				list(GET GIT_OUTPUT 1 GIT_OUTPUT_MINOR)
				list(GET GIT_OUTPUT 2 GIT_OUTPUT_PATCH)
				list(GET GIT_OUTPUT 3 GIT_OUTPUT_TWEAK)
				list(GET GIT_OUTPUT 4 GIT_OUTPUT_BUILD)

				# Special case: Tag contains prerelease
				if(GIT_OUTPUT_PATCH MATCHES "([0-9]+)([a-zA-Z]+)([0-9]*)")
					# Patch requires special parsing.
					set(GIT_OUTPUT_PATCH "${CMAKE_MATCH_1}")
					if(CMAKE_MATCH_3 GREATER 0)
						set(GIT_OUTPUT_PRERELEASE "${CMAKE_MATCH_2}")
					else()
						set(GIT_OUTPUT_PRERELEASE "a")
					endif()
					MATH(EXPR GIT_OUTPUT_TWEAK "${GIT_OUTPUT_TWEAK} + ${CMAKE_MATCH_3}")

					# Modify the global version
					version(MODIFY _VERSION "${_VERSION}" COMPRESS
						MAJOR "${GIT_OUTPUT_MAJOR}"
						MINOR "${GIT_OUTPUT_MINOR}"
						PATCH "${GIT_OUTPUT_PATCH}"
						TWEAK "${GIT_OUTPUT_TWEAK}"
						BUILD "${GIT_OUTPUT_BUILD}"
						PRERELEASE "${GIT_OUTPUT_PRERELEASE}"
						REQUIRE "PATCH;TWEAK"
					)
				else()
					# Modify the global version
					version(MODIFY _VERSION "${_VERSION}" COMPRESS
						MAJOR "${GIT_OUTPUT_MAJOR}"
						MINOR "${GIT_OUTPUT_MINOR}"
						PATCH "${GIT_OUTPUT_PATCH}"
						TWEAK "${GIT_OUTPUT_TWEAK}"
						BUILD "${GIT_OUTPUT_BUILD}"
						PRERELEASE "a"
						REQUIRE "PATCH;TWEAK"
					)
				endif()
			else()
				execute_process(COMMAND "${GIT}" rev-list --count HEAD WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR} RESULT_VARIABLE GIT_RESULT OUTPUT_VARIABLE GIT_OUTPUT ERROR_VARIABLE GIT_ERROR OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
				if((GIT_RESULT EQUAL 0) AND (NOT "${GIT_OUTPUT}" STREQUAL ""))
					version(MODIFY _VERSION "${_VERSION}" COMPRESS
						TWEAK "${GIT_OUTPUT}"
						PRERELEASE "a"
						REQUIRE "PATCH;TWEAK"
					)

					# Determine the build using git.
					execute_process(COMMAND "${GIT}" log -1 "--pretty=format:g%h" --abbrev=8 HEAD WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR} RESULT_VARIABLE GIT_RESULT OUTPUT_VARIABLE GIT_OUTPUT ERROR_VARIABLE GIT_ERROR OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
					if((GIT_RESULT EQUAL 0) AND (NOT "${GIT_OUTPUT}" STREQUAL ""))
						version(MODIFY _VERSION "${_VERSION}" COMPRESS
							BUILD "${GIT_OUTPUT}"
							REQUIRE "PATCH;TWEAK"
						)
					else()
						message(WARNING "Failed to detect build version with 'git'.")
					endif()
				else()
					message(WARNING "Failed to automatically detect version with 'git'.")
				endif()
			endif()
		else()
			message(WARNING "'git' not found, automatic version detection disabled.")
		endif()
	else()
		message(STATUS "Not a git repository, automatic version detection disabled.")
	endif()

	# Allow manual overrides of the detected version.
	if(${PREFIX}VERSION)
		version(PARSE _VERSION_CFG "${${PREFIX}VERSION}" REQUIRE "PATCH;TWEAK")
		if("${_VERSION_CFG_BUILD}" STREQUAL "")
			set(_VERSION_CFG_BUILD "${_VERSION_BUILD}")
		endif()
		version(GENERATE _VERSION COMPRESS
			MAJOR "${_VERSION_CFG_MAJOR}"
			MINOR "${_VERSION_CFG_MINOR}"
			PATCH "${_VERSION_CFG_PATCH}"
			TWEAK "${_VERSION_CFG_TWEAK}"
			PRERELEASE "${_VERSION_CFG_PRERELEASE}"
			BUILD "${_VERSION_CFG_BUILD}"
		)
	endif()

	# Fix up missing parts of the Version
	version(PARSE _VERSION "${_VERSION}" REQUIRE "PATCH;TWEAK")

	set(_VERSION_THIN "${_VERSION_MAJOR}.${_VERSION_MINOR}.${_VERSION_PATCH}")
	if(NOT (_VERSION_PRERELEASE STREQUAL ""))
		set(_VERSION_THIN "${_VERSION_THIN}${_VERSION_PRERELEASE}${_VERSION_TWEAK}")
	endif()
	if(NOT (VERSION_COMMIT STREQUAL ""))
		set(_VERSION_THIN "${_VERSION_THIN}-${_VERSION_BUILD}")
	endif()

	# Metadata
	version(GENERATE PROJECT_VERSION
		MAJOR "${_VERSION_MAJOR}"
		MINOR "${_VERSION_MINOR}"
		PATCH "${_VERSION_PATCH}"
		TWEAK "${_VERSION_TWEAK}"
		REQUIRE "PATCH;TWEAK"
	)
else()
	set(PROJECT_VERSION "0.0.0")
	set(_VERSION_THIN "Development")
endif()

################################################################################
# Options
################################################################################
if (APPLE)
	set(ENABLE_LIBUSB OFF CACHE BOOL "Enable libUSB support.")
elseif(LINUX OR (BSD STREQUAL "FreeBSD"))
	set(ENABLE_LIBUSB ON CACHE BOOL "Enable libUSB support.")
endif()

################################################################################
# Project
################################################################################
project(${PROJECT_NAME}
	LANGUAGES C CXX ASM
	VERSION ${PROJECT_VERSION}
)
message(STATUS "Version ${_VERSION_THIN}")
file(WRITE "${PROJECT_BINARY_DIR}/version" "${_VERSION_THIN}")

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE
	"source/main.c"
	"source/ihex.h"
	"source/ihex.c"
	"source/misc.h"
	"source/misc.c"
	"source/dev.h"
	"source/dev-win32.c"
	"source/dev-libusb.c"
	"source/dev-iokit.c"
	"source/dev-uhid.c"
)
set_source_files_properties(
	"source/dev-win32.c"
	"source/dev-libusb.c"
	"source/dev-iokit.c"
	"source/dev-uhid.c"
	PROPERTIES
		HEADER_FILE_ONLY ON
)
target_include_directories(${PROJECT_NAME} PRIVATE
	"source"
)
if(WIN32)
	# Windows
	target_link_libraries(${PROJECT_NAME} PRIVATE
		hid
		setupapi
		winmm
	)
	set_source_files_properties(
		"source/dev-win32.c"
		PROPERTIES
			HEADER_FILE_ONLY OFF
	)
	target_compile_definitions(${PROJECT_NAME} PRIVATE
		USE_WIN32
	)
elseif(ENABLE_LIBUSB)
	# libUSB: Generic Linux, FreeBSD
	target_link_libraries(${PROJECT_NAME} PRIVATE
		usb
	)
	set_source_files_properties(
		"source/dev-libusb.c"
		PROPERTIES
			HEADER_FILE_ONLY OFF
	)
	target_compile_definitions(${PROJECT_NAME} PRIVATE
		USE_LIBUSB
	)
elseif(APPLE)
	# MacOSX
	find_library(IOKit_Path IOKit)
	find_library(CoreFoundation_Path CoreFoundation)
	target_link_libraries(${PROJECT_NAME} PRIVATE
		"${IOKit_Path}"
		"${CoreFoundation_Path}"
	)
	set_source_files_properties(
		"source/dev-iokit.c"
		PROPERTIES
			HEADER_FILE_ONLY OFF
	)
	target_compile_definitions(${PROJECT_NAME} PRIVATE
		USE_APPLE_IOKIT
	)
elseif(BSD)
	# OpenBSD, NetBSD, ...
	set_source_files_properties(
		"source/dev-uhid.c"
		PROPERTIES
			HEADER_FILE_ONLY OFF
	)
	target_compile_definitions(${PROJECT_NAME} PRIVATE
		USE_UHID
	)
else()
	message(WARNING "Unrecognized Operating System, so not linking anything.")
endif()
install(TARGETS ${PROJECT_NAME} DESTINATION "")

if(HAVE_CLANG_CMAKE)
	# Always do this last, it's order dependent unfortunately.
	generate_compile_commands_json(TARGETS ${PROJECT_NAME})
	clang_tidy(TARGETS ${PROJECT_NAME} VERSION 15.0.0)
	clang_format(TARGETS ${PROJECT_NAME} VERSION 15.0.0 DEPENDENCY GLOBAL)
endif()
