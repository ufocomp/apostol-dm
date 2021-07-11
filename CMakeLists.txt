cmake_minimum_required(VERSION 3.13)
project(apostol-dm)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -rdynamic")

set(PROJECT_NAME "dm")
set(PROJECT_DESCRIPTION "Bitcoin Payment Service (Deal Module)")

message(STATUS "Project name: ${PROJECT_NAME}")
message(STATUS "Project description: ${PROJECT_DESCRIPTION}")

# Settings
# ----------------------------------------------------------------------------------------------------------------------
set(INSTALL_AS_ROOT         ON  CACHE BOOL "Install as root")
set(WITH_POSTGRESQL         OFF CACHE BOOL "Build with PostgreSQL")
set(WITH_SQLITE3            OFF CACHE BOOL "Build with Sqlite3")
set(WITH_CURL               OFF CACHE BOOL "Build with cURL")
set(WITH_BITCOIN_CLIENT     OFF CACHE BOOL "Build with libbitcoin-client")
# ----------------------------------------------------------------------------------------------------------------------

if (INSTALL_AS_ROOT)
    set(INSTALL_BIN_PATH "/usr/sbin")
    set(PROJECT_PREFIX "/etc/${PROJECT_NAME}")
else()
    set(INSTALL_BIN_PATH "/usr/local/sbin")
    set(PROJECT_PREFIX "$ENV{HOME}/.config/${PROJECT_NAME}")
endif()

list( APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake-modules" )

add_compile_options("$<$<CONFIG:DEBUG>:-D_DEBUG>")

set(CORE_LIB_NAME apostol-core)
#set(MODULES_LIB_NAME apostol-modules)

add_custom_target(
        auto_increment_version
        ${CMAKE_COMMAND}
        -D VERSION_FILE=${CMAKE_SOURCE_DIR}/version.h
        -P ${CMAKE_SOURCE_DIR}/AutoVersion.cmake
)

set(PROJECT_LIB_DIR "src/lib")

# Delphi classes for C++
# ----------------------------------------------------------------------------------------------------------------------
set(DELPHI_LIB_NAME delphi)
set(DELPHI_LIB_DIR "${PROJECT_LIB_DIR}/${DELPHI_LIB_NAME}")

# -Iinclude
include_directories(${DELPHI_LIB_DIR}/include)
include_directories(${DELPHI_LIB_DIR}/src)

add_compile_options("-DDELPHI_LIB_EXPORTS")
add_compile_options(-DWWWServerName="${PROJECT_DESCRIPTION}")

if (WITH_POSTGRESQL)
    set(PQ_LIB_NAME "pq")
    add_compile_options("-DWITH_POSTGRESQL")
endif()

if (WITH_SSL)
    set(SSL_LIB_NAME "ssl")
    add_compile_options("-DWITH_SSL")
endif()

if (WITH_SQLITE3)
    set(SQLITE3_LIB_NAME "sqlite3")
    add_compile_options("-DWITH_SQLITE3")
endif()

if (WITH_CURL)
    set(CURL_LIB_NAME "curl")
    add_compile_options("-DWITH_CURL")
endif()

add_subdirectory(${DELPHI_LIB_DIR})

# Find PkgConfig
#------------------------------------------------------------------------------
find_package( PkgConfig REQUIRED )

set( prefix "${CMAKE_PREFIX_PATH}" )
set( exec_prefix "\${prefix}" )
set( libdir "\${exec_prefix}/lib" )
set( includedir "\${exec_prefix}/include" )

set( pkgconfigdir "${libdir}/pkgconfig" CACHE PATH "Path to pkgconfig directory." )

# Find dl
#------------------------------------------------------------------------------
if ((${CMAKE_SYSTEM_NAME} MATCHES "Linux"))
    find_package( Dl REQUIRED )

    message(STATUS "Dl libs: ${dl_LIBS}")
    message(STATUS "Dl libraries: ${dl_LIBRARIES}")
endif()

# rapidxml
# ----------------------------------------------------------------------------------------------------------------------
include_directories(${PROJECT_LIB_DIR}/rapidxml)
file(GLOB lib_files ${lib_files} ${PROJECT_LIB_DIR}/rapidxml/*.hpp)

# picojson
# ----------------------------------------------------------------------------------------------------------------------
include_directories(${PROJECT_LIB_DIR}/picojson)
file(GLOB lib_files ${lib_files} ${PROJECT_LIB_DIR}/picojson/*.h)

# jwt-cpp
# ----------------------------------------------------------------------------------------------------------------------
include_directories(${PROJECT_LIB_DIR}/jwt-cpp)
file(GLOB lib_files ${lib_files} ${PROJECT_LIB_DIR}/jwt-cpp/*.h)

# Find boost
#------------------------------------------------------------------------------
find_package(Boost 1.62.0 REQUIRED COMPONENTS)

set( Boost_LIBRARY_DIR "${Boost_LIBRARY_DIR_RELEASE}" )

set( boost_CPPFLAGS "-I${Boost_INCLUDE_DIR}" )
set( boost_LDFLAGS "-L${Boost_LIBRARY_DIR}" )

# Find OpenPGP
#------------------------------------------------------------------------------
find_package(OpenPGP REQUIRED)

message(STATUS "OpenPGP headers: ${openpgp_INCLUDE_DIRS}")
message(STATUS "OpenPGP directory: ${openpgp_LIBRARY_DIRS}")
message(STATUS "OpenPGP libraries: ${openpgp_LIBRARIES}")

add_definitions( -DOPENSSL_RNG )
add_definitions( -DGPG_COMPATIBLE )

# Find Bitcoin
#------------------------------------------------------------------------------

#set(BITCOIN_VERSION "3.6.0")
set(BITCOIN_VERSION "3.7.0")
#set(BITCOIN_VERSION "4.0.0")

#add_definitions( -DWITH_ICU )

if ( ${BITCOIN_VERSION} VERSION_LESS "3.7.0" )

    set(BITCOIN_LIB_NAME bitcoin)

    find_package(Bitcoin ${BITCOIN_VERSION} REQUIRED)

    message(STATUS "Bitcoin headers: ${bitcoin_INCLUDE_DIRS}")
    message(STATUS "Bitcoin directory: ${bitcoin_LIBRARY_DIRS}")
    message(STATUS "Bitcoin libraries: ${bitcoin_LIBRARIES}")

else()

    if ( ${BITCOIN_VERSION} VERSION_EQUAL "4.0.0" )
        add_definitions(-DBITCOIN_VERSION_4)
    else()
        add_definitions(-DBITCOIN_VERSION_3_7_x)
    endif()

    add_definitions(-DBITCOIN_LIB_NAME="bitcoin-system")
    set(BITCOIN_LIB_NAME bitcoin-system)

    find_package(Bitcoin-System ${BITCOIN_VERSION} REQUIRED)

    message(STATUS "Bitcoin-System headers: ${bitcoin_system_INCLUDE_DIRS}")
    message(STATUS "Bitcoin-System directory: ${bitcoin_system_LIBRARY_DIRS}")
    message(STATUS "Bitcoin-System libraries: ${bitcoin_system_LIBRARIES}")

endif()

# Find Bitcoin-Client
#------------------------------------------------------------------------------
if (WITH_BITCOIN_CLIENT)
    add_compile_options("-DWITH_BITCOIN_CLIENT")

    find_package(Bitcoin-Client ${BITCOIN_VERSION} REQUIRED)

    message(STATUS "Bitcoin-Client headers: ${bitcoin_client_INCLUDE_DIRS}")
    message(STATUS "Bitcoin-Client directory: ${bitcoin_client_LIBRARY_DIRS}")
    message(STATUS "Bitcoin-Client libraries: ${bitcoin_client_LIBRARIES}")
endif()

# Define project common includes directories
#------------------------------------------------------------------------------
include_directories( SYSTEM
        ${bitcoin_INCLUDE_DIRS}
        ${bitcoin_system_INCLUDE_DIRS}
        ${bitcoin_client_INCLUDE_DIRS}
        ${openpgp_INCLUDE_DIRS}
        ${dl_INCLUDE_DIRS}
        )

# Define project common library directories
#------------------------------------------------------------------------------
link_directories(
        ${bitcoin_LIBRARY_DIRS}
        ${bitcoin_system_LIBRARY_DIRS}
        ${bitcoin_client_LIBRARY_DIRS}
        ${openpgp_LIBRARY_DIRS}
        ${dl_LIBRARY_DIRS}
)

# Define project common libraries/linker flags.
#------------------------------------------------------------------------------
link_libraries(
        "-fstack-protector"
        "-fstack-protector-all"
        ${bitcoin_LIBRARIES}
        ${bitcoin_system_LIBRARIES}
        ${bitcoin_client_LIBRARIES}
        ${openpgp_LIBRARIES}
        ${dl_LIBRARIES}
)

# Apostol
# ----------------------------------------------------------------------------------------------------------------------
include_directories(src/app src/core src/modules src/modules/Workers src/modules/Helpers src/processes)

file(GLOB app_files version.h src/app/*.hpp src/app/*.cpp)
file(GLOB core_files src/core/*.hpp src/core/*.cpp)
file(GLOB modules_files src/modules/Modules.hpp src/modules/*/*.hpp src/modules/*/*/*.hpp src/modules/*/*/*.cpp)
file(GLOB processes_files src/processes/Processes.hpp src/processes/*/*.hpp src/processes/*/*.cpp)

# Apostol Core
# ----------------------------------------------------------------------------------------------------------------------
add_library(${CORE_LIB_NAME} STATIC
        $<TARGET_OBJECTS:delphi>
        ${lib_files}
        ${core_files}
        ${modules_files}
        ${processes_files}
        )

target_compile_definitions(${CORE_LIB_NAME} PUBLIC
        APP_NAME="${PROJECT_NAME}"
        APP_DESCRIPTION="${PROJECT_DESCRIPTION}"
        APP_DEFAULT_LOCALE="en_US.UTF-8"
        APP_VAR="${PROJECT_NAME}"
        APP_OLDPID_EXT=".oldbin"
        APP_DEFAULT_USER="nobody"
        APP_DEFAULT_GROUP="nogroup"
        APP_DEFAULT_LISTEN="0.0.0.0"
        APP_PREFIX="${PROJECT_PREFIX}/"
        APP_CONF_PREFIX="conf/"
        APP_CACHE_PREFIX="cache/"
        APP_SBIN_PATH="sbin/${PROJECT_NAME}"
        APP_CONF_FILE="${PROJECT_NAME}.conf"
        APP_PID_FILE="logs/${PROJECT_NAME}.pid"
        APP_LOCK_FILE="logs/${PROJECT_NAME}.lock"
        APP_ERROR_LOG_FILE="logs/error.log"
        APP_ACCESS_LOG_FILE="logs/access.log"
        APP_POSTGRES_LOG_FILE="logs/postgres.log"
        APP_STREAM_LOG_FILE="logs/stream.log"
        APP_DOC_ROOT="www/"
        )

target_link_libraries(${CORE_LIB_NAME} pthread ${PQ_LIB_NAME} ${SQLITE3_LIB_NAME} ${CURL_LIB_NAME} yaml-cpp ssl crypto)

# Apostol modules
# ----------------------------------------------------------------------------------------------------------------------
#add_library(${MODULES_LIB_NAME} STATIC ${modules_files})
#target_link_libraries(${MODULES_LIB_NAME} ${CORE_LIB_NAME})

# BitDeals Payment Service (Deal Module)
# ----------------------------------------------------------------------------------------------------------------------
add_executable(${PROJECT_NAME} ${app_files})
target_link_libraries(${PROJECT_NAME} ${CORE_LIB_NAME})

add_dependencies(${PROJECT_NAME} auto_increment_version)
# ----------------------------------------------------------------------------------------------------------------------

# Install
# ----------------------------------------------------------------------------------------------------------------------
set(INSTALL_PATH "${PROJECT_PREFIX}")

install(TARGETS ${PROJECT_NAME} DESTINATION ${INSTALL_BIN_PATH})

install(DIRECTORY DESTINATION ${INSTALL_PATH})
install(DIRECTORY DESTINATION ${INSTALL_PATH}/oauth2)
install(DIRECTORY DESTINATION ${INSTALL_PATH}/conf)
install(DIRECTORY DESTINATION ${INSTALL_PATH}/logs)
install(DIRECTORY DESTINATION ${INSTALL_PATH}/sites)
install(DIRECTORY conf/oauth2/ DESTINATION ${INSTALL_PATH}/oauth2)
install(DIRECTORY conf/sites/ DESTINATION ${INSTALL_PATH}/sites)
install(DIRECTORY www/ DESTINATION ${INSTALL_PATH}/www)
install(FILES conf/default.conf DESTINATION ${INSTALL_PATH}/conf)
install(FILES conf/oauth2.conf DESTINATION ${INSTALL_PATH}/conf)
install(FILES conf/sites.conf DESTINATION ${INSTALL_PATH}/conf)
install(FILES conf/default.conf DESTINATION ${INSTALL_PATH} RENAME ${PROJECT_NAME}.conf)

if (INSTALL_AS_ROOT)
    install(FILES auto/daemon DESTINATION /etc/init.d RENAME ${PROJECT_NAME} PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    install(CODE "execute_process(COMMAND update-rc.d ${PROJECT_NAME} defaults)")
endif()
