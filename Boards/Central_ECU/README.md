# Central ECU

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
is on the current is limited to 15mA, too low to ignite one of the ignitors or open the solenoid. This dual switching design also provides some reassurance in
the case of a FET failing as they usually fail shorted, allowing the test to be terminated by the ECU regardless of a single failure.

<img width="882" alt="image" src="https://github.com/user-attachments/assets/7685a6f7-b061-4d6f-9e06-47a1be366fea" />

The connectors are another area that might be worth mentioning briefly. It is generally good practice to make sure that things physically can't be plugged in
wrong, because someone will definitly plug that 6S battery into your 2S circuit otherwise. You can see I have designed it such that the 2S and 6S input shares
the same sort of connector, but has the voltage on a different pin so if they get plugged in the wrong way around it dosn't matter. Doing this everywhere will
get impractical, for instance the on and arm switches are the same, but think about the critical places to do this and make sure you do. Either use a different
connector type or different number of pins to switch it up.

Final rant for this section, be careful with your linear voltage regulators. They will have a rated current but thats never really achievable due to thermals.
Make sure you actually do some thermal calculations and stitch the regulators through into the planes underneath with plenty of vias. And chaining the regulators
is another approach, I needed 5V but had very little 5V draw, so by inputting 5V to the 3.3V regulator you effectivly split the heat across both regulators.