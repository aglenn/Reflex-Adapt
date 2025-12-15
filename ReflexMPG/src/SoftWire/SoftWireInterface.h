// Simple I2C/Wire-like interface so multiple software buses can share the same type
// for NintendoExtensionCtrl.
#pragma once

#include "Stream.h"

class SoftWireInterface : public Stream {
public:
  virtual void begin(void) = 0;
  virtual void end(void) = 0;
  virtual void setClock(uint32_t) = 0;

  virtual void beginTransmission(uint8_t address) = 0;
  virtual void beginTransmission(int address) = 0;
  virtual uint8_t endTransmission(uint8_t sendStop) = 0;
  virtual uint8_t endTransmission(void) = 0;

  virtual size_t write(uint8_t data) = 0;
  virtual size_t write(const uint8_t *data, size_t quantity) = 0;

  virtual uint8_t requestFrom(uint8_t address, uint8_t quantity,
                              uint32_t iaddress, uint8_t isize, uint8_t sendStop) = 0;
  virtual uint8_t requestFrom(uint8_t address, uint8_t quantity, uint8_t sendStop) = 0;
  virtual uint8_t requestFrom(int address, int quantity, int sendStop) = 0;
  virtual uint8_t requestFrom(uint8_t address, uint8_t quantity) = 0;
  virtual uint8_t requestFrom(int address, int quantity) = 0;

  virtual int available(void) = 0;
  virtual int read(void) = 0;
  virtual int peek(void) = 0;
  virtual void flush(void) = 0;
};
