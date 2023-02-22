
cmake_minimum_required(VERSION 3.16)
project(stream_tools)
set(CMAKE_CXX_STANDARD 17)

find_library(AVCODEC_LIBRARY avcodec)


add_executable(camera_encoder encode.cpp)
add_executable(decoder decode.cpp)



target_include_directories( camera_encoder PRIVATE ${AVCODEC_INCLUDE_DIR} )
target_link_libraries(camera_encoder  ${AVCODEC_LIBRARY} -lavcodec -lavfilter -lavformat -lavutil -lavdevice )

target_include_directories( decoder PRIVATE ${AVCODEC_INCLUDE_DIR} )
target_link_libraries(decoder  ${AVCODEC_LIBRARY} -lavcodec -lavfilter -lavformat -lavutil -lavdevice -lswscale )