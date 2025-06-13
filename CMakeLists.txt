cmake_minimum_required (VERSION 3.19)

include(${CMAKE_ROOT}/Modules/FindOpenSSL.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(BUILD_SHARED_LIBS ON)

add_compile_definitions(KOANA_BUILD)
add_compile_definitions(NOMINMAX)

set(KOANA_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(KOANA_SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/koana)

set(KOANA_VERSION "0.1.0")

if(WIN32)
	# first, lie to mlspp
	set(OPENSSL_VERSION "1.1.1f")
	
	# then, set the include directories
	set(OPENSSL_INCLUDE_DIR "${KOANA_ROOT_PATH}/win32/include")
	set(OPENSSL_LIBRARIES "${KOANA_ROOT_PATH}/win32/lib")

	include_directories("${KOANA_ROOT_PATH}/win32/include")

	link_libraries("${KOANA_ROOT_PATH}/win32/lib/libssl.lib")
	link_libraries("${KOANA_ROOT_PATH}/win32/lib/libcrypto.lib")

	add_compile_definitions(OPENSSL_SYS_WIN32)
else()
	set(OPENSSL_VERSION "3.5.0f")
	include_directories(${OPENSSL_INCLUDE_DIR})

	link_libraries(${OPENSSL_SSL_LIBRARY})
	link_libraries(${OPENSSL_CRYPTO_LIBRARY})
endif()

project(
	libkoana
	VERSION "${KOANA_VERSION}"
	LANGUAGES CXX
	DESCRIPTION "A native library for DAVE E2EE founded on the work of D++ (https://dpp.dev)"
)

set(SOURCES
	${KOANA_SOURCE_PATH}/koana_context.cpp
	${KOANA_SOURCE_PATH}/dave/cipher_interface.cpp
	${KOANA_SOURCE_PATH}/dave/codec_utils.cpp
	${KOANA_SOURCE_PATH}/dave/cryptor_manager.cpp
	${KOANA_SOURCE_PATH}/dave/decryptor.cpp
	${KOANA_SOURCE_PATH}/dave/encryptor.cpp
	${KOANA_SOURCE_PATH}/dave/frame_processors.cpp
	${KOANA_SOURCE_PATH}/dave/leb128.cpp
	${KOANA_SOURCE_PATH}/dave/mls_key_ratchet.cpp
	${KOANA_SOURCE_PATH}/dave/openssl_aead_cipher.cpp
	${KOANA_SOURCE_PATH}/dave/parameters.cpp
	${KOANA_SOURCE_PATH}/dave/persisted_key_pair_generic.cpp
	${KOANA_SOURCE_PATH}/dave/persisted_key_pair.cpp
	${KOANA_SOURCE_PATH}/dave/session.cpp
	${KOANA_SOURCE_PATH}/dave/user_credential.cpp
	${KOANA_SOURCE_PATH}/dave/util.cpp
	${KOANA_SOURCE_PATH}/dave/version.cpp)

add_subdirectory("${KOANA_ROOT_PATH}/mlspp" "mlspp")
include_directories("${KOANA_ROOT_PATH}/mlspp/include")
include_directories("${KOANA_ROOT_PATH}/mlspp/lib/bytes/include")
include_directories("${KOANA_ROOT_PATH}/mlspp/lib/hpke/include")
include_directories("${KOANA_ROOT_PATH}/mlspp/lib/tls_syntax/include")

add_library(koana SHARED ${SOURCES})

target_include_directories(koana PRIVATE "${KOANA_ROOT_PATH}/include")

add_dependencies(koana mlspp mls_vectors bytes tls_syntax hpke)

target_link_libraries(koana PRIVATE mlspp mls_vectors bytes tls_syntax hpke)

target_compile_options(koana PUBLIC -O3 -Isystem)