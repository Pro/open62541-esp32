/* ESP32 OPC UA Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <sys/param.h>
#include <nvs_flash.h>

#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_netif.h"
#include "ethernet_helper.h"

#ifdef CONFIG_IDF_TARGET_ESP32
#define CHIP_NAME "ESP32"
#endif

#ifdef CONFIG_IDF_TARGET_ESP32S2BETA
#define CHIP_NAME "ESP32-S2 Beta"
#endif

#ifndef UA_ARCHITECTURE_FREERTOSLWIP
#error UA_ARCHITECTURE_FREERTOSLWIP needs to be defined
#endif

#include <open62541.h>

#include <esp_task_wdt.h>
#include <esp_sntp.h>

#define ENABLE_MDNS

static const char *TAG = "MAIN";
static const char *TAG_OPC = "OPC UA";

static bool serverCreated = false;

/* Variable holding number of times ESP32 restarted since first boot.
 * It is placed into RTC memory using RTC_DATA_ATTR and
 * maintains its value when ESP32 wakes from deep sleep.
 */
RTC_DATA_ATTR static int boot_count = 0;

static UA_StatusCode
UA_ServerConfig_setUriName(UA_ServerConfig *uaServerConfig, const char *uri, const char *name) {
    // delete pre-initialized values
    UA_String_deleteMembers(&uaServerConfig->applicationDescription.applicationUri);
    UA_LocalizedText_deleteMembers(&uaServerConfig->applicationDescription.applicationName);

    uaServerConfig->applicationDescription.applicationUri = UA_String_fromChars(uri);
    uaServerConfig->applicationDescription.applicationName.locale = UA_STRING_NULL;
    uaServerConfig->applicationDescription.applicationName.text = UA_String_fromChars(name);

    for (size_t i = 0; i < uaServerConfig->endpointsSize; i++) {
        UA_String_deleteMembers(&uaServerConfig->endpoints[i].server.applicationUri);
        UA_LocalizedText_deleteMembers(
                &uaServerConfig->endpoints[i].server.applicationName);

        UA_String_copy(&uaServerConfig->applicationDescription.applicationUri,
                       &uaServerConfig->endpoints[i].server.applicationUri);

        UA_LocalizedText_copy(&uaServerConfig->applicationDescription.applicationName,
                              &uaServerConfig->endpoints[i].server.applicationName);
    }

    return UA_STATUSCODE_GOOD;
}

static void opcua_task(void *arg) {

    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));

    //The default 64KB of memory for sending and receicing buffer caused problems to many users. With the code below, they are reduced to ~16KB
    UA_UInt32 sendBufferSize = 16000;       //64 KB was too much for my platform
    UA_UInt32 recvBufferSize = 16000;       //64 KB was too much for my platform

    ESP_LOGI(TAG_OPC, "Initializing OPC UA. Free Heap: %d bytes", xPortGetFreeHeapSize());


    UA_Server *server = UA_Server_new();

    UA_ServerConfig *config = UA_Server_getConfig(server);

    UA_ServerConfig_setMinimalCustomBuffer(config, 4840, 0, sendBufferSize, recvBufferSize);

    const char* appUri = "open62541.esp32.demo";
    #ifdef ENABLE_MDNS
    config->mdnsEnabled = true;
    config->mdnsConfig.mdnsServerName = UA_String_fromChars(appUri);
    config->mdnsConfig.serverCapabilitiesSize = 2;
    UA_String *caps = (UA_String *) UA_Array_new(2, &UA_TYPES[UA_TYPES_STRING]);
    caps[0] = UA_String_fromChars("LDS");
    caps[1] = UA_String_fromChars("NA");
    config->mdnsConfig.serverCapabilities = caps;

    // We need to set the default IP address for mDNS since internally it's not able to detect it.
    tcpip_adapter_ip_info_t default_ip;
    esp_err_t ret = tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &default_ip);
    if ((ESP_OK == ret) && (default_ip.ip.addr != INADDR_ANY)) {
        config->mdnsIpAddressListSize = 1;
        config->mdnsIpAddressList = (uint32_t *)UA_malloc(sizeof(uint32_t)*config->mdnsIpAddressListSize);
        memcpy(config->mdnsIpAddressList, &default_ip.ip.addr, sizeof(uint32_t));
    } else {
        ESP_LOGI(TAG_OPC, "Could not get default IP Address!");
    }
    #endif
    UA_ServerConfig_setUriName(config, appUri, "open62541 ESP32 Demo");

    #ifndef CONFIG_ETHERNET_HELPER_CUSTOM_HOSTNAME
        #ifndef ETHERNET_HELPER_STATIC_IP4
            #error You need to set a static IP or a custom hostname with menuconfig
        #else
        UA_String str = UA_STRING(CONFIG_ETHERNET_HELPER_STATIC_IP4_ADDRESS);
        #endif
    #else
    UA_String str = UA_STRING(CONFIG_ETHERNET_HELPER_CUSTOM_HOSTNAME_STR);
    #endif
    UA_String_clear(&config->customHostname);
    UA_String_copy(&str, &config->customHostname);

    printf("xPortGetFreeHeapSize before create = %d bytes\n", xPortGetFreeHeapSize());

    UA_StatusCode retval = UA_Server_run_startup(server);
    if (retval != UA_STATUSCODE_GOOD) {
        ESP_LOGE(TAG_OPC, "Starting up the server failed with %s", UA_StatusCode_name(retval));
        return;
    }

    ESP_LOGI(TAG_OPC, "Starting server loop. Free Heap: %d bytes", xPortGetFreeHeapSize());

    while (true) {
        UA_Server_run_iterate(server, false);
        ESP_ERROR_CHECK(esp_task_wdt_reset());
        taskYIELD();
    }
    UA_Server_run_shutdown(server);

    ESP_ERROR_CHECK(esp_task_wdt_delete(NULL));
}


