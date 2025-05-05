# nunchuck_soft_i2c
Software implementation of the I2C protocol to allow an ESP32 chip to communicate with a Wii Nunchuck.

This project is implemented as a protocol stack:

| Layer | Description |
| -------- | ------- |
| Application | Uses the nunchuck data to do something interesting. |
| Nunchuck | Parses received I2C data into readable nunchuck button information according to the Nintendo Wii protocol. |
| I2C | Software implementation of the I2C protocol; leverages the physical bus to communicate. |
| Physical | Drives the data and clock bus signals. Also enforces blocking single-thread access to the bus. |

![Thumbnail](https://github.com/Manby/nunchuck_soft_i2c/blob/main/thumbnail.jpg?raw=true)
