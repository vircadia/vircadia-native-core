# - Try to find the OpenSSL encryption library
# Once done this will define
#
#  OPENSSL_ROOT_DIR - Set this variable to the root installation of OpenSSL
#
# Read-Only variables:
#  OPENSSL_FOUND - system has the OpenSSL library
#  OPENSSL_INCLUDE_DIR - the OpenSSL include directory
#  OPENSSL_LIBRARIES - The libraries needed to use OpenSSL
#  OPENSSL_VERSION - This is set to $major.$minor.$revision$path (eg. 0.9.8s)
#
# Modified on 7/16/2014 by Stephen Birarda
# This is an adapted version of the FindOpenSSL.cmake module distributed with Cmake 2.8.12.2
# The original license for that file is displayed below.
#
#=============================================================================
# Copyright 2006-2009 Kitware, Inc.
# Copyright 2006 Alexander Neundorf <neundorf@kde.org>
# Copyright 2009-2011 Mathieu Malaterre <mathieu.malaterre@gmail.com>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

if (UNIX)
  find_package(PkgConfig QUIET)
  pkg_check_modules(_OPENSSL QUIET openssl)
endif ()

if (WIN32)

  file(TO_CMAKE_PATH "$ENV{PROGRAMFILES}" _programfiles)

  if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    # http://www.slproweb.com/products/Win32OpenSSL.html
    set(_OPENSSL_ROOT_HINTS ${OPENSSL_ROOT_DIR} $ENV{OPENSSL_ROOT_DIR} $ENV{HIFI_LIB_DIR}/openssl
      "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\OpenSSL (64-bit)_is1;Inno Setup: App Path]"
    )
    set(_OPENSSL_ROOT_PATHS "${_programfiles}/OpenSSL" "${_programfiles}/OpenSSL-Win64" "C:/OpenSSL/" "C:/OpenSSL-Win64/")
  else()
    # http://www.slproweb.com/products/Win32OpenSSL.html
    set(_OPENSSL_ROOT_HINTS ${OPENSSL_ROOT_DIR} $ENV{OPENSSL_ROOT_DIR} $ENV{HIFI_LIB_DIR}/openssl
      "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\OpenSSL (32-bit)_is1;Inno Setup: App Path]"
    )
    set(_OPENSSL_ROOT_PATHS "${_programfiles}/OpenSSL" "${_programfiles}/OpenSSL-Win32" "C:/OpenSSL/" "C:/OpenSSL-Win32/")
  endif()

  unset(_programfiles)
  set(_OPENSSL_ROOT_HINTS_AND_PATHS HINTS ${_OPENSSL_ROOT_HINTS} PATHS ${_OPENSSL_ROOT_PATHS})

else ()
  include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
  hifi_library_search_hints("openssl")

  set(_OPENSSL_ROOT_HINTS_AND_PATHS ${OPENSSL_SEARCH_DIRS})
endif ()

find_path(OPENSSL_INCLUDE_DIR NAMES openssl/ssl.h HINTS ${_OPENSSL_ROOT_HINTS_AND_PATHS} ${_OPENSSL_INCLUDEDIR}
  PATH_SUFFIXES include
)

