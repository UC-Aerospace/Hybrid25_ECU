# Hybrid Engine Control Unit

This repo contains the hardware and firmware for the 2025 Hybrid Rocket Engine FYP test stand electronics.

The hardware consists of three seperate modules, the central ECU alongside a servo and ADC peripheral. Central to peripheral
communication uses CAN-FD and is designed to be modular, while communications to a remote interface unit is done over RS422.
For more information on the RIU it can be found [here](https://github.com/UC-Aerospace/Hybrid25_RIU).

<img width="882" alt="image" src="https://github.com/user-attachments/assets/0b086ce3-42a3-48b5-a7de-66e9607cbf3b" />

<img width="300" src="https://github.com/user-attachments/assets/c9186404-064c-464e-af78-75c3a6b4d8b1" />
<img width="300" src="https://github.com/user-attachments/assets/cef47ce6-662b-40f6-8056-12d7367314a3" />
<img width="300" src="https://github.com/user-attachments/assets/f58a780b-ff3e-45d4-954a-3c8383550991" />

<img width="900" alt="Hotfire" src="https://github.com/user-attachments/assets/00e49bff-133c-4680-9e74-af2937d0e3df" />


## Hardware

First thing to note, make sure you read the [issue!](https://github.com/UC-Aerospace/Hybrid25_ECU/issues/1).

There are issues with these designs, keep them in mind for any future development.

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

Theres a bit of an explaination for the actual hardware design ethos to get you started in the Central ECU board directory. Otherwise between the
boards the main thing to talk about is connectors, comms and power.

The connectors we used on the peripheral boards were a compromise. JST's should never really be used where they might see forces on the wires,
but they are also cheap and the budget was getting out of hand. We mitigated this when testing by externally strain relieving all wires connecting
to both peripheral boards. Also when thinking about connectors try to keep in mind that if something can be plugged into the wrong place, it will
be plugged into the wrong place. Try to keep connectors dissimilar where you can, at least where damage would occur if they went the other way.

Comms is over CAN-FD for the peripheral to central communication and RS422 for central to RIU communication. The descision for this came because
CAN sounded cool to play with and RS422 seemed like the easiest way to get comms upto a couple hundred meters away. Both of these descisions played
out pretty well, with software headaches for both but nothing too serious. The CAN just uses premade serial DB9 cables, as these are cheap and easy
to get hold of. DB9 connectors are really solid and lock well, plus are rated to about 3A per pin. For the link to the RIU we just used generic CAT5
cable, two of the pairs are used for RS422 with another used for the ESTOP signal lines. The plastic connectors on either end are a weakspot, it would
have been nice to use something like etherCON connectors but again, expensive.

Power is something that got changed up a few times. We use lipo batteries as the power source, the ECU has two and theres an extra for the servos. The
ECU has a 2S for general functions, which is also passed to the peripherals which they use. The servo board uses this for the 2 7.4V servos remaining also.
The other battery in the ECU, the 6S, is used for the solenoids and ignitors. This is the power that the ESTOP is inline with, cutting that will abort the
test due to the normally closed solenoid. Finally as we had to add 24V servos there is a seperate 6S battery dedicated to them during the test. This is suboptimal
as we have no insight into the level of it remotely during a test and the standby draw is quite high. If done again just plug this directly into the servo board
and move all servos to 24V. 

## Software

I have yet to find a way to make code sound interesting, and I don't think I'm going to manage it here either. I guess the main thing to point out is there is
a lot of code. Certainly more than I expected going into this, and it took significantly longer than I expected as well. Main word of advice with the software,
it is very easy to get yourself stuck in a trap of rushing rubbish code out.

<img width="622" height="343" alt="image" src="https://github.com/user-attachments/assets/14591506-8dcb-4b1f-93ca-f814919fe7ff" />

There was an extended period this year when we were perpetually two weeks away from testing, and it probably shows in the code. In the end the hold up was with
another system, but the code was still blasted out dangerously fast regardless. Give yourself time in the beginning and temper the goals, its easy to add more
later but hard to strip them away with testing looming.

For some more specifics on the actual code I'll leave a quick README for the specifics under each boards code. Heres a quick rant from the EOY report though.

*All code and hardware design has been done in a single git repository. This was poor practice, in future break the code up into separate repositories, 
the only reason we could make a single repository work is because only one person was working on it. One of the main motivators for doing it this way was to enable
code reuse between the boards, for example the CAN module is common between all three and uses a symbolic linked folder. In future it would be worth investigating
using git submodules for the same thing. VS Code was used as the IDE for the ECU and peripherals, while STM32CubeIDE was used for the RIU. It is **strongly** recommended
that future teams do not use CubeIDE, while IDE’s are a preference eclipse is objectively terrible. The ST support for VS Code is good enough now that after installing
the extensions and CubeMX the development experience is much better.*

*The general layout for each board is given below:*

<img width="345" height="206" alt="image" src="https://github.com/user-attachments/assets/326ed695-d3f7-4e9c-9d04-b41a27f27702" />



### Finite State Machine Diagrams

#### Central ECU

<img width="882" alt="image" src="https://github.com/user-attachments/assets/ce11024d-72e6-4584-9be2-6bc21fa01c2f" />

#### Servo Module

<img width="882" alt="image" src="https://github.com/user-attachments/assets/f395c7c0-076e-4b11-afe0-9f1687392bde" />

### ADC Module

The ADC module has no inherent state machine, it just reads sensors and sends these.

## Operational

### Safety

Safety has been one of the major concerns with this project. To try and mitigate some of those concerns the main solenoid
is normally closed, such that within 350 milliseconds of cutting the power it fully shuts. The ignitors, being potentially
explosive, were also identified as high risk. Therefore, both these items are on a seperate circuit. Two air gaps exist
in this loop, one E-Stop positioned inline and outside the test container, another at the RIU and connected through a relay
to limit current loss.

<img width="882" alt="image" src="https://github.com/user-attachments/assets/7a1bfbcf-406d-4c0b-83eb-f71a759fa2aa" />

<img width="882" alt="image" src="https://github.com/user-attachments/assets/896385d2-68b2-4a51-a38e-45c6f9fbd3bf" />

