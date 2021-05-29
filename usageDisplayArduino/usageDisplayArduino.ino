#include <LEDMatrixDriver.hpp>
#include "CRC.h"

// Based on example "MarqueeText.ino"
// https://github.com/bartoszbielawski/LEDMatrixDriver/blob/master/examples/MarqueeText/MarqueeText.ino
// Example written 16.06.2017 by Marko Oette, www.oette.info

// Define the ChipSelect pin for the led matrix (Dont use the SS or MISO pin of your Arduino!)
// Other pins are Arduino specific SPI pins (MOSI=DIN, SCK=CLK of the LEDMatrix) see https://www.arduino.cc/en/Reference/SPI
const uint8_t LEDMATRIX_CS_PIN = 17;

// Number of 8x8 segments you are connecting
const int LEDMATRIX_SEGMENTS = 16;

// The LEDMatrixDriver class instance
LEDMatrixDriver lmd(LEDMATRIX_SEGMENTS, LEDMATRIX_CS_PIN);

const long SERIAL_TIMEOUT = 100;
const int CHECKSUM_LENGTH = 1;
const uint8_t CHECKSUM_POLYNOM = 0x07;
const bool IGNORE_CHECKSUM = false; // For debugging :)

const int NUM_GRAPHS = 4;
const int NUM_COLUMNS_PER_GRAPH = 32;

void setup() {
  // init the display
  lmd.setEnabled(true);
  lmd.setIntensity(0);   // 0 = low, 10 = high
  lmd.clear();
  lmd.display();

  Serial.begin(57600);
  Serial.setTimeout(SERIAL_TIMEOUT);
  // flush the serial port
  while(Serial.available()){
    Serial.read();
  }
}

bool readUpdates(uint8_t numGraphs, uint8_t* output)
{
  static_assert(CHECKSUM_LENGTH == 1);
  
  uint8_t internalBuffer[numGraphs + CHECKSUM_LENGTH];
  // read the data + checksum
  auto numBytesRead = Serial.readBytes(internalBuffer, numGraphs + CHECKSUM_LENGTH);
  if (numBytesRead != numGraphs + CHECKSUM_LENGTH)
  {
    // There probably was a timeout
    return false;
  }

  if constexpr (CHECKSUM_LENGTH == 1)
  {
    auto checksum = crc8(internalBuffer, numGraphs, CHECKSUM_POLYNOM);
    if (checksum != internalBuffer[numGraphs] && !IGNORE_CHECKSUM)
    {
      // Checksum error!
      return false;
    }
  }

  memcpy(output, internalBuffer, numGraphs * sizeof(uint8_t));
  return true;
}

uint8_t bars[9] = {
  0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF
};

void loop()
{
  static uint8_t values[NUM_GRAPHS];

  bool gotUpdate = readUpdates(NUM_GRAPHS, values);

  if (gotUpdate)
  {
    lmd.scroll(LEDMatrixDriver::scrollDirection::scrollRight);
    
    for (uint8_t graphIndex = 0; graphIndex < NUM_GRAPHS; graphIndex++)
    {
      // Compute the new colum value
      float newValue = static_cast<float>(values[graphIndex]) / 255.0f;
      uint8_t newBar = bars[static_cast<uint8_t>(roundf(newValue * 8.0f))];

      // and set it
      lmd.setColumn(graphIndex * NUM_COLUMNS_PER_GRAPH, newBar);
    }

    lmd.display();
  }
}