if (WIN32 AND NOT CYGWIN)
  if (MSVC)

    # In Visual C++ naming convention each of these four kinds of Windows libraries has it's standard suffix:
    #   * MD for dynamic-release
    #   * MDd for dynamic-debug
    #   * MT for static-release
    #   * MTd for static-debug

    # Implementation details:
    # We are using the libraries located in the VC subdir instead of the parent directory eventhough :
    # libeay32MD.lib is identical to ../libeay32.lib, and
    # ssleay32MD.lib is identical to ../ssleay32.lib

    # The Kitware FindOpenSSL module has been modified here by High Fidelity to look specifically for static libraries

    find_library(LIB_EAY_DEBUG NAMES libeay32MTd
      ${_OPENSSL_ROOT_HINTS_AND_PATHS} PATH_SUFFIXES "lib/VC/static"
    )

    find_library(LIB_EAY_RELEASE NAMES libeay32MT
      ${_OPENSSL_ROOT_HINTS_AND_PATHS} PATH_SUFFIXES "lib/VC/static"
    )

    find_library(SSL_EAY_DEBUG NAMES ssleay32MTd
      ${_OPENSSL_ROOT_HINTS_AND_PATHS} PATH_SUFFIXES "lib/VC/static"
    )

    find_library(SSL_EAY_RELEASE NAMES ssleay32MT
      ${_OPENSSL_ROOT_HINTS_AND_PATHS} PATH_SUFFIXES "lib/VC/static"
    )

    set(LIB_EAY_LIBRARY_DEBUG "${LIB_EAY_DEBUG}")
    set(LIB_EAY_LIBRARY_RELEASE "${LIB_EAY_RELEASE}")
    set(SSL_EAY_LIBRARY_DEBUG "${SSL_EAY_DEBUG}")
    set(SSL_EAY_LIBRARY_RELEASE "${SSL_EAY_RELEASE}")

    include(SelectLibraryConfigurations)
    select_library_configurations(LIB_EAY)
    select_library_configurations(SSL_EAY)

    set(OPENSSL_LIBRARIES ${SSL_EAY_LIBRARY} ${LIB_EAY_LIBRARY})

    find_path(OPENSSL_DLL_PATH NAMES ssleay32.dll PATH_SUFFIXES "bin" ${_OPENSSL_ROOT_HINTS_AND_PATHS})
  endif()
else()

  find_library(OPENSSL_SSL_LIBRARY NAMES ssl ssleay32 ssleay32MD HINTS ${_OPENSSL_ROOT_HINTS_AND_PATHS} ${_OPENSSL_LIBDIR}
    PATH_SUFFIXES lib
  )

  find_library(OPENSSL_CRYPTO_LIBRARY NAMES crypto HINTS ${_OPENSSL_ROOT_HINTS_AND_PATHS} ${_OPENSSL_LIBDIR}
    PATH_SUFFIXES lib
  )

  mark_as_advanced(OPENSSL_CRYPTO_LIBRARY OPENSSL_SSL_LIBRARY)

  # compat defines
  set(OPENSSL_SSL_LIBRARIES ${OPENSSL_SSL_LIBRARY})
  set(OPENSSL_CRYPTO_LIBRARIES ${OPENSSL_CRYPTO_LIBRARY})

  set(OPENSSL_LIBRARIES ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})

endif ()

function(from_hex HEX DEC)
  string(TOUPPER "${HEX}" HEX)
  set(_res 0)
  string(LENGTH "${HEX}" _strlen)

  while (_strlen GREATER 0)
    math(EXPR _res "${_res} * 16")
    string(SUBSTRING "${HEX}" 0 1 NIBBLE)
    string(SUBSTRING "${HEX}" 1 -1 HEX)
    if (NIBBLE STREQUAL "A")
      math(EXPR _res "${_res} + 10")
    elseif (NIBBLE STREQUAL "B")
      math(EXPR _res "${_res} + 11")
    elseif (NIBBLE STREQUAL "C")
      math(EXPR _res "${_res} + 12")
    elseif (NIBBLE STREQUAL "D")
      math(EXPR _res "${_res} + 13")
    elseif (NIBBLE STREQUAL "E")
      math(EXPR _res "${_res} + 14")
    elseif (NIBBLE STREQUAL "F")
      math(EXPR _res "${_res} + 15")
    else()
      math(EXPR _res "${_res} + ${NIBBLE}")
    endif()

    string(LENGTH "${HEX}" _strlen)
  endwhile()

  set(${DEC} ${_res} PARENT_SCOPE)
endfunction()

