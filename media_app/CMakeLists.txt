cmake_minimum_required(VERSION 3.12) #HDTN min version
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_VERBOSE_MAKEFILE FALSE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall -Wformat -std=c++11 -Werror -g")
set(CMAKE_BUILD_TYPE Debug)

project(media_app)


set(MIN_BOOST_VERSION 1.66.0)
find_package(Boost ${MIN_BOOST_VERSION} REQUIRED system date_time filesystem timer thread regex filesystem unit_test_framework program_options iostreams random log log_setup)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

SET(CMAKE_PREFIX_PATH $ENV{HDTN_INSTALL_ROOT} CACHE PATH "Path for find_package to import installed hdtn libraries")

#HDTN - not sure which of these I need yet, just most all of them for now
find_package(HDTNUtil REQUIRED)
find_package(StatsLoggerLib REQUIRED)
find_package(Bpcodec REQUIRED)
find_package(TelemetryDefinitions REQUIRED)
find_package(LtpLib REQUIRED)
find_package(BpAppPatternsLib REQUIRED)
find_package(EgressLib REQUIRED)
find_package(HDTNConfigLib REQUIRED)
find_package(HDTNCgrLib REQUIRED)
find_package(InductManagerLib REQUIRED)
find_package(IngressLib REQUIRED)
find_package(LoggerLib REQUIRED)
find_package(OutductManagerLib REQUIRED)
find_package(StcpLib REQUIRED)
find_package(StorageLib REQUIRED)
find_package(TcpclLib REQUIRED)
find_package(UdpLib REQUIRED)
find_package(UdpDelaySimLib REQUIRED)
find_package(Boost "1.67.0" REQUIRED timer unit_test_framework)
 
find_package(ZLIB) #for compression

add_executable(media_sink
    src/VideoDriver.cpp
	src/MediaApp.cpp
	src/Fragmenter.cpp
	src/Compressor.cpp
	
	MediaSink/src/MediaSinkMain.cpp
	MediaSink/src/MediaSinkRunner.cpp
	MediaSink/src/MediaSink.cpp
	
	# gui library backend files
	include/imgui/imgui.cpp
    include/imgui/imgui_demo.cpp
    include/imgui/imgui_draw.cpp
    include/imgui/imgui_widgets.cpp
    include/imgui/imgui_tables.cpp
    include/imgui/backends/imgui_impl_glfw.cpp
    include/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(media_sink
	PUBLIC 
		
	include/ #user code
	MediaSink/include/
	include/imgui #gui 
	include/imgui/backends #gui
	include/stb #quick file loader
)

target_link_libraries(media_sink
	#gui
  	glfw 
	OpenGL::GL 
	${CMAKE_DL_LIBS}
    
	#HDTN
	HDTN::HDTNUtil
	HDTN::Bpcodec
	HDTN::TelemetryDefinitions
	HDTN::LtpLib
	HDTN::BpAppPatternsLib
	HDTN::EgressLib
	HDTN::HDTNConfigLib
	HDTN::HDTNCgrLib
	HDTN::InductManagerLib
	HDTN::IngressLib
	HDTN::LoggerLib
	HDTN::OutductManagerLib
	HDTN::StcpLib
	HDTN::StorageLib
	HDTN::TcpclLib
	HDTN::UdpLib
	HDTN::UdpDelaySimLib

	Boost::unit_test_framework
	Boost::timer
    Boost::program_options 
	Boost::thread
	Boost::log

	ZLIB::ZLIB
) 


add_executable(media_source 
	src/VideoDriver.cpp
	src/MediaApp.cpp
	src/Fragmenter.cpp
	src/Compressor.cpp
	
	MediaSource/src/MediaSourceMain.cpp
	MediaSource/src/MediaSourceRunner.cpp
	MediaSource/src/MediaSource.cpp
	
	# gui library backend files
	include/imgui/imgui.cpp
    include/imgui/imgui_demo.cpp
    include/imgui/imgui_draw.cpp
    include/imgui/imgui_widgets.cpp
    include/imgui/imgui_tables.cpp
    include/imgui/backends/imgui_impl_glfw.cpp
    include/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(media_source
	PUBLIC 
		
	include/ #user code
	MediaSource/include/
	include/imgui #gui 
	include/imgui/backends #gui
	include/stb #quick file loader
)

target_link_libraries(media_source
	#gui
  	glfw 
	OpenGL::GL 
	${CMAKE_DL_LIBS}
    
	#HDTN
	HDTN::HDTNUtil
	HDTN::TelemetryDefinitions
	HDTN::LtpLib
	HDTN::BpAppPatternsLib
	HDTN::Bpcodec
	HDTN::EgressLib
	HDTN::HDTNConfigLib
	HDTN::HDTNCgrLib
	HDTN::InductManagerLib
	HDTN::IngressLib
	HDTN::LoggerLib
	HDTN::OutductManagerLib
	HDTN::StcpLib
	HDTN::StorageLib
	HDTN::TcpclLib
	HDTN::UdpLib
	HDTN::UdpDelaySimLib

	Boost::unit_test_framework
	Boost::timer
    Boost::program_options 
	Boost::thread
	Boost::log

	ZLIB::ZLIB
) 