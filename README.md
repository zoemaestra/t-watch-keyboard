Arduino code for the LilyGo [T-WATCH-Keyboard](https://lilygo.cc/products/watch-keyboard-c3-v1-0) that outputs to both serial and I2C for use in other projects.

I would STRONGLY advise dumping the firmware to a backup if it is still stock. There are tutorials for backing up/dumping ESP32 firmwares online.

To use this board, you will need an external USB to serial adapter that works at 3.3v. I would personally recommend something using a CH9102. A lot of cheap AliExpress boards that say they autodetect 3.3v don't do this well and can burn out the BBQ10 keyboard's backlights.

In Arduino IDE, select your board as the TTGO T-OI PLUS RISC-V ESP32-C3, since this is the closest board in the list to what you're working with. If it doesn't appear, you'll need to add the ESP32 board list to the IDE. There's plenty of tutorials online for this.

In the IDE with the TTGO T-OI PLUS RISC-V ESP32-C3 selected and your board with serial adapter connected, make sure you have the following options set:

![image](https://github.com/user-attachments/assets/a285c2cf-c099-4681-ad4e-7b14168d294f)

You may have issues uploading and running the code if these are not set!

Once this is done, you should be able to upload the code without issue.
