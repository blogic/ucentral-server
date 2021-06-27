project(ucentral-server C)
cmake_minimum_required(VERSION 2.8.12)
find_package(libwebsockets CONFIG REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${LWS_CMAKE_DIR})
include(CheckCSourceCompiles)
include(LwsCheckRequirements)

set(SAMP ucentral-server)
set(SRCS main.c ucode.c ubus.c)

set(requirements 1)
require_lws_config(LWS_ROLE_WS 1 requirements)
require_lws_config(LWS_WITH_SERVER 1 requirements)

if (requirements)
	add_executable(${SAMP} ${SRCS})

	target_link_libraries(${SAMP} websockets_shared ${LIBWEBSOCKETS_DEP_LIBS})
	target_link_libraries(${SAMP} ubox)
	target_link_libraries(${SAMP} blobmsg_json)
	target_link_libraries(${SAMP} ucode)
	target_link_libraries(${SAMP} ubus)
	add_dependencies(${SAMP} websockets_shared)
endif()
