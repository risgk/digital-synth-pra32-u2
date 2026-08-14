// refs https://www.g200kg.com/archives/2012/10/adsr.html
// refs https://www.g200kg.com/archives/2012/10/adsr2.html
// refs https://www.g200kg.com/archives/2020/07/adsr-1.html

#pragma once

#include "pra32-u2-common.h"
#include "pra32-u2-eg-table.h"

class PRA32_U2_EG {
  static const uint8_t STATE_ATTACK  = 0;
  static const uint8_t STATE_SUSTAIN = 1;
  static const uint8_t STATE_IDLE    = 2;

  uint8_t  m_state;
  int32_t  m_level;
  int16_t  m_level_out;
  int32_t  m_attack;
  int32_t  m_decay;
  int32_t  m_attack_coef;
  int32_t  m_decay_coef;
  int32_t  m_sustain;
  int32_t  m_release;
  int32_t  m_release_coef;
  uint8_t  m_level_note_on_velocity_sensitivity;
  int32_t  m_attack_level;
  int32_t  m_sustain_level;
  int32_t  m_attack_decay_note_on_velocity_sensitivity;
  int32_t  m_release_note_off_velocity_sensitivity;
  uint8_t  m_note_on_velocity;
  uint8_t  m_note_off_velocity;
  uint16_t m_osc_pitch;
  int32_t  m_attack_decay_pitch_amt;
  boolean  m_release_eq_decay;

public:
  PRA32_U2_EG()
  : m_state()
  , m_level()
  , m_level_out()
  , m_attack()
  , m_decay()
  , m_attack_coef()
  , m_decay_coef()
  , m_sustain()
  , m_release()
  , m_release_coef()
  , m_level_note_on_velocity_sensitivity()
  , m_attack_level()
  , m_sustain_level()
  , m_attack_decay_note_on_velocity_sensitivity()
  , m_release_note_off_velocity_sensitivity()
  , m_note_on_velocity(64)
  , m_note_off_velocity(64)
  , m_osc_pitch(60 << 8)
  , m_attack_decay_pitch_amt()
  , m_release_eq_decay()
  {
    m_state = STATE_IDLE;
    set_attack(0);
    set_decay(0);
    set_sustain(127);
    set_release(0);
  }

  INLINE void set_attack(uint8_t controller_value) {
    m_attack = controller_value;
    update_attack_coef();
  }

  INLINE void set_decay(uint8_t controller_value) {
    m_decay = controller_value;
    update_decay_coef();
  }

  INLINE void set_sustain(uint8_t controller_value) {
    m_sustain = (controller_value == 127) ? 128 : controller_value;
    update_sustain_level();
  }

  INLINE void set_release(uint8_t controller_value) {
    m_release = controller_value;
    update_release_coef();
  }

  INLINE void set_release_eq_decay(uint8_t controller_value) {
    m_release_eq_decay = (controller_value >= 64);
    update_release_coef();
  }

  INLINE void set_level_note_on_velocity_sensitivity(uint8_t controller_value) {
    m_level_note_on_velocity_sensitivity = (controller_value + 1) >> 1;
  }

  INLINE void set_attack_decay_note_on_velocity_sensitivity(uint8_t controller_value) {
    m_attack_decay_note_on_velocity_sensitivity = ((controller_value - 63) >> 1) << 1;
  }

  INLINE void set_release_note_off_velocity_sensitivity(uint8_t controller_value) {
    m_release_note_off_velocity_sensitivity = ((controller_value - 63) >> 1) << 1;
  }

  INLINE void set_attack_decay_pitch_amt(uint8_t controller_value) {
    m_attack_decay_pitch_amt = ((controller_value - 63) >> 1) << 1;
  }

  INLINE void note_on(uint8_t velocity, uint16_t osc_pitch = (60 << 8)) {
    m_note_on_velocity = (velocity <= 127) ? velocity : m_note_on_velocity;
    m_osc_pitch = osc_pitch;

    update_attack_coef();
    update_decay_coef();

    m_attack_level = ((((m_note_on_velocity * m_level_note_on_velocity_sensitivity) +
                        (127 * (64 - m_level_note_on_velocity_sensitivity))) * 16384) / 127)
                     << (EG_LEVEL_MAX_BITS - 20);
    update_sustain_level();

    m_state = STATE_ATTACK;
  }

