message ("----------------------------------------------------------")
message ("Build COSMOS/core")
#--------------------------------------------------------------------
# Windows/MSVC2013 notes:
# * Near the end of this script are some replacement lines to select the
#   MSVC static runtime. Turn USE_MSVC_STATIC_RUNTIME to OFF to
#   use the default "multithreaded DLL" style of RTL
# * For x64 linkage give the "-A x64" option to the CMAKE command
#--------------------------------------------------------------------
option(USE_MSVC_STATIC_RUNTIME "USE Static Runtime with MSVC" "OFF")

#--------------------------------------------------------------------
# user config variables
#--------------------------------------------------------------------

# define the project name
set(PROJECT_NAME cosmos-core)

# configure the COSMOS/core project here!
include(${CMAKE_SOURCE_DIR}/cmake/config.cmake)

# Type of build
set(CMAKE_BUILD_TYPE Debug) # Release or Debug

# Type of architecture (arm, ...)
add_definitions( -DCROSS_TYPE_${CROSS_TYPE})
message(STATUS "Architecture: ${CROSS_TYPE}")

# define folder where your programs are going to be installed
#set(INSTALL_DIR run)

# by default

# Enable the creating of compile_commands.json for use with code analysis tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#--------------------------------------------------------------------
# automatic config

# comment
#set(CMAKE_LEGACY_CYGWIN_WIN32 0) # Remove when CMake >= 2.8.4 is required
cmake_minimum_required(VERSION 3.1)
#cmake_minimum_required(VERSION 3.3) # if ( ... IN_LIST ...)

project(${PROJECT_NAME})

set(COSMOS_SOURCE ${CMAKE_SOURCE_DIR}/..)
#message ("COSMOS_SOURCE: ${COSMOS_SOURCE}")
set (COSMOS_CORE_SOURCE ${CMAKE_SOURCE_DIR})
#message ("COSMOS_CORE_SOURCE: ${COSMOS_CORE_SOURCE}")
#set(COSMOS_CORE ${CMAKE_SOURCE_DIR})

# define what libraries you want to compile
set(BUILD_COSMOS_CORE_AGENT                ON)          #ON/OFF
set(BUILD_COSMOS_CORE_PHYSICS              ON)          #ON/OFF
set(BUILD_COSMOS_CORE_MATH                 ON)          #ON/OFF
set(BUILD_COSMOS_CORE_MODULE               ON)          #ON/OFF
set(BUILD_COSMOS_CORE_SUPPORT              ON)          #ON/OFF
set(BUILD_COSMOS_CORE_DEVICES              ON)          #ON/OFF
set(BUILD_COSMOS_CORE_THIRDPARTY_JPEG      ON)          #ON/OFF
set(BUILD_COSMOS_CORE_THIRDPARTY_ZLIB      ON)          #ON/OFF
set(BUILD_COSMOS_CORE_THIRDPARTY_EIGEN     ON)          #ON/OFF
set(BUILD_COSMOS_PROJECT_USER              OFF)         #ON/OFF
set(BUILD_COSMOS_CORE_THIRDPARTY_PNG       ON)          #ON/OFF
if (MSVC)
    set(BUILD_COSMOS_CORE_THIRDPARTY_DIRENT     ON)          #ON/OFF
else()
    set(BUILD_COSMOS_CORE_THIRDPARTY_DIRENT     OFF)
endif()


#Note: Actual flags are COSMOS_CXX_FLAGS + COSMOS_CXX_FLAGS_RELEASE/DEBUG?
#set(COSMOS_CXX_FLAGS "-Wmaybe-uninitialized -Wtype-limits -Wall -pedantic -std=c++11 -U__STRICT_ANSI__ -Wno-invalid-offsetof -Wno-format")
set(COSMOS_CXX_FLAGS "-Wmaybe-uninitialized -Wtype-limits -Wall -pedantic -std=c++11 -Wno-invalid-offsetof -Wno-format")

