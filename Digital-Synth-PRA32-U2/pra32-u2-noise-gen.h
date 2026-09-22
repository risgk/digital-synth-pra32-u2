// refs https://en.wikipedia.org/wiki/Xorshift

#pragma once

#include "pra32-u2-common.h"

class PRA32_U2_NoiseGen {
  uint32_t m_state_a;
  uint32_t m_state_a_prev;

public:
  // Scatter the seed, so that the state does not start with a few bits set
  // (which would bias the first outputs) and so that the states of several
  // instances are linearly independent. The last bit is set to keep the state
  // away from zero, which xorshift cannot leave
  PRA32_U2_NoiseGen(uint32_t seed)
  : m_state_a((0x9E3779B9u * (seed + 1)) | 1u)
  , m_state_a_prev(0)
  {
  }

  INLINE int32_t process() {
    m_state_a_prev = m_state_a;

    uint32_t x = m_state_a;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    m_state_a = x;

    int32_t noise_int23 = (m_state_a >> 9) - (1 << 22);
    return noise_int23;
  }

  INLINE int32_t get() {
    int32_t noise_int23 = (m_state_a >> 9) - (1 << 22);
    return noise_int23;
  }

  INLINE void get_rand_uint8_array(uint8_t array[8]) {
    array[0] = (m_state_a_prev >>  0) & 0xFFu;
    array[1] = (m_state_a_prev >>  8) & 0xFFu;
    array[2] = (m_state_a_prev >> 16) & 0xFFu;
    array[3] = (m_state_a_prev >> 24) & 0xFFu;
    array[4] = (m_state_a      >>  0) & 0xFFu;
    array[5] = (m_state_a      >>  8) & 0xFFu;
    array[6] = (m_state_a      >> 16) & 0xFFu;
    array[7] = (m_state_a      >> 24) & 0xFFu;
  }
};
