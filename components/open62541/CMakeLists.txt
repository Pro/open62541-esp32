
# This directory needs to exist, otherwise idf_component_register fails
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/open62541")

# Create a dummy file so that idf_component_register succeeds
if(NOT EXISTS "${CMAKE_BINARY_DIR}/open62541/open62541.c")
    FILE(WRITE ${CMAKE_BINARY_DIR}/open62541/open62541.c
         "#error open62541 not yet generated"
         )
endif()

set(USE_ENCRYPTION OFF)

set(ADDITIONAL_REQUIRES "")
if (USE_ENCRYPTION)
    set(ADDITIONAL_REQUIRES mbedtls)
endif()

idf_component_register(
        SRCS ${CMAKE_BINARY_DIR}/open62541/open62541.c custom_getaddrinfo.c
        REQUIRES lwip vfs freertos ${ADDITIONAL_REQUIRES}
        INCLUDE_DIRS ${CMAKE_BINARY_DIR}/open62541
        PRIV_INCLUDE_DIRS ${IDF_PATH}/components/freertos/include/freertos
)

set(open62541_OPTIONS
    "-DUA_ARCHITECTURE=freertosLWIP"
    "-DUA_ENABLE_AMALGAMATION=ON"
    "-DUA_NAMESPACE_ZERO=FULL"
    "-DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON"
    "-DUA_ENABLE_DISCOVERY=ON"
    "-DUA_ENABLE_DISCOVERY_MULTICAST=ON"
    "-DUA_ENABLE_TYPEDESCRIPTION=OFF"
    "-DUA_ENABLE_STATUSCODE_DESCRIPTIONS=ON"
    "-DUA_LOGLEVEL=300"
    "-DUA_ENABLE_NODESET_COMPILER_DESCRIPTIONS=OFF"
    "-DUA_FILE_NS0_BLACKLIST=${CMAKE_CURRENT_SOURCE_DIR}/../../nodes_blacklist.txt"
)


if (USE_ENCRYPTION)
    set(open62541_OPTIONS ${open62541_OPTIONS} "-DUA_ENABLE_ENCRYPTION=ON")
endif()

externalproject_add(open62541_build
                    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/repo
                    #GIT_REPOSITORY https://github.com/open62541/open62541.git
                    # Latest tested commit on master branch. Newer may work. You can also use 'master' directly as GIT_TAG value
                    #GIT_TAG 76a6cab604fb5309c91a8849183417da1383d3d9
                    BINARY_DIR "${CMAKE_BINARY_DIR}/open62541"
                    CMAKE_ARGS  ${open62541_OPTIONS}
                    BUILD_COMMAND cmake --build . --target open62541-code-generation
                    INSTALL_COMMAND ""
                    )

add_dependencies(${COMPONENT_TARGET} open62541_build)
target_compile_options(${COMPONENT_LIB} PUBLIC -DUA_getaddrinfo=custom_getaddrinfo -DUA_ENABLE_AMALGAMATION -DUA_ARCHITECTURE_FREERTOSLWIP -DUA_ARCHITECTURE_FREERTOSLWIP_POSIX_CLOCK)