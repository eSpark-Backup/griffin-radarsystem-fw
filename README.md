# griffin-radarsystem-fw

## Hardware Summary

The hardware that has been sent by the Client is illustrated

- Custom board (master board)
  - With XM125 Module
    - On-module MCU name: STM32
    - On module Radar sensor name: a121
    - With on-board MCU: Yes
    - Status: Not working
    - Remarks: MCU pins are short

- Custom board (small board)
  - With XM124 Module
    - On-module MCU name: STM32
    - On module Radar sensor name: a111
    - With on board MCU: No
    - Status: Working
    - Remarks: -

- Development board (XE125)
  - With XM125 Module
    - On-module MCU name: STM32
    - On module Radar sensor name: A121
    - With on board MCU: ---
    - Status: Working
    - Remarks: Developing project with it since the custom board master is not working

  - With XM125 Module
    - On-module MCU name: STM32
    - On module Radar sensor name: A121
    - With on board MCU: ---
    - Status: Not working
    - Remarks: One UART is not working


Image of all items received

![image](https://github.com/eSpark-Consultants/griffin-radarsystem-fw/assets/131872950/52d82f3c-f36f-4e90-b30f-8ada3b95bb82)

## Basic Idea of The Project

- The master board with an A121 Radar sensor functions as a radar system for detecting approaching objects.
- Upon detection of an approaching object, it activates the flatcoin vibration motor for 2 seconds.
- A small board with an A111 radar system is aimed at the ground to establish a reference velocity.
- The small board is positioned at an angle, as shown in the image below.
  ![small board position](https://github.com/eSpark-Consultants/griffin-radarsystem-fw/assets/131872950/e1aeab34-3b04-4bae-82f0-5eadd3d3931a)
- Detection by the small board starts from 5 feet away to accommodate potential arm or foot movements when the device is worn.
- Similarly, detection by the master board begins from 5 feet away to avoid registering foot movements.
- The main sensor is configured to detect objects within a maximum range of 20 meters.
- To distinguish moving objects from stationary ones, the velocity of the forward-facing radar (master board) is compared to that of the ground (small board).
- If the forward-facing radar's (master board) velocity exceeds that of the ground (small board), it indicates an approaching object.
- Additional filtering techniques, such as averaging, may be applied if necessary.
- A threshold is set to activate the flatcoin vibration motor only when the approaching object's velocity exceeds 1 meter per second relative to the ground radar's velocity.

## Hardware Configuration

### Master Board

Since the master board provided by the client is not functioning, as detailed in the 'Hardware Summary,' we are employing the STM32 Nucleo board with the XE125 development kit, following the schematic of the custom board (master) [Master PCB](https://github.com/eSpark-Consultants/griffin-radarsystem-fw/blob/main/docs/pcb%20schematic.pdf). As shown in the PCB the controller has been communicating with the Module XM125 over I2C.

### Small Board

The small board just contains module XM124 module with the header as shown in the schematic below:

![Small Board Schematic](https://github.com/eSpark-Consultants/griffin-radarsystem-fw/assets/76909080/50f3a983-dfff-4cc3-84c0-5a1f07cfa45c)

Since the board requires 1.8 volts and the USB to UART converter (CP201x) works on 3v3 or 5 volts therefore we have to bridge a level shifter between the board and the CP201x. The connections are shown in the image below:

![image](https://github.com/eSpark-Consultants/griffin-radarsystem-fw/assets/131872950/2dcdac32-645a-4666-9dd1-a8c34cc4ec8b)

## Software setup

To establish the environment for software development, follow these steps or alternatively, clone the project from the firmware section located below this one.

1. First we need to install SDKs from [SDK](https://developer.acconeer.com/) for both A111 and A121

   Install for A111 

   ![image](https://github.com/eSpark-Consultants/griffin-radarsystem-fw/assets/131872950/5242f7db-14c0-4176-8e1c-8b4c5b66816e)

   Install for A121

   ![image](https://github.com/eSpark-Consultants/griffin-radarsystem-fw/assets/131872950/623fdda2-a671-4fce-bfeb-97b022614961)

2. After downloading the SDKs you can follow the following [document](https://github.com/eSpark-Consultants/griffin-radarsystem-fw/blob/1-firmware-development-for-approacing-object-detection-v10/Firmware%20Development/xm125/doc/XM125%20Software%20User%20Guide.pdf) from page number 7 to set up your STM32 Cube IDE for the development. This document is for XM125 but the same steps will be followed for the XM124 module.

## Firmware

### Master Firmware

[Master Firmware](https://github.com/eSpark-Consultants/griffin-radarsystem-fw/tree/1-firmware-development-for-approacing-object-detection-v10/Firmware%20Development/Master%20Firmware/RADAR)

### Slave Firmware 

[Slave Firmware (A121) ](https://github.com/eSpark-Consultants/griffin-radarsystem-fw/tree/1-firmware-development-for-approacing-object-detection-v10/Firmware%20Development/xm125)
[Slave Firmware (A111) ](https://github.com/eSpark-Consultants/griffin-radarsystem-fw/tree/1-firmware-development-for-approacing-object-detection-v10/Firmware%20Development/A111)

Note:
  For flashing the code on the XM124 module we need to use the STM32 programmer. Follow this [Link](https://www.st.com/resource/en/user_manual/um2237-stm32cubeprogrammer-software-description-stmicroelectronics.pdf)
  
## Acconeer Support and Their Exploration Tool:

Acconeer has offered exceptional assistance with their exploration tool, which is compatible with both A111 and A121 sensors. This UI-based tool facilitates radar testing and serves as a valuable resource throughout your firmware development process, allowing you to verify compatibility along the way. 

Follow the following YouTube tutorials regarding the [Exploration tool](https://github.com/acconeer/acconeer-python-exploration):

[A121](https://www.youtube.com/watch?v=Z8lQgxaJFOY)

[A111](https://www.youtube.com/watch?v=WKVoFGJzubs&t=266s)







  


 
