#cmake_minimum_required(VERSION 3.5)

include_directories(${COSMOS_CORE_SOURCE}/libraries/thirdparty/zlib)

FILE(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
FILE(GLOB INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_library(localpng STATIC ${SOURCES} ${INCLUDES})

install(TARGETS localpng DESTINATION lib)
install(FILES ${INCLUDES} DESTINATION include/thirdparty/png)


if(${DEPLOY_SRC_FILES_TO_REMOTE} MATCHES "YES")
    add_deployment_source_directory(${CMAKE_CURRENT_SOURCE_DIR} "src")
endif()
