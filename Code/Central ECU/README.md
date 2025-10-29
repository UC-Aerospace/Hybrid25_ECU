# Central ECU Code

If its your job in 2026 to work on this, good luck. Hopefully the base here is firm enough to build on but the code was
undeniably rushed. To get you started though:

### Layout

* All the user code is in the src/ or include/ folders. Everything else is the STM HAL primarily.
* The Core/Src/main.c file is the main code entry, this has been patched to call src/app.c after it sets everything up.
* The CMakeLists might have to be changed from time to time as well.
* A seriously major battle was fought over getting the SD card working.
Some of the FATFS files were modified in other places, the stuff I added is mainly in Middlewares/Third_Party/FatFs. I really hope no one else has to touch that.

The ECU_Mainboard.ioc can be opened in cubemx to configure most of the HAL options.

### Gotchas

1) The code is currently locked to a specific MCU. This is because I have a habbit of flashing the wrong code when working with
multiple tabs. It checks the UID of the MCU on entry to app.c and will abort if it dosn't match.
2) The code is set up with the intention to support more boards being added modularly but there would be at least a bit of work that has
to go into tweaking the communication protocols I suspect.
3) The SD card is slooooow. It can sometimes take upto 100ms, generally on a flush but it can be hard to predict. This is the reason a lot
of the buffers are so large.
4) The timebase has been changed from a systick interrupt to one of the hardware timers. This still drifts quite quickly (1ms+ every 10 seconds)
compared to the other boards. Switching all from the internal occilator to an external one by populating the part across all boards would improve this.

### Tasks

To see all the tasks head into the app.c file. The timings allocated to these could definitly be optimised but these were pretty much just what worked.
For a quick explaination of each:

#### task_poll_can_handlers()

CAN operates on a two level system. All frames have a priority set by the ID, priority 0 and 1 (error and heartbeat) and serviced in ISR context,
while priorities 2 and 3 (commands and data) are added to a buffer by the ISR and it returns. This buffer is then polled from the can_handler module,
which will decode it and take action based on the contents.

#### task_poll_rs422()

Similarly to how the CAN works the RS422 again uses a hardware and handler approach. This time as the RS422 is really just a UART peripheral the RX fifo
is serviced with DMA into a large circular buffer. This polling function then checks the circular buffer, and after checking the length byte if a full
frame exists in the buffer it will compute a CRC to validate it before decoding and actioning.

#### task_poll_battery()

Pretty simple this one, both batteries are connected to a ADC pin through a voltage divider so a reading is taken, converted to a voltage and then to a
percentage with a lookup table. This updates the OLED screen as well as sending the percentages over RS422.

#### test_servo_poll()

This is the debug USB interface and could be safely disabled for tests. This exposes a command line input over the USB serial port, both for moving the
servos and setting the internal real-time clock that is used in logs.

#### task_flush_sd_card()

When a write happens to the SD card it dosn't really occur. The SD card needs to be flushed to retain this after power loss, so we have to flush it every
now and again. Be warned, flushes can be slooow.

#### fsm_tick()

The main finite state machine, run the next iteration of it.

#### can_service_tx_queue()

Similarly to the revieve this will place messages from the software queue into the hardware tx queue. Upto 3 messages can be in the hardware buffer at once.

#### task_send_heartbeat()

Sends a heartbeat both over CAN broadcast and RS422.

#### spicy_send_status_update()

This is a really dumb name, but "spicy" refers to anything with the ignitors and solenoid which are on the "spicy" circuit. The RS422 frame for this was added
late, hence having its own task. It just sends a bunch of status information.
