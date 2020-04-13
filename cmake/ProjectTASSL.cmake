include(ExternalProject)
include(GNUInstallDirs)

if (BUILD_GM)
	if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
		set(TASSL_CONFIG_COMMAND perl ./Configure darwin64-x86_64-cc)
	else()
		set(TASSL_CONFIG_COMMAND bash config -Wl,--rpath=./ shared)
	endif ()

	set(TASSL_BUILD_COMMAND make)

	ExternalProject_Add(tassl-1.1.1b
		PREFIX ${CMAKE_SOURCE_DIR}/deps
		DOWNLOAD_NO_PROGRESS 1
		DOWNLOAD_NAME TASSL-1.1.1b
		GIT_REPOSITORY https://github.com/bxq2011hust/TASSL-1.1.1b.git
		GIT_TAG f32d589b2611dcb22675e9a2baa8a668cbf518dd
		# GIT_SHALLOW true
		BUILD_IN_SOURCE 1
		CONFIGURE_COMMAND ${TASSL_CONFIG_COMMAND}
		LOG_CONFIGURE 1
		LOG_BUILD 1
		LOG_INSTALL 1
		BUILD_COMMAND ${TASSL_BUILD_COMMAND}
		INSTALL_COMMAND ""
	)

	ExternalProject_Get_Property(tassl-1.1.1b SOURCE_DIR)
	add_library(TASSL STATIC IMPORTED)
	set(TASSL_SUFFIX .a)
	set(TASSL_INCLUDE_DIRS ${SOURCE_DIR}/include)
	set(TASSL_CRYPTO_NCLUDE_DIRS ${SOURCE_DIR}/crypto/include)
	set(TASSL_LIBRARY ${SOURCE_DIR}/libssl${TASSL_SUFFIX})
	set(TASSL_CRYPTO_LIBRARIE ${SOURCE_DIR}/libcrypto${TASSL_SUFFIX})
	set(TASSL_LIBRARIES ${TASSL_LIBRARY} ${TASSL_CRYPTO_LIBRARIE} dl)
	set_property(TARGET TASSL PROPERTY IMPORTED_LOCATION ${TASSL_LIBRARIES})
	set_property(TARGET TASSL PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${TASSL_INCLUDE_DIRS})

	set(OPENSSL_INCLUDE_DIRS ${TASSL_INCLUDE_DIRS})
	set(OPENSSL_LIBRARIES ${TASSL_LIBRARY})
endif()



