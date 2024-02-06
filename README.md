# griffin-radarsystem-fw

## Hardware Summary

The hardware that has been sent by the Client is illustrated

- Custom board (master board)
  - With XM125 Module
    - On-module MCU name: STM32
    - On module Radar sensor name: a121
    - With on board MCU: Yes
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

Since the master board provided by the client is not functioning, as detailed in the 'Hardware Summary,' we are employing the STM32 Nucleo board with the XE125 development kit, following the schematic of the custom board (master).

  


 
