message(STATUS "${CMAKE_CURRENT_SOURCE_DIR} : ")

# Mimicking the libraries specified in the CMakeLists of the support folder
add_subdirectory(slip)
add_subdirectory(transfer)
add_subdirectory(print)
add_subdirectory(log)
add_subdirectory(convert)
add_subdirectory(network)
add_subdirectory(namespace)
add_subdirectory(string)
add_subdirectory(json)
add_subdirectory(time)
add_subdirectory(data)
add_subdirectory(packethandler)
add_subdirectory(channel)
add_subdirectory(beacon)
add_subdirectory(packet)