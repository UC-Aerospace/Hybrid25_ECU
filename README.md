# Hybrid Engine Control Unit

This repo contains the hardware and firmware for the 2025 Hybrid Rocket Engine FYP test stand electronics.

The hardware consists of three seperate modules, the central ECU alongside a servo and ADC peripheral. Central to peripheral
communication uses CAN-FD and is designed to be modular, while communications to a remote interface unit is done over RS422.
For more information on the RIU it can be found [here](https://github.com/UC-Aerospace/Hybrid25_RIU).

<img width="882" alt="image" src="https://github.com/user-attachments/assets/0b086ce3-42a3-48b5-a7de-66e9607cbf3b" />

## Hardware

### MCU

The hardware of all these boards has been based on the STM32G0 MCU as a platform. The selection process for this was pretty
much based on wanting to use a STM32 for experience and the G0 in particular has two CAN-FD modems and was quite affordable.
This was a mistake, although it ended up working out okay. The STM32G0 critically lacks a SDIO peripheral which is a much
faster and more robust way of interacting with a SD card than SPI, as well as lacking QSPI and only having one SPI bus,
which meant we couldn't implement external flash memory for logging to. Given the issues with SD cards on an actual launch
vechicle that has been experienced I would have the following reccomendations:

* If you make new hardware switch MCU. Still use a ST product but pivot to something with SDIO and QSPI. Look into the F4, F7, H4 and H7 lines.
* Then implement a system where logging happens to flash during a test/launch, and then copies to SD afterwards. This means transfer to a PC
is still easy, you still get the bulk storage of SD but don't rely on it during high vibration periods.
* Don't go for a dual core. The cost increase dosn't justify the advantage and the complexity is significant.
* On cost the total sum of all the electronics this year was large enough not to worry about penny pinching on the MCU you buy maybe 5 of. $8 to $15
is inconsequential and well worth it if it saves you time.

Otherwise though the G0 has served it's purpose well this year. Honestly I have no idea how close we are cutting it with processing time overhead,
the MCU is plenty fast but a lot of the external I/O is quite slow (comms, sensors, SD card). Buffering is the name of the game in cases like that,
and trying to pass as much off to hardware peripherals as possible. Future teams could investigate a switch back to FreeRTOS for more flexibility around
scheduling, this will save some headaches and add a bunch more.

### Layout

<img width="882" alt="image" src="https://github.com/user-attachments/assets/86e384f3-1a66-4b6a-aac1-fbbb014cdd8d" />

The central ECU is pretty simple and carries out a couple of tasks. Firstly is comms which is easy to see with CAN on the left and RS422 on the right
of the schematic. The SD card occupies the top right, with pull up resistors on the main data lines. The top right has a display over I2C and a
pressure and temp sensor. On the status LED front the other boards had two controlled from GPIOs while the central ECU only has one. Two is much
better as you can have a status and error LED, don't discount just how useful a few LEDs are. And finally the interesting bit is probably the spicy stuff.

<img width="882" alt="image" src="https://github.com/user-attachments/assets/4dd37550-c5a0-44e8-8218-9285e8c88f6f" />

This is for the control of the single solenoid and two ignitor channels. The layout is very similar for both, except the ignitors have continuity detection
as well. First off theres a connector populated to allow for an external regulator if the voltage requirements of the solenoid changes. This occured as we
made this board before selecting a solenoid. A comparator then is used on the input so we know when the RIU and external ESTOP are released.

For the actual switching there is both a high and low side switch. The high side switch is common to both ignitors and solenoid, and comes in the form of a
EFuse. Theres some big advantages to doing it this way, for one it limits the current spike that occurs when switching the outputs, and also will protect the
circuit in the case of a shorted output. A GPIO toggles this, allowing for a hardware "Arm" controlled by software. A 2K resistor bypasses the highside switch,
this allows for continuity detection. Nominally no current will flow through this as the low side switches will be off, but even if one of the low side switches
is on the current is limited to 15mA, too low to ignite one of the ignitors or open the solenoid.

## Software

...

### Finite State Machine Diagrams

#### Central ECU

<img width="882" alt="image" src="https://github.com/user-attachments/assets/ce11024d-72e6-4584-9be2-6bc21fa01c2f" />

#### Servo Module

<img width="882" alt="image" src="https://github.com/user-attachments/assets/f395c7c0-076e-4b11-afe0-9f1687392bde" />


## Operational

### Safety

Safety has been one of the major concerns with this project. To try and mitigate some of those concerns the main solenoid
is normally closed, such that within 350 milliseconds of cutting the power it fully shuts. The ignitors, being potentially
explosive, were also identified as high risk. Therefore, both these items are on a seperate circuit. Two air gaps exist
in this loop, one E-Stop positioned inline and outside the test container, another at the RIU and connected through a relay
to limit current loss.

<img width="882" alt="image" src="https://github.com/user-attachments/assets/7a1bfbcf-406d-4c0b-83eb-f71a759fa2aa" />

<img width="882" alt="image" src="https://github.com/user-attachments/assets/896385d2-68b2-4a51-a38e-45c6f9fbd3bf" />

