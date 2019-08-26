###############################################################################
# FindOpenPGP
#
# Use this module by invoking find_package with the form::
#
#   find_package( OpenPGP
#     [REQUIRED]             # Fail with error if OpenPGP is not found
#   )
#
#   Defines the following for use:
#
#   openpgp_FOUND                  - true if headers and requested libraries were found
#   openpgp_INCLUDE_DIRS           - include directories for OpenPGP libraries
#   openpgp_LIBRARY_DIRS           - link directories for OpenPGP libraries
#   openpgp_LIBRARIES              - OpenPGP libraries to be linked
#   openpgp_PKG                    - OpenPGP pkg-config package specification.
#

if (MSVC)
    if ( OpenPGP_FIND_REQUIRED )
        set( _openpgp_MSG_STATUS "SEND_ERROR" )
    else ()
        set( _openpgp_MSG_STATUS "STATUS" )
    endif()

    set( openpgp_FOUND false )
    message( ${_openpgp_MSG_STATUS} "MSVC environment detection for 'OpenPGP' not currently supported." )
else ()
    # required
    if ( OpenPGP_FIND_REQUIRED )
        set( _openpgp_REQUIRED "REQUIRED" )
    endif()

    # quiet
    if ( OpenPGP_FIND_QUIETLY )
        set( _openpgp_QUIET "QUIET" )
    endif()

    # modulespec
    set( _openpgp_MODULE_SPEC "OpenPGP" )

    pkg_check_modules( openpgp ${_openpgp_REQUIRED} ${_openpgp_QUIET} "${_openpgp_MODULE_SPEC}" )
    set( openpgp_PKG "${_openpgp_MODULE_SPEC}" )
endif()
