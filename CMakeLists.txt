# 最小版本，可以通过cmake --version查看
cmake_minimum_required(VERSION 3.16.3)

# 项目名称，建议大写
project(OPENCV)

find_package(OpenCV)
# 搜索自定义的头文件目录，可以使用绝对路径 ${CMAKE_SOURCE_DIR}/include
include_directories(include)
include_directories(${OpenCV_INCLUDES)

# 设置编译时的选项，如O2优化，开启警告,使用的C++标准等  此处为SSE avx2
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")

# 调试时使用该项
set(CMAKE_BUILD_TYPE Debug)

# SSE
# set(CMAKE_CXX_FLAGS “${CMAKE_CXX_FLAGS} -msse3”)

# 生成可执行文件命令，第一个参数是生成可执行文件的名称，后面的参数是源文件代码
add_executable(main main.cpp ./src/opencv.cpp ./src/myconvert.cpp ./src/lut.cpp ./src/mysse.cpp)

# 链接到库
target_link_libraries(main ${OpenCV_LIBS})

# 指定生成可执行文件的输出目录
set(EXECUTABLE_OUTPUT_PATH "../out")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_USE_CXX11_ABI=0")

