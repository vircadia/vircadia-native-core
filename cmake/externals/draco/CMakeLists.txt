set(EXTERNAL_NAME draco)

if (ANDROID)
  set(ANDROID_CMAKE_ARGS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}" "-DANDROID_NATIVE_API_LEVEL=19")
endif ()

if (APPLE)
  set(EXTRA_CMAKE_FLAGS -DCMAKE_CXX_FLAGS=-stdlib=libc++ -DCMAKE_EXE_LINKER_FLAGS=-stdlib=libc++)
endif ()

include(ExternalProject)
ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL https://public.highfidelity.com/dependencies/draco-1.1.0.zip
  URL_MD5 208f8b04c91d5f1c73d731a3ea37c5bb
  CONFIGURE_COMMAND CMAKE_ARGS  ${ANDROID_CMAKE_ARGS} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>-$<CONFIG> ${EXTRA_CMAKE_FLAGS}
  LOG_DOWNLOAD 1
  LOG_CONFIGURE 1
  LOG_BUILD 1
)

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)
set(SUFFIXED_INSTALL_DIR "${INSTALL_DIR}-$<CONFIG>")

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SUFFIXED_INSTALL_DIR}/include CACHE PATH "List of Draco include directories")

if (UNIX)
  set(LIB_PREFIX "lib")
  set(LIB_EXT "a")
elseif (WIN32)
  set(LIB_EXT "lib")
endif ()

set(${EXTERNAL_NAME_UPPER}_LIBRARY ${SUFFIXED_INSTALL_DIR}/lib/${LIB_PREFIX}draco.${LIB_EXT} CACHE FILEPATH "Path to Draco release library")
set(${EXTERNAL_NAME_UPPER}_ENCODER_LIBRARY ${SUFFIXED_INSTALL_DIR}/lib/${LIB_PREFIX}dracoenc.${LIB_EXT} CACHE FILEPATH "Path to Draco encoder release library")
set(${EXTERNAL_NAME_UPPER}_DECODER_LIBRARY ${SUFFIXED_INSTALL_DIR}/lib/${LIB_PREFIX}dracodec.${LIB_EXT} CACHE FILEPATH "Path to Draco decoder release library")
