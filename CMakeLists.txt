cmake_minimum_required( VERSION 2.8 )
project( icp )

find_package( OpenCV REQUIRED ) 
find_package( glfw3 3.3 REQUIRED )
find_package( OpenGL REQUIRED )
find_package( GLEW REQUIRED )

include_directories( ${OpenCV_INCLUDE_DIRS} )
include_directories( ${GLEW_INCLUDE_DIRS} )
# include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/irrKlang/bin/linux-gcc-64 )

file( GLOB SOURCES
    *.h
    *.hpp
    *.cpp
    imgui/*.h
    imgui/*.cpp
    irrKlang/include/*.h
)

add_executable( icp ${SOURCES} )

target_link_libraries( icp ${OpenCV_LIBS} )
target_link_libraries( icp glfw )
target_link_libraries( icp OpenGL::GL )
target_link_libraries( icp ${GLEW_LIBRARIES} )
target_link_libraries( icp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/irrKlang/bin/linux-gcc-64/libIrrKlang.so )
