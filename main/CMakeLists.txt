
idf_component_register(SRCS "main.c"
                       PRIV_INCLUDE_DIRS ${IDF_PATH}/components/freertos/include/freertos
                       REQUIRES open62541 ethernet_helper nvs_flash esp_adc_cal)

# Build static library, do not build test executables
option(BUILD_SHARED_LIBS OFF)
option(BUILD_TESTING OFF)

# ESP32 is little endian
add_definitions(-DUA_ENABLE_AMALGAMATION -DUA_ARCHITECTURE_FREERTOSLWIP)
#add_definitions(-Wno-ignored-qualifiers)

# Unfortunately the library performs install and export. Would
# have been nice if devs made that an option like BUILD_SHARED_LIBS
# and BUILD_TESTING. Override install() and export() to do nothing
# instead.
function(install)
endfunction()

function(export)
endfunction()