static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    /* UA_Server *server = *(UA_Server**) arg;
    if (server) {
        ESP_LOGI(TAG, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    }*/
}

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    sntp_init();
}

static bool obtain_time(void)
{
    initialize_sntp();

    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));
    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo;
    memset(&timeinfo, 0, sizeof(struct tm));
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry <= retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        ESP_ERROR_CHECK(esp_task_wdt_reset());
    }
    time(&now);
    localtime_r(&now, &timeinfo);
    ESP_ERROR_CHECK(esp_task_wdt_delete(NULL));
    return timeinfo.tm_year > (2016 - 1900);
}

static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    ESP_LOGI(TAG, "WIFI Connected");

    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        if (!obtain_time()) {
            ESP_LOGE(TAG, "Could not get time from NTP. Using default timestamp.");
        }
        // update 'now' variable with current time
        time(&now);
    }
    localtime_r(&now, &timeinfo);
    ESP_LOGI(TAG, "Current time: %d-%02d-%02d %02d:%02d:%02d", timeinfo.tm_year + 1900, timeinfo.tm_mon+1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    if (!serverCreated) {
        ESP_LOGI(TAG, "Starting OPC UA Task");
        // We need a big stack depth here. You may adapt it if necessary
        xTaskCreate(opcua_task, "opcua_task", 24336, NULL, 10, NULL);
        serverCreated = true;
    }
}

void app_main(void)
{
    ++boot_count;
    ESP_LOGI(TAG, "Boot count: %d", boot_count);

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU cores, WiFi%s%s, ",
           CHIP_NAME,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    spi_flash_init();

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Heap Info:\n");
    printf("\tInternal free: %d bytes\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    printf("\tSPI free: %d bytes\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    printf("\tDefault free: %d bytes\n", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
    printf("\tAll free: %d bytes\n", xPortGetFreeHeapSize());


    //static UA_Server *server = NULL;

    ESP_ERROR_CHECK(nvs_flash_init());
    esp_netif_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(esp_task_wdt_init(10, true));
    // Remove idle tasks from watchdog
    ESP_ERROR_CHECK(esp_task_wdt_delete(xTaskGetIdleTaskHandleForCPU(0)));
    ESP_ERROR_CHECK(esp_task_wdt_delete(xTaskGetIdleTaskHandleForCPU(1)));

    /* Register event handlers to stop the server when Wi-Fi or Ethernet is disconnected,
     * and re-start it upon connection.
     */
#ifdef CONFIG_ETHERNET_HELPER_WIFI
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, NULL));
#endif // CONFIG_ETHERNET_HELPER_WIFI
#ifdef CONFIG_ETHERNET_HELPER_ETHERNET
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &connect_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_DISCONNECTED, &disconnect_handler, NULL));
#endif // CONFIG_ETHERNET_HELPER_ETHERNET

    ESP_LOGI(TAG, "Waiting for wifi connection. OnConnect will start OPC UA...");

    ESP_ERROR_CHECK(ethernet_helper_connect());

}