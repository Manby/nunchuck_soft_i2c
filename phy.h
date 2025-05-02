#ifndef PHY_HEADER
#define PHY_HEADER

#include "Arduino.h"

/*
typedef enum {
  REQUESTING,
  SENDING
} PhyI2CMode;

typedef enum {
  START_0,
  START_1,
  START_2,
  START_3,
  PUSH_BIT,
  WAIT_LISTEN_ACK_0,
  WAIT_LISTEN_ACK_1,
  LISTEN_ACK,
  READ_BIT,
  TRY_SEND_ACK,
  SENT_ACK,
  STOP_0,
  STOP_1,
  STOP_2
} PhyI2CState;

void dataLow();
void dataHigh();
int dataRead();

void clockLow();
void clockHigh();
void clockToggle();

void IRAM_ATTR halftick();
*/

void phy_clearBuf();

void phy_init(int clock_pin, int data_pin);

void phy_acquireLine();

void phy_releaseLine();

int phy_pushBytes(unsigned char *bytes, size_t num);

int phy_beginSend(TaskHandle_t owner_task_handle);

int phy_beginRequest(TaskHandle_t owner_task_handle, size_t num_bytes_requested);

int phy_getReturnCode();

int phy_getNumBytesCounted();

int phy_getRequestedData(unsigned char *dest);

#endif // PHY_HEADER