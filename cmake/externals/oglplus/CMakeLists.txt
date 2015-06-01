set(EXTERNAL_NAME oglplus)
string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

include(ExternalProject)
ExternalProject_Add(
  	${EXTERNAL_NAME}
  	URL http://softlayer-dal.dl.sourceforge.net/project/oglplus/oglplus-0.61.x/oglplus-0.61.0.zip
	URL_MD5 bb55038c36c660d2b6c7be380414fa60
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
  	LOG_DOWNLOAD 1
)

ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/include ${SOURCE_DIR}/implement CACHE TYPE INTERNAL)

