cmake_minimum_required(VERSION 3.21)
project(UnplugRedsVSTTrove)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
if(APPLE)
	set (CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version" FORCE)
	set (CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE INTERNAL "")
endif()

include(FetchContent)
Set(FETCHCONTENT_QUIET FALSE)
FetchContent_Declare(
	JUCE
	GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
	GIT_TAG master
	GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(JUCE)
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

if (BANNERTYPE GREATER 0)
	add_compile_definitions(BANNER)
	if (BANNERTYPE GREATER 1)
		add_compile_definitions(BETA)
	endif()

	if(APPLE)
		set(outdir "../../../Setup/build_mac/free")
		set(freedir "../../../Setup/build_mac/free")
	else()
		if(UNIX)
			set(outdir "../../../Setup/build_linux/free")
			set(freedir "../../../Setup/build_linux/free")
		else()
			set(outdir "../../../Setup/build_windows/free")
			set(freedir "../../../Setup/build_windows/free")
		endif()
	endif()
else()
	if(APPLE)
		set(outdir "../../../Setup/build_mac/paid")
		set(freedir "../../../Setup/build_mac/free")
	else()
		if(UNIX)
			set(outdir "../../../Setup/build_linux/paid")
			set(freedir "../../../Setup/build_linux/free")
		else()
			set(outdir "../../../Setup/build_windows/paid")
			set(freedir "../../../Setup/build_windows/free")
		endif()
	endif()
endif()

add_subdirectory(Modules)
add_subdirectory(Plugins)