# Force strict ANSI
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -U__STRICT_ANSI__")

find_package (Threads)

# Debugging?
if (CMAKE_BUILD_TYPE MATCHES "Debug")
    if (MSVC)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    else()
        if (${CROSS_TYPE} MATCHES "arm")
            set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -gdwarf-2")
        else()
            set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
        endif()
    endif()
    message("Build Settings: Debug ON")
endif()

#option(BUILD_GPROF "Build for Profiling" OFF)
if (BUILD_GPROF MATCHES ON)
    message("Build Settings: Profiling ON")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
endif ()

option(BUILD_STATIC "Build Static Executables" OFF)
if (BUILD_STATIC AND NOT MSVC)
    message("Build Settings: static executables ON")
    set(BUILD_SHARED_LIBS OFF)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++ -Wl,--no-as-needed")
endif ()


# Cmake macros, files to deploy, etc
include(${CMAKE_SOURCE_DIR}/cmake/macros.cmake)
#set(COSMOS_LIBS localzlib)
set(COSMOS_LIBS)

# -----------------------------------------------
# Linux specific
message(STATUS "System: ${CMAKE_SYSTEM_NAME} Cross: ${CROSS_TYPE}")
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux") # or just UNIX

    # Configure compiler arguments (GCC)
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        # Set Compiler flags
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COSMOS_CXX_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lm -lpthread")
        message("Build Compiler: GCC")


        set(COMPILER_IS_LINUX_GCC 1)

        # set the COSMOS_LIBS for GCC
        #TODO: remove redundancy libs, this can be resolved when Build the libs
        #set(COSMOS_LIBS  )

    endif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

    #  option(BUILD_ARM "Build for ARM architecture" OFF)
    if (${CROSS_TYPE} MATCHES "arm" OR ${CROSS_TYPE} MATCHES "iobc")
        if("$ENV{COSMOS}" STREQUAL "")
            set(COSMOS_INSTALL_FOLDER "~/cosmos/${CROSS_TYPE}")
        else()
            set(COSMOS_INSTALL_FOLDER "$ENV{COSMOS}/${CROSS_TYPE}")
        endif("$ENV{COSMOS}" STREQUAL "")

        set(CMAKE_TOOLCHAIN_file "${PROJECT_SOURCE_DIR}/build/toolchain_${CROSS_TYPE}_linux.cmake")
        message(STATUS "Cross compile: ${CROSS_TYPE}")

    else()
        if("$ENV{COSMOS}" STREQUAL "")
            set(COSMOS_INSTALL_FOLDER "~/cosmos") #was /usr/local/cosmos/linux
        else()
            set(COSMOS_INSTALL_FOLDER "$ENV{COSMOS}")
        endif("$ENV{COSMOS}" STREQUAL "")
    endif()

endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

# Generic specific
if(${CMAKE_SYSTEM_NAME} MATCHES "minix")

    # Configure compiler arguments (GCC)
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        # Set Compiler flags
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COSMOS_CXX_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lm -lpthread")
        message("Build Compiler: GCC")


        set(COMPILER_IS_LINUX_GCC 1)

        # set the COSMOS_LIBS for GCC
        #TODO: remove redundancy libs, this can be resolved when Build the libs
        #set(COSMOS_LIBS  )

    endif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

        if("$ENV{COSMOS}" STREQUAL "")
            set(COSMOS_INSTALL_FOLDER "~/cosmos/${CROSS_TYPE}")
        else()
            set(COSMOS_INSTALL_FOLDER "$ENV{COSMOS}/${CROSS_TYPE}")
        endif("$ENV{COSMOS}" STREQUAL "")

        set(CMAKE_TOOLCHAIN_file "${PROJECT_SOURCE_DIR}/build/toolchain_${CROSS_TYPE}_${CMAKE_SYSTEM_NAME}.cmake")

