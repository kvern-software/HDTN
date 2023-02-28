cmake_minimum_required(VERSION 3.12)  # CMake version check

project(dtn_rtp_streaming)          
set(CMAKE_CXX_STANDARD 11)   
set(CMAKE_VERBOSE_MAKEFILE FALSE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall -Wformat -std=c++2a  -g") # -Werror

set(MIN_BOOST_VERSION 1.66.0)
find_package(Boost ${MIN_BOOST_VERSION} REQUIRED system date_time filesystem timer thread regex filesystem unit_test_framework program_options iostreams random log log_setup)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(ZLIB) #for compression
find_library(AVCODEC_LIBRARY avcodec)

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

# add_subdirectory(libsdptransform)
# add_subdirectory(uvgRTP)

set(DTN_SOURCES 
	src/dtn/DtnContext.cpp
	src/dtn/DtnEncoder.cpp
	src/dtn/DtnFrameQueue.cpp
	src/dtn/DtnMedia.cpp
	src/dtn/DtnMediaStream.cpp
	src/dtn/DtnSession.cpp
	src/dtn/DtnUtil.cpp
	src/dtn/DtnRtp.cpp
)

set(GUI_SOURCES 
	# gui library backend files
	MediaGui/include/imgui/imgui.cpp
	MediaGui/include/imgui/imgui_demo.cpp
	MediaGui/include/imgui/imgui_draw.cpp
	MediaGui/include/imgui/imgui_widgets.cpp
	MediaGui/include/imgui/imgui_tables.cpp
	MediaGui/include/imgui/backends/imgui_impl_glfw.cpp
	MediaGui/include/imgui/backends/imgui_impl_opengl3.cpp
)

##### SINK
add_executable(dtn_rtp_stream_sink   
	#gui
	MediaGui/MediaGui.cpp
	${GUI_SOURCES}

	src/video_driver/VideoDriver.cpp
	# dtn related items such as streams 
	${DTN_SOURCES}

	MediaSink/MediaSink.cpp
	MediaSink/MediaSinkMain.cpp
	MediaSink/MediaSinkRunner.cpp)

target_include_directories(dtn_rtp_stream_sink PUBLIC
	src/dtn/include
	src/video_driver
	MediaGui/
	MediaGui/include/imgui
	MediaGui/include/imgui/backends #gui
	MediaGui/include/stb #quick file loader
)
target_link_libraries(dtn_rtp_stream_sink
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
	${AVCODEC_LIBRARY}
) 

##### SOURCE
add_executable(dtn_rtp_stream_source   	
	# dtn related items such as streams 
	MediaSource/MediaSource.cpp
	MediaSource/MediaSourceMain.cpp
    MediaSource/MediaSourceRunner.cpp
	
	# dtn related items such as streams 
	${DTN_SOURCES}
	
	src/video_driver/VideoDriver.cpp
)
target_include_directories(dtn_rtp_stream_source PUBLIC
	src/dtn/include
	src/video_driver
)
target_link_libraries(dtn_rtp_stream_source   
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
	${AVCODEC_LIBRARY}
) 


##### STREAM
add_executable(bpsend_stream
	BpSendStream/src/BpSendStream.cpp
	BpSendStream/src/BpSendStreamMain.cpp
	BpSendStream/src/BpSendStreamRunner.cpp

	${DTN_SOURCES}
)
target_include_directories(bpsend_stream PUBLIC 
		BpSendStream/include 
		src/dtn/include
)
target_link_libraries(bpsend_stream   
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
	# ${AVCODEC_LIBRARY}
) 
