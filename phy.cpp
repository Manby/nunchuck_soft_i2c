#include "phy.h"

static SemaphoreHandle_t phy_mutex;

static hw_timer_t *phy_hw_timer = NULL;
static int phy_return_code;
static TaskHandle_t phy_owner_task_handle;

#define PHY_BYTES_BUF_SIZE 1024
static unsigned char phy_bytes_buf [PHY_BYTES_BUF_SIZE];
static size_t phy_bytes_buf_len;

// 100KHz Clock
#define PHY_CLOCK_FREQUENCY 100000
#define PHY_CLOCK_SLOWDOWN 100

//#define PHY_IGNORE_NACK

static int phy_data_pin;
static int phy_clock_pin;

static int phy_clock;

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

static PhyI2CMode phy_mode;
static size_t phy_num_bytes_requested;
static PhyI2CState phy_state;
static int phy_next_bit;
static int phy_next_byte;

static void dataLow() {
  digitalWrite(phy_data_pin, 0);
}

static void dataHigh() {
  digitalWrite(phy_data_pin, 1);
}

static int dataRead() {
  return digitalRead(phy_data_pin);
}

static void clockLow() {
  phy_clock = 0;
  digitalWrite(phy_clock_pin, phy_clock);
}

static void clockHigh() {
  phy_clock = 1;
  digitalWrite(phy_clock_pin, phy_clock);
}

static void clockToggle() {
  phy_clock ^= 1;
  digitalWrite(phy_clock_pin, phy_clock);
}

static void IRAM_ATTR halftick() {
  
  /*
  Serial.print("Tick ");
  if (phy_clock == 0) Serial.print("/\\");
  if (phy_clock == 1) Serial.print("\\/");
  Serial.print(" : ");

  switch (phy_state) {
    case START_0:
      Serial.print("START_0");
      break;

    case START_1:
      Serial.print("START_1");
      break;

    case START_2:
      Serial.print("START_2");
      break;

    case START_3:
      Serial.print("START_3");
      break;

    case PUSH_BIT:
      Serial.print("PUSH_BIT");
      Serial.print(" ");
      Serial.print(phy_bytes_buf_len);
      Serial.print(" ");
      Serial.print(phy_next_byte);
      break;

    case WAIT_LISTEN_ACK:
      Serial.print("WAIT_LISTEN_ACK");
      break;

    case LISTEN_ACK:
      Serial.print("LISTEN_ACK");
      break;

    case READ_BIT:
      Serial.print("READ_BIT");
      break;

    case TRY_SEND_ACK:
      Serial.print("TRY_SEND_ACK");
      break;

    case SENT_ACK:
      Serial.print("SENT_ACK");
      break;

    case STOP_0:
      Serial.print("STOP_0");
      break;

    case STOP_1:
      Serial.print("STOP_1");
      break;

    case STOP_2:
      Serial.print("STOP_2");
      break;
  }
  Serial.println();
  Serial.flush();
  */
  

  // State machine, driven by hardware timer

  switch (phy_state) {
    case START_0:
      dataLow();
      phy_state = START_1;
      break;

    case START_1:
      phy_state = START_2;
      break;
    
    case START_2:
      clockLow();
      phy_state = START_3;
      break;
    
    case START_3:
      clockHigh();
      phy_state = PUSH_BIT;
      phy_next_bit = 7;
      phy_next_byte = 0;
      break;
    
    case PUSH_BIT:


      if (phy_clock == 1) {
        // Will be falling edge; push bit
        unsigned char byte = phy_bytes_buf[phy_next_byte];
        int bit = (byte >> phy_next_bit) & 1;
        if (bit == 0) {
          dataLow();
          //Serial.println("PUSH 0");
        } else {
          dataHigh();
          //Serial.println("PUSH 1");
        }
        phy_next_bit--;
        
      } else {
        // Rising edge
        if (phy_next_bit == -1) {
          // We've pushed a byte, so now listen for the ACK
          phy_next_bit = 7;
          phy_next_byte++;
          phy_state = WAIT_LISTEN_ACK_0;
        }
      }

      clockToggle();
      break;

    case WAIT_LISTEN_ACK_0:
      // The falling edge at the beginning of the ACK cycle
      clockToggle();
      pinMode(phy_data_pin, INPUT_PULLUP);
      phy_state = WAIT_LISTEN_ACK_1;
      break;
    
    case WAIT_LISTEN_ACK_1:
      // The rising edge during the ACK cycle
      clockToggle();
      if (phy_clock == 0) phy_state = LISTEN_ACK;
      break;
    
    case LISTEN_ACK: {
      
      // Just after the ACK cycle; check for the ACK
      int read = dataRead();
      int ack = read == 1 ? 0 : 1;
      Serial.print("READ: ");
      Serial.println(read);
      Serial.print("ACK: ");
      Serial.println(ack);

      #ifndef PHY_IGNORE_NACK
      if (!ack) {
        // Terminate with NACK error
        phy_return_code = 1;
        phy_state = STOP_0;
        break;
      }
      #endif
      
      if (phy_mode == SENDING) {
        if (phy_next_byte == phy_bytes_buf_len) {
          // We've pushed all the bytes, so terminate
          phy_return_code = 0;
          phy_state = STOP_0;
        } else {
          // Push next byte
          phy_state = PUSH_BIT;
          pinMode(phy_data_pin, OUTPUT);
        }
      } else {
        phy_state = READ_BIT;
      }
      clockToggle();
      break;
    }

    case READ_BIT:
      if (phy_clock == 0) {
        // Falling edge; read bit
        int bit = dataRead();
        if (bit) {
          phy_bytes_buf[phy_next_byte] |= 1 << phy_next_bit;
        } else {
          phy_bytes_buf[phy_next_byte] &= !(1 << phy_next_bit);
        }
        phy_next_bit--;

        if (phy_next_bit == -1) {
          // We've received a byte, so now send an ACK if we can accept more
          phy_next_bit = 7;
          phy_next_byte++;
          phy_state = TRY_SEND_ACK;
          pinMode(phy_data_pin, OUTPUT);
        }
      }
      clockToggle();

      break;
    
    case TRY_SEND_ACK:
      if (phy_next_byte == phy_num_bytes_requested+1 || phy_next_byte == PHY_BYTES_BUF_SIZE) {
        // Received all requested bytes or buffer is full, so send NACK to signal sender to stop
        dataHigh();
        // Terminate with appropriate error code
        phy_return_code = phy_next_byte == phy_num_bytes_requested+1 ? 0 : 2;
        phy_state = STOP_0;
      } else {
        // Send ACK
        dataLow();
        phy_state = SENT_ACK;
      }
      clockToggle();
      break;
    
    case SENT_ACK:
      clockToggle();
      phy_state = READ_BIT;
      pinMode(phy_data_pin, INPUT_PULLUP);
      break;
    
    case STOP_0:
      clockHigh();
      pinMode(phy_data_pin, OUTPUT);
      phy_state = STOP_1;
      break;

    case STOP_1:
      phy_state = STOP_2;
      break;
    
    case STOP_2:
      dataHigh();

      // Delete driving hardware timer
      timerEnd(phy_hw_timer);
      // Notify owning task
      BaseType_t xHigherPriorityTaskWasWoken = pdFALSE;
      vTaskNotifyGiveFromISR(phy_owner_task_handle, &xHigherPriorityTaskWasWoken);
      portYIELD_FROM_ISR(xHigherPriorityTaskWasWoken);

      break;
  }
}


