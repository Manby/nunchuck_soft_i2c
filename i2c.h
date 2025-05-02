#ifndef I2C_HEADER
#define I2C_HEADER

#include <cstddef>

struct I2CSendResult {
  int return_code;
  size_t num_sent;
};

struct I2CRequestResult {
  int return_code;
  size_t num_received;
}; 

void i2c_init(int clock_pin, int data_pin);

// Blocking send
struct I2CSendResult i2c_send(unsigned char address, unsigned char *bytes, size_t num);

// Blocking request
struct I2CRequestResult i2c_request(unsigned char address, unsigned char *dest, size_t num);

#endif // I2C_HEADER