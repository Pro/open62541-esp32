# OPC UA on a ESP32 Microcontroller

open62541 OPC UA example for an ESP32 Microcontroller

This repository shows an exemplary use of the open62541 OPC UA Server implementation on a ESP32 microcontroller.

I used for my implementation and tests the awesome TinyPICO board which provides a soapload of Flash and RAM.

The used open62541 version is https://github.com/open62541/open62541/commit/5478e563159ecc3269ccce3d3088135776ca933a

## Install Dependencies

Before you do anything else, make sure that you followed the Getting Started guide of the ESP IDF Framework, especially install all the dependencies:

https://docs.espressif.com/projects/esp-idf/en/latest/get-started/

**This example example only works with the newest 4.1 Version.**

I tested with branch `release/v4.1` (commit 5ef1b390026270503634ac3ec9f1ec2e364e23b2)

The rest of this README assumes that the IDF_PATH and PATH environment variable are correctly set.

## Setting a fixed ttyUSB Port

If you plug in your microcontroller via USB, Linux will automatically assign a `/dev/ttyUSB` port, starting with index 0. Depending how many USB devices you have connected, it may happen that every time you plug the ESP32, it gets another `ttyUSB` name.
To statically asign a custom port, follow these steps (for Ubuntu):

1. Plug in the ESP32 and call `udevadm info -a -n /dev/ttyUSB0 | grep -E '({serial}|{idProduct}|{idVendor})' | head -n3` (make sure that `ttyUSB0` is the ESP32)
2. Create a new udev rule with `sudo nano /etc/udev/rules.d/99-usb-serial.rules` and the following content, where you need to replace the values whith the one from previous command.
    ```
    SUBSYSTEM=="tty", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea60", ATTRS{serial}=="01435008", SYMLINK+="ttyUSB9"
    ```
3. Now unplug and plug in ESP32 again, and it will be on `/dev/ttyUSB9`

See also:
http://hintshop.ludvig.co.nz/show/persistent-names-usb-serial-devices/

## Building

Make sure that all submodules are initialized:

```
cd open62541-esp32
git submodule update --init --recursive
```

As first step you need to configure the WIFI settings and other possible settings.

Execute:

```
idf.py menuconfig  
```

There you need to set at least the following options:

1. Ethernet Helper Configuration  
    a. WiFi SSID  
    b. WiFi Password  
    c. Use custom hostname (or Static IPv4 alternatively). It is recommended to use a hostname which ends with `.local` (e.g. "esp-opcua.local"). This allows other PCs to resolve the hostname using mDNS.
2. Set the correct value for your Serial flasher config.
    a. Flash SPI Mode -> QIO  
    a. Flash SPI speed -> 40 Mhz  
    b. You should at least have 4MB Flash    
3. An ESP board with external PSRAM is strongly recommended, otherwise the nodeset will not fit in the RAM  
    a. Set it up in Component config -> ESP32-specific -> Support for external, SPI-connected RAM  
    b. For the TinyPICO board I'm just using the default settings there  
  
Then build the `flash` target to flash it to the ESP32.

It is recommended to set the `ESPTOOL_BAUD=1500000` environment variable to get faster flashing.

```
export ESPTOOL_BAUD=1500000
idf.py -p /dev/ttyUSB9 flash
```

### Configure for CLion

If you are using CLion, you need to configure the environment correctly so that code completion works.

1. Settings -> Build, Execution, Deployment -> Toolchains -> Add  
    a. Set the Name to `ESP32`  
    b. Set the C Compiler path to: `$HOME/.espressif/tools/xtensa-esp32-elf/esp32-2019r2-8.2.0/xtensa-esp32-elf/bin/xtensa-esp32-elf-gcc`  
    c. Set the CXX Compiler path to: `$HOME/.espressif/tools/xtensa-esp32-elf/esp32-2019r2-8.2.0/xtensa-esp32-elf/bin/xtensa-esp32-elf-g++`    
2. Settings -> Build, Execution, Deployment -> CMake  
    a. Change the Toolchain to the `ESP32` toolchain created in previous step  
    b. Modify the Environment Variables and add the following:  
        - `IDF_PATH=$HOME/esp/esp-idf`  
        - `ESPTOOL_BAUD=1500000`  
    c. Add all the paths which you get as output by sourcing the ESP IDF config to the `PATH` variable inside the Environment View.  
    
To speed up menuconfig, you may also use the following command:

```
idf.py --build-dir=cmake-build-debug-esp32 -G "Unix Makefiles" menuconfig  
```

To monitor if you built with CLion, just cd into the build directory and run monitor make target:

```
cd cmake-build-debug-esp
make monitor
```


## Monitoring and Debugging

The ESP IDF Framework provides a nice way to see logging output of the microcontroller:

```
idf.py -p /dev/ttyUSB9 monitor
```

An example output is shown here. In this case the OPC UA Endpoint would be: `opc.tcp://10.200.0.107:4840`

```
--- idf_monitor on /dev/ttyUSB0 115200 ---
--- Quit: Ctrl+] | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H ---
ets Jun  8 2016 00:22:57

rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
configsip: 188777542, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:2
load:0x3fff0018,len:4
load:0x3fff001c,len:7268
load:0x40078000,len:16424
load:0x40080400,len:5232
entry 0x40080704
I (78) boot: Chip Revision: 1
I (78) boot_comm: chip revision: 1, min. bootloader chip revision: 0
I (39) boot: ESP-IDF v4.0-beta2-174-g99fb9a3f7 2nd stage bootloader
I (40) boot: compile time 17:47:16
I (40) boot: Enabling RNG early entropy source...
I (46) qio_mode: Enabling default flash chip QIO
I (51) boot: SPI Speed      : 40MHz
I (55) boot: SPI Mode       : QIO
I (59) boot: SPI Flash Size : 4MB
I (63) boot: Partition Table:
I (67) boot: ## Label            Usage          Type ST Offset   Length
I (74) boot:  0 nvs              WiFi data        01 02 0000a000 00005000
I (82) boot:  1 phy_init         RF data          01 01 0000f000 00001000
I (89) boot:  2 factory          factory app      00 00 00010000 00200000
I (97) boot:  3 storage          Unknown data     01 81 00210000 00084000
I (104) boot: End of partition table
I (108) boot_comm: chip revision: 1, min. application chip revision: 0
I (115) esp_image: segment 0: paddr=0x00010020 vaddr=0x3f400020 size=0x274b0 (160944) map
I (174) esp_image: segment 1: paddr=0x000374d8 vaddr=0x3ffb0000 size=0x043b4 ( 17332) load
I (180) esp_image: segment 2: paddr=0x0003b894 vaddr=0x40080000 size=0x00400 (  1024) load
0x40080000: _WindowOverflow4 at /home/user/esp/esp-idf/components/freertos/xtensa_vectors.S:1778

I (181) esp_image: segment 3: paddr=0x0003bc9c vaddr=0x40080400 size=0x04374 ( 17268) load
I (196) esp_image: segment 4: paddr=0x00040018 vaddr=0x400d0018 size=0xbd8b4 (776372) map
0x400d0018: _stext at ??:?

I (435) esp_image: segment 5: paddr=0x000fd8d4 vaddr=0x40084774 size=0x11954 ( 72020) load
0x40084774: heap_caps_malloc_default at /home/user/esp/esp-idf/components/heap/heap_caps.c:144

I (462) esp_image: segment 6: paddr=0x0010f230 vaddr=0x50000000 size=0x00004 (     4) load
I (476) boot: Loaded app from partition at offset 0x10000
I (476) boot: Disabling RNG early entropy source...
I (477) psram: This chip is ESP32-PICO
I (482) spiram: Found 64MBit SPI RAM device
I (486) spiram: SPI RAM mode: flash 40m sram 40m
I (491) spiram: PSRAM initialized, cache is in low/high (2-core) mode.
I (498) cpu_start: Pro cpu up.
I (502) cpu_start: Application information:
I (507) cpu_start: Project name:     open62541-esp32
I (512) cpu_start: App version:      0884345-dirty
I (518) cpu_start: Compile time:     Nov 29 2019 17:47:23
I (524) cpu_start: ELF file SHA256:  0df5c12680813471...
I (530) cpu_start: ESP-IDF:          v4.0-beta2-174-g99fb9a3f7
I (537) cpu_start: Starting app cpu, entry point is 0x4008151c
0x4008151c: call_start_cpu1 at /home/user/esp/esp-idf/components/esp32/cpu_start.c:272

I (0) cpu_start: App cpu up.
I (1428) spiram: SPI SRAM memory test OK
I (1428) heap_init: Initializing. RAM available for dynamic allocation:
I (1428) heap_init: At 3FFAE6E0 len 00001920 (6 KiB): DRAM
I (1434) heap_init: At 3FFBA440 len 00025BC0 (150 KiB): DRAM
I (1441) heap_init: At 3FFE0440 len 00003AE0 (14 KiB): D/IRAM
I (1447) heap_init: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (1454) heap_init: At 400960C8 len 00009F38 (39 KiB): IRAM
I (1460) cpu_start: Pro cpu start user code
I (1465) spiram: Adding pool of 4096K of external SPI memory to heap allocator
I (1486) spi_flash: detected chip: generic
I (1486) spi_flash: flash io: qio
I (1486) cpu_start: Starting scheduler on PRO CPU.
I (0) cpu_start: Starting scheduler on APP CPU.
I (1494) spiram: Reserving pool of 32K of internal memory for DMA/internal allocations
I (1504) MAIN: Boot count: 1
This is ESP32 chip with 2 CPU cores, WiFi/BT/BLE, silicon revision 1, 4MB embedded flash
Heap Info:
	Internal free: 308616 bytes
	SPI free: 4194264 bytes
	Default free: 4429428 bytes
	All free: 4429428 bytes
I (1584) wifi: wifi driver task: 3ffc9fcc, prio:23, stack:3584, core=0
I (1584) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
I (1584) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
I (1614) wifi: wifi firmware version: 7b0c0f3
I (1614) wifi: config NVS flash: enabled
I (1614) wifi: config nano formating: disabled
I (1614) wifi: Init dynamic tx buffer num: 32
I (1614) wifi: Init data frame dynamic rx buffer num: 32
I (1624) wifi: Init management frame dynamic rx buffer num: 32
I (1624) wifi: Init management short buffer num: 32
I (1634) wifi: Init static tx buffer num: 16
I (1634) wifi: Init static rx buffer size: 1600
I (1644) wifi: Init static rx buffer num: 10
I (1644) wifi: Init dynamic rx buffer num: 32
I (1654) ethernet_helper: Connecting to SSID...
I (1744) phy: phy_version: 4102, 2fa7a43, Jul 15 2019, 13:06:06, 0, 0
I (1744) wifi: mode : sta (<MAC_ADDRESS>)
I (2824) wifi: new:<9,2>, old:<1,0>, ap:<255,255>, sta:<9,2>, prof:1
I (3964) wifi: state: init -> auth (b0)
I (3964) wifi: state: auth -> assoc (0)
I (3964) wifi: state: assoc -> run (10)
I (4004) wifi: connected with SSID, aid = 1, channel 9, 40D, bssid = <MAC_ADDRESS>, security type: 4, phy: bgn, rssi: -32
I (4004) wifi: pm start, type: 1

I (4014) wifi: AP's beacon interval = 102400 us, DTIM period = 1
I (5064) MAIN: WIFI Connected
I (5064) MAIN: Time is not set yet. Connecting to WiFi and getting time over NTP.
I (5064) MAIN: Initializing SNTP
I (5064) MAIN: Waiting for system time to be set... (1/10)
I (7074) MAIN: Waiting for system time to be set... (2/10)
I (7964) MAIN: Notification of a time synchronization event
I (9074) MAIN: Starting OPC UA Task
I (9074) tcpip_adapter: sta ip: 10.200.0.107, mask: 255.255.0.0, gw: 10.200.0.1
I (9074) OPC UA: Initializing OPC UA. Free Heap: 4340648 bytes
I (9074) ethernet_helper: Connected to SSID
I (9084) ethernet_helper: IPv4 address: 10.200.0.107
I (9084) ethernet_helper: IPv6 address: fe80:0000:0000:0000:daa0:1aff:fe23:7d0c
I (9094) MAIN: Waiting for wifi connection. OnConnect will start OPC UA...
[1970-01-01 00:00:07.990 (UTC+0000)] warn/server	Username/Password configured, but no encrypting SecurityPolicy. This can leak credentials on the network.
[1970-01-01 00:00:08.000 (UTC+0000)] warn/userland	AcceptAll Certificate Verification. Any remote certificate will be accepted.
xPortGetFreeHeapSize before create = 4186240 bytes
[1970-01-01 00:00:08.020 (UTC+0000)] info/network	TCP network layer listening on opc.tcp://opcua-esp:4840/
I (9514) OPC UA: Starting server loop. Free Heap: 4185720 bytes
```

## Connecting to the OPC UA Server

By default the example code is querying a DHCP server for an ip address. Use the monitoring view to see which
IP address was assigned to the controller. Then you can just use any OPC UA client to connect to the microcontroller.
