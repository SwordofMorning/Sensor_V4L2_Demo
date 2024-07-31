PROJECT(Sensor_V4L2_Demo)

CMAKE_MINIMUM_REQUIRED(VERSION 3.5)

SET(COMPILER_PATH "/home/xjt/_Workspace_/RV1126/RV1126_RV1109_LINUX_SDK_V2.2.5.1_20230530/buildroot/output/rockchip_rv1126_rv1109/host/bin/")

SET(CMAKE_C_COMPILER ${COMPILER_PATH}arm-linux-gnueabihf-gcc)
SET(CMAKE_CXX_COMPILER ${COMPILER_PATH}arm-linux-gnueabihf-g++)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s -O3 -lrt -fopenmp")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s -O3 -lrt -fopenmp")

ADD_SUBDIRECTORY(src bin)