#pragma once

#include <stdio.h>
#include "./Digital-Synth-PRA32-U2/pra32-u2-common.h"

class PRA32_U2_WAVFileOut {
  FILE*    m_file;
  uint32_t m_max_size;
  uint32_t m_data_size;
  boolean  m_closed;

public:
  PRA32_U2_WAVFileOut()
  : m_file()
  , m_max_size()
  , m_data_size()
  , m_closed()
  {}

  INLINE void open(const char* path, uint16_t sec) {
    m_file = fopen(path, "wb");
    fwrite("RIFF", 1, 4, m_file);
    fwrite("\x00\x00\x00\x00", 1, 4, m_file);
    fwrite("WAVE", 1, 4, m_file);
    fwrite("fmt ", 1, 4, m_file);
    fwrite("\x10\x00\x00\x00", 1, 4, m_file);
    fwrite("\x01\x00\x02\x00", 1, 4, m_file);
    uint32_t a[1] = {SAMPLING_RATE};
    fwrite(a, 4, 1, m_file);
    a[0] = {SAMPLING_RATE * 6};
    fwrite(a, 4, 1, m_file);
    fwrite("\x06\x00\x18\x00", 1, 4, m_file);
    fwrite("data", 1, 4, m_file);
    fwrite("\x00\x00\x00\x00", 1, 4, m_file);
    m_max_size = (SAMPLING_RATE) * 3 * sec;
    m_data_size = 0;
    m_closed = false;
  }

  INLINE void write(int32_t left24, int32_t right24) {
    if (m_data_size < m_max_size) {
      uint8_t l[3] = {static_cast<uint8_t>((left24  >>  0) & 0xFF),
                      static_cast<uint8_t>((left24  >>  8) & 0xFF),
                      static_cast<uint8_t>((left24  >> 16) & 0xFF)};
      uint8_t r[3] = {static_cast<uint8_t>((right24 >>  0) & 0xFF),
                      static_cast<uint8_t>((right24 >>  8) & 0xFF),
                      static_cast<uint8_t>((right24 >> 16) & 0xFF)};
      fwrite(l, 3, 1, m_file);
      fwrite(r, 3, 1, m_file);
      m_data_size += 3;
      m_data_size += 3;
    } else {
      close();
      m_closed = true;
    }
  }

  INLINE void close() {
    if (!m_closed) {
      fseek(m_file, 0, SEEK_END);
      long file_size = ftell(m_file);
      fseek(m_file, 4, SEEK_SET);
      uint32_t a[1] = {static_cast<uint32_t>(file_size) - 8};
      fwrite(a, 4, 1, m_file);
      fseek(m_file, 40, SEEK_SET);
      uint32_t a2[1] = {static_cast<uint32_t>(file_size) - 36};
      fwrite(a2, 4, 1, m_file);
      fclose(m_file);
      printf("End Of Recording\n");
    }
  }
};
