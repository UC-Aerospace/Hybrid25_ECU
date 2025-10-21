# Hybrid Engine Control Unit

This repo contains the hardware and firmware for the 2025 Hybrid Rocket Engine FYP test stand electronics.

The hardware consists of three seperate modules, the central ECU alongside a servo and ADC peripheral. Central to peripheral
communication uses CAN-FD and is designed to be modular, while communications to a remote interface unit is done over RS422.
For more information on the RIU it can be found [here](https://github.com/UC-Aerospace/Hybrid25_RIU).

<img width="882" alt="image" src="https://github.com/user-attachments/assets/0b086ce3-42a3-48b5-a7de-66e9607cbf3b" />

## Hardware

...

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

