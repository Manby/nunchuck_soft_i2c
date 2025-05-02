#include "i2c.h"
#include "nunchuck.h"

static unsigned char raw_buffer[6];

void setup() {
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(1000));
  
  /*
  my_bytes[0] = 0xC3;
  my_bytes[1] = 0xD3;
  my_bytes[2] = 0x1F;
  my_bytes[3] = 0x44;

  Serial.println("Initialising I2C...");
  i2c_init(12, 13);

  Serial.println("Beginning send...");
  I2CSendResult result = i2c_send(target_addr, my_bytes, 4);
  //vTaskDelay(pdMS_TO_TICKS(1000));

  Serial.print("Return code: ");
  Serial.println(result.return_code);
  Serial.print("Num bytes sent: ");
  Serial.println(result.num_sent);
  Serial.flush();
  */

  Serial.println("Initialising I2C...");
  i2c_init(22, 21);

  unsigned char handshake[2];
  handshake[0] = 0xf0;
  handshake[1] = 0x55;
  Serial.println("Awaiting user prompt...");
  while (!(Serial.read() == 'g')) {}
  Serial.println("Performing handshake...");
  I2CSendResult result = i2c_send(0xA4, handshake, 2);
  Serial.print("Hanshake part 1 returned: ");
  Serial.print(result.return_code);
  Serial.print(", ");
  Serial.print(result.num_sent);
  Serial.println(" bytes sent");

  /*
  unsigned char handshake[4];
  handshake[0] = 0xF0;
  handshake[1] = 0x55;
  handshake[2] = 0xFB;
  handshake[3] = 0x00;
  Serial.println("Awaiting user prompt...");
  while (!(Serial.read() == 'g')) {}
  Serial.println("Performing handshake...");
  I2CSendResult result = i2c_send(0xA4, handshake, 2);
  Serial.print("Hanshake part 1 returned: ");
  Serial.println(result.return_code);
  result = i2c_send(0xA4, handshake+2, 2);
  Serial.print("Hanshake part 2 returned: ");
  Serial.println(result.return_code);
  */

  //vTaskDelete(NULL);
}

void loop() {
  Serial.println("Awaiting user prompt...");
  while (!(Serial.read() == 'g')) {}

  Serial.println("Beginning request...");
  I2CRequestResult result = i2c_request(0xA5, raw_buffer, 6);
  //vTaskDelay(pdMS_TO_TICKS(1000));

  Serial.print("Return code: ");
  Serial.println(result.return_code);
  Serial.print("Num bytes received: ");
  Serial.println(result.num_received);

  /*
  for (int i = 0; i < 6; i++) {
    Serial.println(my_bytes[i]);
  }
  */

  struct NunchuckData data = parseNunchuckRaw(raw_buffer);
  Serial.print("Joystick X: ");
  Serial.println(data.joystick_x);
  Serial.print("Joystick Y: ");
  Serial.println(data.joystick_y);
  Serial.print("Accelerometer X: ");
  Serial.println(data.accelerometer_x);
  Serial.print("Accelerometer Y: ");
  Serial.println(data.accelerometer_y);
  Serial.print("Accelerometer Z: ");
  Serial.println(data.accelerometer_z);
  Serial.print("C-button: ");
  Serial.println(data.c_button);
  Serial.print("Z-button: ");
  Serial.println(data.z_button);
}
