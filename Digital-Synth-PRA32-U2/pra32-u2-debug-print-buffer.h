#pragma once

// Debug Print Buffer (for printing to USB Serial from the secondary core)
//
// - The USB stack runs on the primary core, so USB Serial is accessed only on the primary core
// - The secondary core (writer) writes to this lock-free single-producer single-consumer ring buffer,
//   and the primary core (reader) transfers the data to USB Serial by transfer_to()
// - Neither the writer nor the reader blocks: write() drops data if the buffer is full,
//   and transfer_to() writes only as much as the output can accept now

#include "pra32-u2-common.h"
#include <Arduino.h>
#include <hardware/sync.h>

class PRA32_U2_DebugPrintBuffer : public Print {
  static const uint32_t BUFFER_SIZE = 256;  // Must be a power of 2

  uint8_t           m_buffer[BUFFER_SIZE];
  volatile uint32_t m_write_count;  // Updated only by the writer (free-running)
  volatile uint32_t m_read_count;   // Updated only by the reader (free-running)

public:
  PRA32_U2_DebugPrintBuffer() : m_buffer(), m_write_count(0), m_read_count(0) {}

  // Writer
  virtual int availableForWrite() {
    return BUFFER_SIZE - (m_write_count - m_read_count);
  }

  // Writer
  virtual size_t write(uint8_t c) {
    uint32_t write_count = m_write_count;
    if ((write_count - m_read_count) >= BUFFER_SIZE) {
      return 0;
    }

    m_buffer[write_count & (BUFFER_SIZE - 1)] = c;
    __dmb();  // Make the data visible before the count
    m_write_count = write_count + 1;
    return 1;
  }

  using Print::write;

  // Reader
  template <typename T>
  void transfer_to(T& output) {
    uint32_t read_count = m_read_count;
    uint32_t count = m_write_count - read_count;
    if (count == 0) {
      return;
    }

    __dmb();  // Read the data after the count

    int available = output.availableForWrite();
    if (available <= 0) {
      return;
    }

    if (count > static_cast<uint32_t>(available)) {
      count = available;
    }

    uint8_t chunk[BUFFER_SIZE];
    for (uint32_t i = 0; i < count; i++) {
      chunk[i] = m_buffer[(read_count + i) & (BUFFER_SIZE - 1)];
    }

    __dmb();  // Finish reading the data before releasing the space
    m_read_count = read_count + count;

    output.write(chunk, count);  // The data is dropped if the host is not connected
    output.flush();
  }
};