endif(${CMAKE_SYSTEM_NAME} MATCHES "minix")


# -----------------------------------------------
# Windows specific
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows") #WIN32

    # Configure compiler arguments GCC (MinGw)
    if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
        # Set Compiler flags
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COSMOS_CXX_FLAGS}")
        message("Build with G++ (MinGw)")
        set (COMPILER_IS_WINDOWS_MINGW 1)

        # set the COSMOS_LIBS for MinGW
        #TODO: remove redundancy libs, this can be resolved when Build the libs
        set(COSMOS_LIBS  wsock32 winmm ws2_32 iphlpapi)

    elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC") #or just MSVC
        message("Build with MSVC")

        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W4")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_NONSTDC_NO_DEPRECATE")

        set(COMPILER_IS_WINDOWS_MSVC 1)

        # set the COSMOS_LIBS for MSVC
        #TODO: remove redundancy libs, this can be resolved when Build the libs
        set(COSMOS_LIBS  dirent wsock32 winmm ws2_32 iphlpapi)

    endif()

    if("$ENV{COSMOS}" STREQUAL "")
        set(COSMOS_INSTALL_FOLDER "c:/cosmos") #was /usr/local/cosmos/linux
    else()
        set(COSMOS_INSTALL_FOLDER "$ENV{COSMOS}")
    endif("$ENV{COSMOS}" STREQUAL "")

    # if we are cross compiling then put the files inside the "crosscompile" folder
    if (${CROSS_TYPE} MATCHES "arm")
        message("Build for targe ${CROSS_TYPE}")
        set(COSMOS_INSTALL_FOLDER "c:/cosmos/${CROSS_TYPE}")
        set(CMAKE_TOOLCHAIN_file "${PROJECT_SOURCE_DIR}/cmake/toolchain_${CROSS_TYPE}_win")
    endif()

    # if we have set an environment variable COSMOS_INSTALL (for specific reasons, not default)
    if(NOT ("$ENV{COSMOS_INSTALL}" STREQUAL ""))
        set(COSMOS_INSTALL_FOLDER "$ENV{COSMOS_INSTALL}")
    endif()

    if(MSVC AND ((${USE_MSVC_STATIC_RUNTIME} MATCHES "ON")))
        message("Build MSVC Static")
        # Below foreach is to remove the /MD from the linker settings to use a static runtime link
        # referenced from https://www.mail-archive.com/cmake@cmake.org/msg02225.html
        foreach (MODE "_DEBUG" "_MINSIZEREL" "_RELEASE" "_RELWITHDEBINFO")
            STRING(REPLACE "/MD" "/MT" TMP "${CMAKE_C_FLAGS${MODE}}")
            set(CMAKE_C_FLAGS${MODE} "${TMP}" CACHE STRING "" FORCE)
            #message(STATUS "C_${MODE}=${CMAKE_C_FLAGS${MODE}}")
            STRING(REPLACE "/MD" "/MT" TMP "${CMAKE_CXX_FLAGS${MODE}}")
            set(CMAKE_CXX_FLAGS${MODE} "${TMP}" CACHE STRING "" FORCE)
            #message(STATUS "CXX_${MODE}=${CMAKE_CXX_FLAGS${MODE}}")
        endforeach (MODE)

        message("MSVC CXX Flags: ${CMAKE_CXX_FLAGS}")

    elseif(MSVC)
        message("MSVC CXX Flags: ${CMAKE_CXX_FLAGS}")

    endif()

endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

