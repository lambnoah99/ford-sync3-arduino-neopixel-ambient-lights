#include <SPI.h>
#include <mcp_can.h>
#include <Adafruit_NeoPixel.h>

// CAN-IDs
#define PLATFORM C1MCA

#if PLATFORM == C1MCA
  #define AMBI_REC_PID 0x3DA    // APIM->BCM (Arduino)
  #define AMBI_CMD_PID 0x3E3    // BCM (Arduino) -> APIM
#else
  // Maybe Other Platforms with different IDs?
  #define AMBI_REC_PID 0x3DA
  #define AMBI_CMD_PID 0x3E3
#endif

// Can Connection
MCP_CAN CAN(10);  // MCP2515 CS pin
unsigned long canId;
uint8_t len = 0;
uint8_t buf[8];



void setup() {
  Serial.begin(9600);

  while (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) != CAN_OK) {
    Serial.println("Couldn't connect to MCP, retrying...");
    delay(500);
  }
  Serial.println("Connected to MCP2515");

  CAN.setMode(MCP_NORMAL); // Normal Mode so MCP2515 sends ACK Messages
}

void loop() {
  if(CAN.checkReceive() == CAN_MSGAVAIL) {
    CAN.readMsgBuf(&canId, &len, buf);

      switch(canId) {
        case AMBI_REC_PID:
          commandedColorIndex = buf[0];
          commandedBrightness = buf[1];
          Serial.print(commandedColorIndex);
          Serial.print(", ");
          Serial.println(commandedBrightness);

          send_light_state();
          break;
      }
    }
}

// Sends Ambilight-State to APIM so it reflects in the ui
void send_light_state() {
  uint8_t data[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, commandedColorIndex, commandedBrightness, 0x00 };
  CAN.sendMsgBuf(AMBI_CMD_PID, 0, 8, data);
}