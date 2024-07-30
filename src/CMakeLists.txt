FILE(
    GLOB_RECURSE SRC_LIST 
    ./*.c
    ./*.cpp
)

# Exe output path
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR/bin})

ADD_EXECUTABLE(sensor_v4l2_demo ${SRC_LIST})

# Link lib and so
TARGET_LINK_LIBRARIES(
    sensor_v4l2_demo 
)