# -----------------------------------------------
# MacOS specific
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

    message("Build on Mac")
    message("Compiler is ${CMAKE_CXX_COMPILER_ID}")

    # Configure compiler arguments (GCC)
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        # Set Compiler flags
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COSMOS_CXX_FLAGS}")
#        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lm")
        message("Build with GCC")

        set(COMPILER_IS_MACOS_GCC 1)

        # set the COSMOS_LIBS for GCC
        #set(COSMOS_LIBS  )
    endif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

    # Configure compiler arguments (Clang)
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        # Set Compiler flags
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COSMOS_CXX_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lm")
        message("Build with Clang")

        set(COMPILER_IS_MACOS_CLANG 1)

        # set the COSMOS_LIBS for GCC
        #set(COSMOS_LIBS  )
    endif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")


    if("$ENV{COSMOS}" STREQUAL "")
        set(COSMOS_INSTALL_FOLDER "~/cosmos/${CROSS_TYPE}")
    else()
        set(COSMOS_INSTALL_FOLDER "$ENV{COSMOS}/macos")
    endif("$ENV{COSMOS}" STREQUAL "")

endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")


# -----------------------------------------------
# Extend COSMOS_LIBS with standard libraries
#message("COSMOS_INSTALL_FOLDER: ${COSMOS_INSTALL_FOLDER}")

set(CMAKE_INSTALL_PREFIX "${COSMOS_INSTALL_FOLDER}" CACHE PATH "default install path" FORCE )
#message("Install: ${CMAKE_INSTALL_PREFIX}")


#get_filename_component(OS_FOLDER ${CMAKE_BINARY_DIR} NAME)

# Establish includes, both standard and device specific
# Standard inludes
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libraries)


# -----------------------------------------------
# To load the COSMOS source into another project using cmake
# we cannot use the CMAKE_SOURCE_DIR by default because it will conflict
# with the new project CMAKE_SOURCE_DIR, let's use the COSMOS_CORE variable to
# properly define where the COSMOS_CORE files are

# Establish libraries, both standard and device specific
include(${CMAKE_SOURCE_DIR}/cmake/build_core_libs.cmake)

# print libraries to link
#message("COSMOS/core libraries: ${COSMOS_LIBS}")

# -----------------------------------------------
#message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
#message("CMAKE_EXE_LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS}")

# -----------------------------------------------
# Enable CTests in the root directory
if(${BUILD_PROGRAMS_TESTS} MATCHES "YES")
    enable_testing()
    message("Tests have been enabled.")
endif()

# -----------------------------------------------
# now that the libraries have been compiled let's add the cosmos-core programs
if(${BUILD_PROGRAMS} MATCHES "YES")
    add_subdirectory(${PROJECT_SOURCE_DIR}/programs)
endif()


# -----------------------------------------------
# now that the libraries have been compiled let's add the cosmos-core programs
if(${BUILD_TUTORIALS} MATCHES "YES")
    add_subdirectory(${PROJECT_SOURCE_DIR}/tutorials)
endif()


# -----------------------------------------------
# to deploy on a remote device
#include(${CMAKE_SOURCE_DIR}/cmake/deploy_files.cmake)

# add a target to generate API documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
add_custom_target(doc
${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
COMMENT "Generating API documentation with Doxygen" VERBATIM
)
endif(DOXYGEN_FOUND)

# Download and unpack googletest at configure time
configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
          message(FATAL_ERROR "CMake step for googletest failed: ${result}")
          endif()
          execute_process(COMMAND ${CMAKE_COMMAND} --build .
            RESULT_VARIABLE result
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
          if(result)
                    message(FATAL_ERROR "Build step for googletest failed: ${result}")
                        endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
                 ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
                                                  EXCLUDE_FROM_ALL)

# The gtest/gtest_main targets carry header search path
# dependencies automatically when using CMake 2.8.11 or
# later. Otherwise we have to add them here ourselves.
if (CMAKE_VERSION VERSION_LESS 2.8.11)
	  include_directories("${gtest_SOURCE_DIR}/include")
	  endif()

# Now simply link against gtest or gtest_main as needed. Eg
#add_executable(agent_001 agent_001.cpp)
#target_link_libraries(agent_001 gtest_main)
#add_test(NAME example_test COMMAND example)
