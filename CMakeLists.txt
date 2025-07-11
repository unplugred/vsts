cmake_minimum_required(VERSION 3.21)
project(UnplugRedsVSTTrove)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
if(APPLE)
	set (CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment version" FORCE)
	set (CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE INTERNAL "")
endif()

include(FetchContent)
Set(FETCHCONTENT_QUIET FALSE)
FetchContent_Declare(
	JUCE
	GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
	GIT_TAG 7.0.12
	GIT_PROGRESS TRUE
)
FetchContent_Declare(
	clap-juce-extensions
	GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
	GIT_TAG main
)
FetchContent_MakeAvailable(JUCE clap-juce-extensions)

set_property(GLOBAL PROPERTY USE_FOLDERS YES)
option(JUCE_BUILD_EXTRAS "Build extras" OFF)
option(JUCE_BUILD_EXAMPLES "Build examples" OFF)
option(JUCE_ENABLE_MODULE_SOURCE_GROUPS "Enable module source groups" ON)
option(JUCE_COPY_PLUGIN_AFTER_BUILD "Copy plugin after build" ON)

if(BANNERTYPE EQUAL 2)
	if(APPLE)
		set(outdir "../../../setup/build_mac/paid")
		set(freedir "../../../setup/build_mac/free")
	else()
		if(UNIX)
			set(outdir "../../../setup/build_linux/paid")
			set(freedir "../../../setup/build_linux/free")
		else()
			set(outdir "../../../setup/build_win/paid")
			set(freedir "../../../setup/build_win/free")
		endif()
	endif()
else()
	add_compile_definitions(BANNER)
	if(BANNERTYPE EQUAL 1)
		add_compile_definitions(BETA)
	endif()

	if(APPLE)
		set(outdir "../../../setup/build_mac/free")
		set(freedir "../../../setup/build_mac/free")
	else()
		if(UNIX)
			set(outdir "../../../setup/build_linux/free")
			set(freedir "../../../setup/build_linux/free")
		else()
			set(outdir "../../../setup/build_win/free")
			set(freedir "../../../setup/build_win/free")
		endif()
	endif()
endif()

add_subdirectory(modules)
add_subdirectory(plugins)