if (OPENSSL_INCLUDE_DIR)
  if(OPENSSL_INCLUDE_DIR AND EXISTS "${OPENSSL_INCLUDE_DIR}/openssl/opensslv.h")
    file(STRINGS "${OPENSSL_INCLUDE_DIR}/openssl/opensslv.h" openssl_version_str
         REGEX "^#[ ]?define[\t ]+OPENSSL_VERSION_NUMBER[\t ]+0x([0-9a-fA-F])+.*")

    # The version number is encoded as 0xMNNFFPPS: major minor fix patch status
    # The status gives if this is a developer or prerelease and is ignored here.
    # Major, minor, and fix directly translate into the version numbers shown in
    # the string. The patch field translates to the single character suffix that
    # indicates the bug fix state, which 00 -> nothing, 01 -> a, 02 -> b and so
    # on.

    string(REGEX REPLACE "^.*OPENSSL_VERSION_NUMBER[\t ]+0x([0-9a-fA-F])([0-9a-fA-F][0-9a-fA-F])([0-9a-fA-F][0-9a-fA-F])([0-9a-fA-F][0-9a-fA-F])([0-9a-fA-F]).*$"
           "\\1;\\2;\\3;\\4;\\5" OPENSSL_VERSION_LIST "${openssl_version_str}")
    list(GET OPENSSL_VERSION_LIST 0 OPENSSL_VERSION_MAJOR)
    list(GET OPENSSL_VERSION_LIST 1 OPENSSL_VERSION_MINOR)
    from_hex("${OPENSSL_VERSION_MINOR}" OPENSSL_VERSION_MINOR)
    list(GET OPENSSL_VERSION_LIST 2 OPENSSL_VERSION_FIX)
    from_hex("${OPENSSL_VERSION_FIX}" OPENSSL_VERSION_FIX)
    list(GET OPENSSL_VERSION_LIST 3 OPENSSL_VERSION_PATCH)

    if (NOT OPENSSL_VERSION_PATCH STREQUAL "00")
      from_hex("${OPENSSL_VERSION_PATCH}" _tmp)
      # 96 is the ASCII code of 'a' minus 1
      math(EXPR OPENSSL_VERSION_PATCH_ASCII "${_tmp} + 96")
      unset(_tmp)
      # Once anyone knows how OpenSSL would call the patch versions beyond 'z'
      # this should be updated to handle that, too. This has not happened yet
      # so it is simply ignored here for now.
      string(ASCII "${OPENSSL_VERSION_PATCH_ASCII}" OPENSSL_VERSION_PATCH_STRING)
    endif ()

    set(OPENSSL_VERSION "${OPENSSL_VERSION_MAJOR}.${OPENSSL_VERSION_MINOR}.${OPENSSL_VERSION_FIX}${OPENSSL_VERSION_PATCH_STRING}")
  endif ()
endif ()

include(FindPackageHandleStandardArgs)

set(OPENSSL_REQUIREMENTS OPENSSL_LIBRARIES OPENSSL_INCLUDE_DIR)
if (WIN32)
  list(APPEND OPENSSL_REQUIREMENTS OPENSSL_DLL_PATH)
endif ()

if (OPENSSL_VERSION)
  find_package_handle_standard_args(OpenSSL
    REQUIRED_VARS
      ${OPENSSL_REQUIREMENTS}
    VERSION_VAR
      OPENSSL_VERSION
    FAIL_MESSAGE
      "Could NOT find OpenSSL, try to set the path to OpenSSL root folder in the system variable OPENSSL_ROOT_DIR"
  )
else ()
  find_package_handle_standard_args(OpenSSL "Could NOT find OpenSSL, try to set the path to OpenSSL root folder in the system variable OPENSSL_ROOT_DIR"
    ${OPENSSL_REQUIREMENTS}
  )
endif ()

mark_as_advanced(OPENSSL_INCLUDE_DIR OPENSSL_LIBRARIES OPENSSL_SEARCH_DIRS)