  INLINE void note_off(uint8_t velocity, boolean sound_off = false) {
    m_note_off_velocity = velocity + ((!velocity) * 64);

    update_release_coef();

    m_state = STATE_IDLE;

    const uint32_t keep_mask = -static_cast<int32_t>(sound_off ^ 1);
    m_level &= keep_mask;
    m_level_out = m_level >> 16;
  }

  INLINE int16_t get_output() {
    return m_level_out;
  }

  INLINE void process_at_low_rate() {
#if 1
    switch (m_state) {
    case STATE_ATTACK:
      m_level = ((m_attack_level - 1) << 1) - (multiply_shift_right((((m_attack_level - 1) << 1) - m_level), m_attack_coef, 32) << 2);
      m_level = minimum(m_level, EG_LEVEL_MAX);
      m_state = /* STATE_ATTACK * (m_level < m_attack_level) + */ STATE_SUSTAIN * (m_level >= m_attack_level);
      break;

    case STATE_SUSTAIN:
      {
        int32_t effective_sustain = minimum(m_sustain_level, m_level);
        m_level = effective_sustain + (multiply_shift_right((m_level - effective_sustain), m_decay_coef, 32) << 2);
      }
      break;

    case STATE_IDLE:
      m_level = multiply_shift_right(m_level, m_release_coef, 32) << 2;
      break;
    }

    m_level_out = m_level >> 16;
#endif
  }

private:
  INLINE void update_attack_coef() {
    int32_t attack = m_attack * (1 << EG_TABLE_EXT_BITS);
    attack += ((((m_note_on_velocity - 64) * m_attack_decay_note_on_velocity_sensitivity)) >> (6 - EG_TABLE_EXT_BITS));
    attack += ((m_osc_pitch - (60 << 8)) * m_attack_decay_pitch_amt) >> (14 - EG_TABLE_EXT_BITS);
    attack = clamp(attack, 0, 127 * (1 << EG_TABLE_EXT_BITS));
    m_attack_coef = g_eg_attack_decay_release_coef_table[attack + 16 * (1 << EG_TABLE_EXT_BITS)];
  }

  INLINE void update_decay_coef() {
    int32_t decay = m_decay * (1 << EG_TABLE_EXT_BITS);
    decay += ((((m_note_on_velocity - 64) * m_attack_decay_note_on_velocity_sensitivity)) >> (6 - EG_TABLE_EXT_BITS));
    decay += ((m_osc_pitch - (60 << 8)) * m_attack_decay_pitch_amt) >> (14 - EG_TABLE_EXT_BITS);
    decay = clamp(decay, 0, 126 * (1 << EG_TABLE_EXT_BITS));
    m_decay_coef = g_eg_attack_decay_release_coef_table[decay];

    m_decay_coef = (m_decay_coef & -(m_decay != 127)) |
                   (0x40000000   & -(m_decay == 127));
  }

  INLINE void update_sustain_level() {
    m_sustain_level = (m_attack_level >> 5) * m_sustain;
  }

  INLINE void update_release_coef() {
    int32_t release;

    if (m_release_eq_decay) {
      release = m_decay * (1 << EG_TABLE_EXT_BITS);
      release += ((((m_note_on_velocity - 64) * m_attack_decay_note_on_velocity_sensitivity)) >> (6 - EG_TABLE_EXT_BITS));
      release += ((m_osc_pitch - (60 << 8)) * m_attack_decay_pitch_amt) >> (14 - EG_TABLE_EXT_BITS);
    } else {
      release = m_release * (1 << EG_TABLE_EXT_BITS);
    }

    release += ((((m_note_off_velocity - 64) * m_release_note_off_velocity_sensitivity)) >> (6 - EG_TABLE_EXT_BITS));
    release  = clamp(release, 0, 127 * (1 << EG_TABLE_EXT_BITS));

    m_release_coef = g_eg_attack_decay_release_coef_table[release];
  }
};
