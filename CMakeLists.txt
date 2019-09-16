cmake_minimum_required(VERSION 3.13)
project(deal-module)

# Settings
# ----------------------------------------------------------------------------------------------------------------------
set(INSTALL_AS_ROOT OFF CACHE BOOL "Install as root")
# ----------------------------------------------------------------------------------------------------------------------

if (INSTALL_AS_ROOT)
    set(INSTALL_BIN_PATH "/usr/sbin")
    set(PROJECT_PREFIX "/etc/${PROJECT_NAME}")
else()
    set(INSTALL_BIN_PATH "/usr/bin")
    set(PROJECT_PREFIX "$ENV{HOME}/.${PROJECT_NAME}")
endif()

list( APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake-modules" )

add_compile_options("$<$<CONFIG:DEBUG>:-D_DEBUG>")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")

set(PROJECT_NAME "bps-dm")
set(PROJECT_DESCRIPTION "Bitcoin Payment Service (Deal Module)")

message(STATUS "Project name: ${PROJECT_NAME}")
message(STATUS "Project description: ${PROJECT_DESCRIPTION}")

set(CORE_LIB_NAME apostol-core)
set(MODULES_LIB_NAME apostol-modules)

add_custom_target(
        auto_increment_version
        ${CMAKE_COMMAND}
        -D VERSION_FILE=${CMAKE_SOURCE_DIR}/version.h
        -P ${CMAKE_SOURCE_DIR}/AutoVersion.cmake
)

# Delphi classes for C++
# ----------------------------------------------------------------------------------------------------------------------
add_compile_options("-DDELPHI_LIB_EXPORTS")
add_compile_options(-DWWWServerName="${PROJECT_DESCRIPTION}")

set(DELPHI_LIB_DIR "src/lib/delphi")

add_subdirectory(${DELPHI_LIB_DIR})

# -Iinclude
include_directories(${DELPHI_LIB_DIR}/include)
include_directories(${DELPHI_LIB_DIR}/src)

# Find PkgConfig
#------------------------------------------------------------------------------
find_package( PkgConfig REQUIRED )

set( prefix "${CMAKE_PREFIX_PATH}" )
set( exec_prefix "\${prefix}" )
set( libdir "\${exec_prefix}/lib" )
set( includedir "\${exec_prefix}/include" )

set( pkgconfigdir "${libdir}/pkgconfig" CACHE PATH "Path to pkgconfig directory." )

# Find OpenPGP
#------------------------------------------------------------------------------
find_package(OpenPGP REQUIRED)

message(STATUS "OpenPGP headers: ${openpgp_INCLUDE_DIRS}")
message(STATUS "OpenPGP directory: ${openpgp_LIBRARY_DIRS}")
message(STATUS "OpenPGP libraries: ${openpgp_LIBRARIES}")

add_definitions( -DOPENSSL_RNG )

# Find dl
#------------------------------------------------------------------------------
if ((${CMAKE_SYSTEM_NAME} MATCHES "Linux"))
    find_package( Dl REQUIRED )

    message(STATUS "Dl libs: ${dl_LIBS}")
    message(STATUS "Dl libraries: ${dl_LIBRARIES}")
endif()

# Define project common includes directories
#------------------------------------------------------------------------------
include_directories( SYSTEM
        ${openpgp_INCLUDE_DIRS}
        ${dl_INCLUDE_DIRS}
        )

# Define project common library directories
#------------------------------------------------------------------------------
link_directories(
        ${openpgp_LIBRARY_DIRS}
        ${dl_LIBRARY_DIRS}
)

# Define project common libraries/linker flags.
#------------------------------------------------------------------------------
link_libraries(
        "-fstack-protector"
        "-fstack-protector-all"
        ${openpgp_LIBRARIES}
        ${dl_LIBRARIES}
)

# Project files
# ----------------------------------------------------------------------------------------------------------------------
include_directories(src/app src/core src/modules)

file(GLOB app_files version.h src/app/*.hpp src/app/*.cpp)
file(GLOB core_files src/core/*.hpp src/core/*.cpp)
file(GLOB modules_files src/modules/Modules.hpp src/modules/*/*.hpp src/modules/*/*.cpp)

# Apostol Core
# ----------------------------------------------------------------------------------------------------------------------
add_library(${CORE_LIB_NAME} STATIC
        $<TARGET_OBJECTS:delphi>
        ${core_files}
        )

target_compile_definitions(${CORE_LIB_NAME} PUBLIC
        APP_NAME="${PROJECT_NAME}"
        APP_DESCRIPTION="${PROJECT_DESCRIPTION}"
        APP_DEFAULT_LOCALE="en_US.UTF-8"
        APP_VAR="${PROJECT_NAME}"
        APP_OLDPID_EXT=".oldbin"
        APP_DEFAULT_USER="nobody"
        APP_DEFAULT_GROUP="nogroup"
        APP_DEFAULT_LISTEN="127.0.0.1"
        APP_PREFIX="${PROJECT_PREFIX}/"
        APP_CONF_PREFIX="conf/"
        APP_CACHE_PREFIX="cache/"
        APP_SBIN_PATH="sbin/${PROJECT_NAME}"
        APP_CONF_FILE="${PROJECT_NAME}.conf"
        APP_PID_FILE="logs/${PROJECT_NAME}.pid"
        APP_LOCK_FILE="logs/${PROJECT_NAME}.lock"
        APP_ERROR_LOG_FILE="logs/error.log"
        APP_ACCESS_LOG_FILE="logs/access.log"
        APP_DOC_ROOT="www/"
        )

target_link_libraries(${CORE_LIB_NAME} pthread)

# Apostol modules
# ----------------------------------------------------------------------------------------------------------------------
add_library(${MODULES_LIB_NAME} STATIC ${modules_files})
target_link_libraries(${MODULES_LIB_NAME} ${CORE_LIB_NAME})

# Apostol Bitcoin Module
# ----------------------------------------------------------------------------------------------------------------------
add_executable(${PROJECT_NAME} ${app_files})
target_link_libraries(${PROJECT_NAME} ${MODULES_LIB_NAME})

add_dependencies(${PROJECT_NAME} auto_increment_version)
# ----------------------------------------------------------------------------------------------------------------------

# Install
# ----------------------------------------------------------------------------------------------------------------------
set(INSTALL_PATH "${PROJECT_PREFIX}")

install(TARGETS ${PROJECT_NAME} DESTINATION ${INSTALL_BIN_PATH})

install(DIRECTORY DESTINATION ${INSTALL_PATH})
install(DIRECTORY DESTINATION ${INSTALL_PATH}/conf)
install(DIRECTORY DESTINATION ${INSTALL_PATH}/logs)
install(DIRECTORY doc/www/ DESTINATION ${INSTALL_PATH}/www)
install(FILES conf/default.conf DESTINATION ${INSTALL_PATH}/conf)
install(FILES conf/default.conf DESTINATION ${INSTALL_PATH} RENAME ${PROJECT_NAME}.conf)

if (INSTALL_AS_ROOT)
    install(FILES auto/daemon DESTINATION /etc/init.d RENAME ${PROJECT_NAME} PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    install(CODE "execute_process(COMMAND update-rc.d ${PROJECT_NAME} defaults)")
endif()
