###############################################################################
# FindDelphi
#
# Use this module by invoking find_package with the form::
#
#   find_package( Delphi
#     [REQUIRED]             # Fail with error if Delphi is not found
#   )
#
#   Defines the following for use:
#
#   delphi_FOUND                  - true if headers and requested libraries were found
#   delphi_INCLUDE_DIRS           - include directories for Delphi libraries
#   delphi_LIBRARY_DIRS           - link directories for Delphi libraries
#   delphi_LIBRARIES              - Delphi libraries to be linked
#   delphi_PKG                    - Delphi pkg-config package specification.
#

if (MSVC)
    if ( Delphi_FIND_REQUIRED )
        set( _delphi_MSG_STATUS "SEND_ERROR" )
    else ()
        set( _delphi_MSG_STATUS "STATUS" )
    endif()

    set( delphi_FOUND false )
    message( ${_delphi_MSG_STATUS} "MSVC environment detection for 'Delphi' not currently supported." )
else ()
    # required
    if ( Delphi_FIND_REQUIRED )
        set( _delphi_REQUIRED "REQUIRED" )
    endif()

    # quiet
    if ( Delphi_FIND_QUIETLY )
        set( _delphi_QUIET "QUIET" )
    endif()

    # modulespec
    set( _delphi_MODULE_SPEC "delphi" )

    pkg_check_modules( delphi ${_delphi_REQUIRED} ${_delphi_QUIET} "${_delphi_MODULE_SPEC}" )
    set( delphi_PKG "${_delphi_MODULE_SPEC}" )
endif()