// public

void phy_clearBuf() {
  phy_bytes_buf_len = 0;
}

void phy_init(int clock_pin, int data_pin) {
  phy_mutex = xSemaphoreCreateMutex();

  phy_clock_pin = clock_pin;
  phy_data_pin = data_pin;
  
  pinMode(phy_clock_pin, OUTPUT);
  pinMode(phy_data_pin, OUTPUT);
  //!!!

  phy_clearBuf();
}

void phy_acquireLine() {
  xSemaphoreTake(phy_mutex, portMAX_DELAY);
}

void phy_releaseLine() {
  xSemaphoreGive(phy_mutex);
}

int phy_pushBytes(unsigned char *bytes, size_t num) {
  size_t i = 0;
  while (i < num && phy_bytes_buf_len < PHY_BYTES_BUF_SIZE) {
    phy_bytes_buf[phy_bytes_buf_len] = bytes[i];
    phy_bytes_buf_len++;
    i++;
  }
  return i;
}

int phy_beginSend(TaskHandle_t owner_task_handle) {
  phy_owner_task_handle = owner_task_handle;
  phy_return_code = -1;

  phy_state = START_0;
  phy_mode = SENDING;
  
  dataHigh();
  clockHigh();

  phy_hw_timer = timerBegin(2*PHY_CLOCK_FREQUENCY);  // Tick at rate of 2MHz
  if (phy_hw_timer == NULL) {
    Serial.println("Failed to create timer");
    return 1;
  }
  Serial.println("Created timer");

  timerAttachInterrupt(phy_hw_timer, &halftick);
  timerAlarm(phy_hw_timer, PHY_CLOCK_SLOWDOWN, true, 0);  // timer, alarm goes off once counter hits 1 tick, then it resets (autoreloads) counter to 0
  Serial.println("Set timer alarm");

  return 0;
}

int phy_beginRequest(TaskHandle_t owner_task_handle, size_t num_bytes_requested) {
  phy_owner_task_handle = owner_task_handle;
  phy_return_code = -1;

  phy_state = START_0;
  phy_mode = REQUESTING;
  phy_num_bytes_requested = num_bytes_requested;
  
  dataHigh();
  clockHigh();

  phy_hw_timer = timerBegin(2*PHY_CLOCK_FREQUENCY);  // Tick at rate of 2MHz
  if (phy_hw_timer == NULL) {
    Serial.println("Failed to create timer");
    return 1;
  }
  Serial.println("Created timer");

  timerAttachInterrupt(phy_hw_timer, &halftick);
  timerAlarm(phy_hw_timer, PHY_CLOCK_SLOWDOWN, true, 0);  // timer, alarm goes off once counter hits 1 tick, then it resets (autoreloads) counter to 0
  Serial.println("Set timer alarm");

  return 0;
}

int phy_getReturnCode() {
  return phy_return_code;
}

int phy_getNumBytesCounted() {
  return phy_next_byte - 1;
}

int phy_getRequestedData(unsigned char *dest) {
  int num_bytes_received = phy_next_byte - 1;
  for (int i = 0; i < num_bytes_received; i++) {
    dest[i] = phy_bytes_buf[i+1];
  }
  return num_bytes_received;
}
