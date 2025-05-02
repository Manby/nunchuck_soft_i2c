#ifndef NUNCHUCK_HEADER
#define NUNCHUCK_HEADER

struct NunchuckData {
  unsigned char joystick_x;
  unsigned char joystick_y;
  unsigned short accelerometer_x;
  unsigned short accelerometer_y;
  unsigned short accelerometer_z;
  unsigned char c_button;
  unsigned char z_button;
};

struct NunchuckData parseNunchuckRaw(unsigned char *raw);

#endif // NUNCHUCK_HEADER