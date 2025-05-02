#include "nunchuck.h"

struct NunchuckData parseNunchuckRaw(unsigned char *raw) {
  struct NunchuckData data;

  data.joystick_x = raw[0];
  data.joystick_y = raw[1];
  data.accelerometer_x = ((unsigned short) raw[2]) << 2 | ((raw[5] & 0b00001100) >> 2);
  data.accelerometer_y = ((unsigned short) raw[3]) << 2 | ((raw[5] & 0b00110000) >> 4);
  data.accelerometer_z = ((unsigned short) raw[4]) << 2 | ((raw[5] & 0b11000000) >> 6);
  data.c_button = (raw[5] & 0b00000010) >> 1;
  data.z_button = raw[5] & 0b0000001;

  return data;
}
