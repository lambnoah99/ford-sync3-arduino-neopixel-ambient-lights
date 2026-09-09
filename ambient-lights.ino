#include <SPI.h>
#include <mcp_can.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN 2
#define NUM_PIXELS 11
#define TRANSITION_TIME 2000 / NUM_PIXELS
#define FADE_TIME_MS 20       // Time in ms between Dim steps, shouldn't be too long to prevent dropped packets 

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

// Neopixel Connection
Adafruit_NeoPixel pixels(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
uint8_t commandedColorIndex = 1;
uint8_t commandedBrightness = 255;
uint8_t actualColorIndex = 1;
uint8_t actualBrightness = 50;

uint32_t colors[8] = {
  0,        // Black
  65535,    // Ice Blue
  13395456, // Orange
  255,      // Blue
  16711680, // Red
  65535,    // Green
  200,      // Navy
  13369446  // Purple
};

void setup() {
  Serial.begin(9600);
  pixels.begin();

  while (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) != CAN_OK) {
    Serial.println("Couldn't connect to MCP, retrying...");
    delay(500);
  }
  Serial.println("Connected to MCP2515");

  // Filter out unneeded CAN packets
  mcp2515.setFilterMask(MCP2515::MASK0, false, 0x7FF);
  mcp2515.setFilter(MCP2515::RXF0, false, 0x3DA);
  CAN.setMode(MCP_NORMAL); // Normal Mode so MCP2515 sends ACK Messages

  // Startup Animation
  pixels.clear();
  delay(1000);
  for(int i=0; i<NUM_PIXELS; ++i) {
    pixels.setPixelColor(i, colors[actualColorIndex]);
    pixels.show();

    delay(TRANSITION_TIME);
  }
  for(int i=0; i<255; ++i) {
    ++actualBrightness;
    pixels.setBrightness(actualBrightness);
    pixels.show();

    delay(TRANSITION_TIME * 2);
  }
  


  delay(2000);  
}

void loop() {
  if(CAN.checkReceive() == CAN_MSGAVAIL) {
    CAN.readMsgBuf(&canId, &len, buf);

      switch(canId) {
        case AMBI_REC_PID:
          commandedColorIndex = buf[0];
          commandedBrightness = buf[1];

          send_light_state();
          break;
      }
    }

  // Smooth Brightness Transition
  if(commandedBrightness > actualBrightness) {
    actualBrightness++;
    delay(FADE_TIME_MS);
    pixels.show();
  } else if(commandedBrightness < actualBrightness) {
    actualBrightness--;
    delay(FADE_TIME_MS);
    pixels.show();
  }

  // Only render color if it actually changed
  if(commandedColorIndex != actualColorIndex) {
    actualColorIndex = commandedColorIndex;
    pixels.fill(colors[actualColorIndex], 0, NUM_PIXELS);
    pixels.show();
  }

}

// Sends Ambilight-State to APIM so it reflects in the ui
void send_light_state() {
  uint8_t data[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, commandedColorIndex, commandedBrightness, 0x00 };
  CAN.sendMsgBuf(AMBI_CMD_PID, 0, 8, data);
}