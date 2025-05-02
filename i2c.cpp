#include "i2c.h"
#include "phy.h"

void i2c_init(int clock_pin, int data_pin) {
  phy_init(clock_pin, data_pin);
}

// Blocking send
struct I2CSendResult i2c_send(unsigned char address, unsigned char *bytes, size_t num) {
  phy_acquireLine();

  phy_clearBuf();
  phy_pushBytes(&address, 1);
  size_t num_try_send = phy_pushBytes(bytes, num);
  phy_beginSend(xTaskGetCurrentTaskHandle());
  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

  int return_code = phy_getReturnCode();
  size_t num_bytes_sent = phy_getNumBytesCounted();

  phy_releaseLine();

  return {return_code, num_bytes_sent};
}

// Blocking request
struct I2CRequestResult i2c_request(unsigned char address, unsigned char *dest, size_t num) {
  phy_acquireLine();

  phy_clearBuf();
  phy_pushBytes(&address, 1);
  phy_beginRequest(xTaskGetCurrentTaskHandle(), num);
  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

  int return_code = phy_getReturnCode();
  size_t num_bytes_received = phy_getNumBytesCounted();

  phy_getRequestedData(dest);

  phy_releaseLine();

  return {return_code, num_bytes_received};
}