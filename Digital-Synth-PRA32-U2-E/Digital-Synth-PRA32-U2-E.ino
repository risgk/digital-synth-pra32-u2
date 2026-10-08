/*
 * Digital Synth PRA32-U2/E (Experimental)
 *
 * PRA32-U2/M (PRA32-U2 Multi-Timbre Edition) for M5Stack AtomS3 Lite (ESP32-S3)
 * and M5Stack Atomic Audio-3.5 Base
 */

#define PRA32_U2_VERSION                       "v3.8.0    "

//#define PRA32_U2_USE_DEBUG_PRINT

#define PRA32_U2_USE_USB_MIDI                  // Select USB Mode: "USB-OTG (TinyUSB)" in the Arduino IDE "Tools" menu

#define PRA32_U2_USE_UART_MIDI

#define PRA32_U2_DEBUG_PRINT_SERIAL            Serial  // USB CDC, next to USB MIDI

#define PRA32_U2_UART_MIDI_SPEED               (31250)
//#define PRA32_U2_UART_MIDI_SPEED               (38400)

#define PRA32_U2_UART_MIDI_SERIAL              Serial2
#define PRA32_U2_UART_MIDI_TX_PIN              (2)   // Grove
#define PRA32_U2_UART_MIDI_RX_PIN              (1)   // Grove

#define PRA32_U2_MIDI_CH                       (0)  // 0-based

// for M5Stack Atomic Audio-3.5 Base
#define PRA32_U2_I2S_DATA_PIN                  (5)
#define PRA32_U2_I2S_BCLK_PIN                  (8)
#define PRA32_U2_I2S_LRCLK_PIN                 (6)
#define PRA32_U2_I2S_SWAP_LEFT_AND_RIGHT       (false)
#define PRA32_U2_I2C_SDA_PIN                   (38)
#define PRA32_U2_I2C_SCL_PIN                   (39)
#define PRA32_U2_ES8311_DAC_VOLUME             (0xBF)  // 0.5 dB steps, 0xBF = 0 dB

#define PRA32_U2_I2S_BUFFERS                   (2)   // Output buffer (maximum latency): 2 * 64 = 128 frames (2.7 ms)
#define PRA32_U2_I2S_BUFFER_WORDS              (64)  // Frames per buffer (= frames processed in each loop)

// for M5Stack AtomS3 Lite
// The board "M5AtomS3" covers the AtomS3 too, which has no RGB LED, so the LED is switched here.
// It is not lit with PSRAM (BOARD_HAS_PSRAM), whose OPI PSRAM uses GPIO35, the RGB LED pin
#define PRA32_U2_M5STACK_ATOMS3_LITE
#define PRA32_U2_LED_LEVEL_R                   (0)
#define PRA32_U2_LED_LEVEL_G                   (32)
#define PRA32_U2_LED_LEVEL_B                   (0)

#define PRA32_U2_USE_2_CORES_FOR_SIGNAL_PROCESSING

#define PRA32_U2_SYNTH_TASK_STACK_SIZE         (8192)
#define PRA32_U2_SECONDARY_TASK_STACK_SIZE     (8192)

//#define PRA32_U2_LIMIT_DELAY_TIME_TO_SAVE_MEM

#define PRA32_U2_ENABLE_LAYERING

#if defined(PRA32_U2_ENABLE_LAYERING)
#define PRA32_U2_NUMBER_OF_SYNTHS              (4 + 3)
#else
#define PRA32_U2_NUMBER_OF_SYNTHS              (4)
#endif

////////////////////////////////////////////////////////////////

#if !defined(ARDUINO_ARCH_ESP32)
#error This sketch is for M5Stack AtomS3 Lite (ESP32-S3); use "Digital-Synth-PRA32-U2-M.ino" for Raspberry Pi Pico 2
#endif  // !defined(ARDUINO_ARCH_ESP32)

// The synth core runs from the flash (through the cache). IRAM_ATTR cannot stand in for the
// RP2350's .time_critical: on the template member functions, which are COMDAT, the literals
// are placed after their use, and the linker rejects them
#define __not_in_flash_func(func)              func

// The wave tables (about 130 KB) do not fit in the RAM together with the synths, so they are
// stored in the flash, and copied to the PSRAM at startup if there is one (see
// move_wave_tables_to_psram()). They are never written. ".irom1.text" is where the
// linker script collects the read-only data placed in the flash by a section attribute; a
// ".rodata.*" name would work too, but the assembler warns about it for the writable tables
#define PRA32_U2_OSC_WAVE_TABLE_ATTR           __attribute__((section(".irom1.text")))

uint8_t g_midi_ch = PRA32_U2_MIDI_CH;

#include "pra32-u2-common.h"
#include "pra32-u2-synth.h"

#include <atomic>

boolean g_synth_is_in_polyphonic_mode = true;
PRA32_U2_Synth<false, false, true, 0, false, false> g_synth;
PRA32_U2_Synth<true,  false, true, 1, false, true>  g_sub_1_synth;
PRA32_U2_Synth<true,  false, true, 2, false, true>  g_sub_2_synth;
PRA32_U2_Synth<true,  false, true, 3, false, true>  g_sub_3_synth;

#include <MIDI.h>
struct MySettings : public midi::DefaultSettings {
  static const long BaudRate = PRA32_U2_UART_MIDI_SPEED;
  static const bool HandleNullVelocityNoteOnAsNoteOff = false;
};

#if defined(PRA32_U2_USE_USB_MIDI)
#if ARDUINO_USB_MODE
#error Select USB Mode: "USB-OTG (TinyUSB)" in the Arduino IDE "Tools" menu
#endif  // ARDUINO_USB_MODE
#include <USB.h>
#include <USBMIDI.h>
USBMIDI g_usb_midi("PRA32-U2/E");
#endif  // defined(PRA32_U2_USE_USB_MIDI)

#if defined(PRA32_U2_USE_UART_MIDI)
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, PRA32_U2_UART_MIDI_SERIAL, UART_MIDI, MySettings);
#endif

#include <Wire.h>
#include <driver/i2s_std.h>

#if defined(BOARD_HAS_PSRAM)
#include <esp_heap_caps.h>
#include <cstring>
#include <utility>
#include <vector>
#endif  // defined(BOARD_HAS_PSRAM)

static i2s_chan_handle_t s_i2s_output = NULL;
static int32_t           s_i2s_frames[PRA32_U2_I2S_BUFFER_WORDS * 2];

static TaskHandle_t      s_synth_task     = NULL;
static TaskHandle_t      s_secondary_task = NULL;

static boolean           s_wave_tables_in_psram = false;

static volatile uint32_t s_debug_measurement_min_us      = UINT32_MAX;
static volatile uint32_t s_debug_measurement_max_us      = 0;
static volatile uint32_t s_debug_measurement_counted     = 0;

static volatile uint32_t s_secondary_core_processing_request  = 0;
// The FX inputs, which each synth adds its output to by its FX Routing (on both cores)
static PRA32_U2_FxBusSample s_fx_bus = {};

// The two cores share the internal SRAM without a data cache, so this only has to keep
// the compiler (and the write buffer) from reordering the accesses around the flags
static INLINE void memory_barrier() {
  std::atomic_thread_fence(std::memory_order_seq_cst);
}

void handleNoteOn(byte channel, byte pitch, byte velocity);
void handleNoteOff(byte channel, byte pitch, byte velocity);
void handleControlChange(byte channel, byte number, byte value);
void handleProgramChange(byte channel, byte number);
void handlePitchBend(byte channel, int bend);
void handleAfterTouchPoly(byte channel, byte note, byte pressure);
void handleAfterTouchChannel(byte channel, byte pressure);

static void write_i2c_register(uint8_t address, uint8_t reg, uint8_t value) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

static uint8_t read_i2c_register(uint8_t address, uint8_t reg) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(address, static_cast<uint8_t>(1));
  return Wire.read();
}

// Follows es8311_init() in M5Atomic-EchoBase, minus the microphone. The ES8311 takes its master
// clock from SCLK, so the dividers are for SCLK = 48 kHz x 64 = 3.072 MHz: 48 kHz and 32-bit
// slots only
static void start_es8311() {
  const uint8_t ES8311 = 0x18;
  write_i2c_register(ES8311, 0x00, 0x1F);  // Reset
  delay(20);
  write_i2c_register(ES8311, 0x00, 0x00);
  write_i2c_register(ES8311, 0x00, 0x80);  // Power on, slave
  write_i2c_register(ES8311, 0x01, 0xBF);  // All clocks on, MCLK from SCLK
  write_i2c_register(ES8311, 0x02, (read_i2c_register(ES8311, 0x02) & 0x07) | (2 << 3));  // Pre-multiply x4
  write_i2c_register(ES8311, 0x03, 0x10);  // ADC OSR
  write_i2c_register(ES8311, 0x04, 0x10);  // DAC OSR
  write_i2c_register(ES8311, 0x05, 0x00);  // ADC and DAC dividers 1
  write_i2c_register(ES8311, 0x06, (read_i2c_register(ES8311, 0x06) & 0xC0) | 0x03);  // SCLK not inverted, divider 4
  write_i2c_register(ES8311, 0x07, read_i2c_register(ES8311, 0x07) & 0xC0);  // LRCK divider 0x00FF
  write_i2c_register(ES8311, 0x08, 0xFF);
  write_i2c_register(ES8311, 0x09, 0x10);  // SDP in, I2S, 32-bit
  write_i2c_register(ES8311, 0x0A, 0x10);  // SDP out, I2S, 32-bit
  write_i2c_register(ES8311, 0x0D, 0x01);  // Analog power up
  write_i2c_register(ES8311, 0x0E, 0x02);
  write_i2c_register(ES8311, 0x12, 0x00);  // DAC power up
  write_i2c_register(ES8311, 0x13, 0x10);  // Output to HP drive
  write_i2c_register(ES8311, 0x1C, 0x6A);
  write_i2c_register(ES8311, 0x37, 0x08);  // DAC equalizer bypassed
  write_i2c_register(ES8311, 0x32, PRA32_U2_ES8311_DAC_VOLUME);
}

// The PI4IOE5V6408 drives the power amplifier's enable from P0
static void start_pi4ioe() {
  const uint8_t PI4IOE = 0x43;
  read_i2c_register(PI4IOE, 0x00);
  write_i2c_register(PI4IOE, 0x07, 0x00);  // Outputs high-impedance
  write_i2c_register(PI4IOE, 0x0D, 0xFF);  // Pull-ups
  write_i2c_register(PI4IOE, 0x03, 0x6F);  // Directions
  write_i2c_register(PI4IOE, 0x05, 0xFF);  // Outputs high
}

// The 24-bit samples are sent left-justified in 32-bit slots (BCLK = 64 fs), as on the RP2350
static void start_audio() {
  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  chan_cfg.dma_desc_num  = PRA32_U2_I2S_BUFFERS;
  chan_cfg.dma_frame_num = PRA32_U2_I2S_BUFFER_WORDS;
  chan_cfg.auto_clear    = true;
  i2s_new_channel(&chan_cfg, &s_i2s_output, NULL);

  i2s_std_config_t std_cfg = {
    .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLING_RATE),
    .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
    .gpio_cfg = {
      .mclk = I2S_GPIO_UNUSED,
      .bclk = static_cast<gpio_num_t>(PRA32_U2_I2S_BCLK_PIN),
      .ws   = static_cast<gpio_num_t>(PRA32_U2_I2S_LRCLK_PIN),
      .dout = static_cast<gpio_num_t>(PRA32_U2_I2S_DATA_PIN),
      .din  = I2S_GPIO_UNUSED,
      .invert_flags = {
        .mclk_inv = false,
        .bclk_inv = false,
        .ws_inv   = false,
      },
    },
  };
  i2s_channel_init_std_mode(s_i2s_output, &std_cfg);
  i2s_channel_enable(s_i2s_output);

  Wire.begin(PRA32_U2_I2C_SDA_PIN, PRA32_U2_I2C_SCL_PIN, 100000U);
  start_es8311();
  start_pi4ioe();
}

#if defined(BOARD_HAS_PSRAM)
// Copies the wave tables in the flash to the PSRAM, and points the table arrays at the copies.
// Run before the synths are initialized. The Osc reads the table arrays again at the control
// rate, and a pointer it took before (and the LFO, which reads its sine table directly) still
// reads the same table in the flash. If the PSRAM runs out, the rest stay in the flash
static boolean move_wave_tables_to_psram() {
  if (!psramFound()) {
    return false;
  }

  struct TableArray {
    int16_t** tables;
    uint32_t  length;
  };
  const TableArray table_arrays[] = {
    { g_osc_saw_wave_tables,      sizeof(g_osc_saw_wave_tables)      / sizeof(g_osc_saw_wave_tables[0])      },
    { g_osc_saw2_wave_tables,     sizeof(g_osc_saw2_wave_tables)     / sizeof(g_osc_saw2_wave_tables[0])     },
    { g_osc_triangle_wave_tables, sizeof(g_osc_triangle_wave_tables) / sizeof(g_osc_triangle_wave_tables[0]) },
    { g_osc_square_wave_tables,   sizeof(g_osc_square_wave_tables)   / sizeof(g_osc_square_wave_tables[0])   },
    { g_osc_sine_wave_tables,     sizeof(g_osc_sine_wave_tables)     / sizeof(g_osc_sine_wave_tables[0])     },
  };

  // A table is shared by several notes (and waveforms), so it is copied once
  std::vector<std::pair<int16_t*, int16_t*>> copied;  // (in the flash, in the PSRAM)

  for (const TableArray& table_array : table_arrays) {
    for (uint32_t i = 0; i < table_array.length; i++) {
      int16_t* table = table_array.tables[i];
      int16_t* copy  = NULL;
      for (const auto& pair : copied) {
        if (pair.first == table) {
          copy = pair.second;
          break;
        }
      }

      if (copy == NULL) {
        // The arrays point past the first entry, the number of index bits, which is followed
        // by (1 << bits) + 1 samples
        uint32_t entries = (1 << table[-1]) + 2;
        int16_t* buffer = static_cast<int16_t*>(heap_caps_malloc(entries * sizeof(int16_t), MALLOC_CAP_SPIRAM));
        if (buffer == NULL) {
          return false;
        }
        std::memcpy(buffer, table - 1, entries * sizeof(int16_t));
        copy = buffer + 1;
        copied.push_back(std::make_pair(table, copy));
      }

      table_array.tables[i] = copy;
    }
  }

  return true;
}
#endif  // defined(BOARD_HAS_PSRAM)

#if defined(PRA32_U2_USE_USB_MIDI)
// Stands in for the MIDI library, which has no transport for the core's USBMIDI. Dispatches the
// same messages, with pitch bend signed as the library would hand it over
static void read_usb_midi() {
  midiEventPacket_t packet;
  while (g_usb_midi.readPacket(&packet)) {
    byte channel = (packet.byte1 & 0x0F) + 1;
    switch (packet.header & 0x0F) {
    case 0x8:
      handleNoteOff(channel, packet.byte2, packet.byte3);
      break;
    case 0x9:
      handleNoteOn(channel, packet.byte2, packet.byte3);
      break;
    case 0xA:
      handleAfterTouchPoly(channel, packet.byte2, packet.byte3);
      break;
    case 0xB:
      handleControlChange(channel, packet.byte2, packet.byte3);
      break;
    case 0xC:
      handleProgramChange(channel, packet.byte2);
      break;
    case 0xD:
      handleAfterTouchChannel(channel, packet.byte2);
      break;
    case 0xE:
      handlePitchBend(channel, ((packet.byte3 << 7) | packet.byte2) - 8192);
      break;
    }
  }
}
#endif  // defined(PRA32_U2_USE_USB_MIDI)

// Core 0: the same as loop1() of PRA32-U2/M (the Main Synth's Voices 3 and 4, and the Sub
// Synths 1 and 3). Woken once per buffer, so that core 0 is free (for USB and the idle task)
// while the synth task waits for the I2S output
void __not_in_flash_func(secondary_task)(void* parameter) {
  g_synth.initialize_secondary_core();
  g_sub_1_synth.initialize_secondary_core();
  g_sub_2_synth.initialize_secondary_core();
  g_sub_3_synth.initialize_secondary_core();

  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    for (uint32_t i = 0; i < PRA32_U2_I2S_BUFFER_WORDS; i++) {
      while (s_secondary_core_processing_request == 0) {
        ;
      }
      memory_barrier();

      PRA32_U2_StereoSample sub_3_synth_output = { 0, 0 };
      PRA32_U2_StereoSample sub_1_synth_output = { 0, 0 };
      if (g_synth_is_in_polyphonic_mode == false) {
        sub_3_synth_output = g_sub_3_synth.process(0, 0);
        sub_1_synth_output = g_sub_1_synth.process(0, 0);
      }

      boolean processed = false;
      while (processed == false) {
        processed = g_synth.secondary_core_process();
      }

      // Added after the Main Synth's Voices 3 and 4, which the synth task waits for in
      // the middle of g_synth.process(), so that it is done while that goes on
      g_sub_3_synth.add_to_fx_bus(sub_3_synth_output, s_fx_bus);
      g_sub_1_synth.add_to_fx_bus(sub_1_synth_output, s_fx_bus);

      memory_barrier();
      s_secondary_core_processing_request = 0;
    }
  }
}

// Core 1: the same as loop() of PRA32-U2/M. MIDI is read here, between the buffers, so that
// the synths are never changed while they are processed
void __not_in_flash_func(synth_task)(void* parameter) {
  for (;;) {
    for (uint32_t i = 0; i < ((PRA32_U2_I2S_BUFFER_WORDS + 31) / 32) + 1; i++) {
#if defined(PRA32_U2_USE_USB_MIDI)
      read_usb_midi();
#endif  // defined(PRA32_U2_USE_USB_MIDI)

#if defined(PRA32_U2_USE_UART_MIDI)
      UART_MIDI.read();
#endif
    }

    boolean mode = g_synth.is_in_polyphonic_mode();
    if (g_synth_is_in_polyphonic_mode != mode) {
      g_synth_is_in_polyphonic_mode = mode;
      g_sub_1_synth.all_notes_off(true);
      g_sub_2_synth.all_notes_off(true);
      g_sub_3_synth.all_notes_off(true);
    }

#if defined(PRA32_U2_USE_DEBUG_PRINT)
    uint32_t debug_measurement_start_us = micros();
#endif  // defined(PRA32_U2_USE_DEBUG_PRINT)

    memory_barrier();
    xTaskNotifyGive(s_secondary_task);

    for (uint32_t i = 0; i < PRA32_U2_I2S_BUFFER_WORDS; i++) {
      s_secondary_core_processing_request = 1;

      // Each synth adds its output to the FX input selected by its FX Routing.
      // The secondary core adds its synths only after g_synth.process() requests
      // the Main Synth's Voices 3 and 4, so s_fx_bus can be written before that
      PRA32_U2_StereoSample sub_2_synth_output = { 0, 0 };
      if (g_synth_is_in_polyphonic_mode == false) {
        sub_2_synth_output = g_sub_2_synth.process(0, 0);
      }

      clear_fx_bus(s_fx_bus);
      g_sub_2_synth.add_to_fx_bus(sub_2_synth_output, s_fx_bus);
      memory_barrier();

      PRA32_U2_StereoSample synth_output = g_synth.process<false, true>(0, 0);

      while (s_secondary_core_processing_request) {
        ;
      }
      memory_barrier();

      g_synth.add_to_fx_bus(synth_output, s_fx_bus);

      PRA32_U2_StereoSample synth_fx_output = g_synth.process_fx(s_fx_bus);
      if (PRA32_U2_I2S_SWAP_LEFT_AND_RIGHT) {
        s_i2s_frames[i * 2]     = synth_fx_output.right * 256;
        s_i2s_frames[i * 2 + 1] = synth_fx_output.left  * 256;
      } else {
        s_i2s_frames[i * 2]     = synth_fx_output.left  * 256;
        s_i2s_frames[i * 2 + 1] = synth_fx_output.right * 256;
      }
    }

#if defined(PRA32_U2_USE_DEBUG_PRINT)
    uint32_t debug_measurement_elapsed_us = micros() - debug_measurement_start_us;
    s_debug_measurement_min_us -= s_debug_measurement_counted *
                                  (debug_measurement_elapsed_us < s_debug_measurement_min_us) *
                                  (s_debug_measurement_min_us - debug_measurement_elapsed_us);
    s_debug_measurement_max_us += s_debug_measurement_counted *
                                  (debug_measurement_elapsed_us > s_debug_measurement_max_us) *
                                  (debug_measurement_elapsed_us - s_debug_measurement_max_us);
    s_debug_measurement_counted = 1;
#endif  // defined(PRA32_U2_USE_DEBUG_PRINT)

    // One i2s_channel_write per buffer, not per frame: each call takes the channel's lock
    size_t bytes_written;
    i2s_channel_write(s_i2s_output, s_i2s_frames, sizeof(s_i2s_frames), &bytes_written, portMAX_DELAY);
  }
}

void setup() {
#if defined(PRA32_U2_USE_DEBUG_PRINT)
  PRA32_U2_DEBUG_PRINT_SERIAL.begin(115200);
#endif  // defined(PRA32_U2_USE_DEBUG_PRINT)

#if defined(PRA32_U2_USE_USB_MIDI)
  // Both names are ignored with USB CDC On Boot enabled, which starts USB before setup(). The
  // MIDI interface keeps the name given to g_usb_midi either way
  USB.manufacturerName("ISGK Instruments");
  USB.productName("PRA32-U2/E");
  g_usb_midi.begin();
  USB.begin();
#endif  // defined(PRA32_U2_USE_USB_MIDI)

#if defined(PRA32_U2_USE_UART_MIDI)
  // Started on its pins before UART_MIDI.begin(), whose pinless begin() keeps the pins already
  // set. Left to itself, that begin() would put UART2 on its default pins, GPIO19 and GPIO20,
  // which are the ESP32-S3's USB D- and D+
  PRA32_U2_UART_MIDI_SERIAL.begin(PRA32_U2_UART_MIDI_SPEED, SERIAL_8N1, PRA32_U2_UART_MIDI_RX_PIN, PRA32_U2_UART_MIDI_TX_PIN);
  UART_MIDI.setHandleNoteOn(handleNoteOn);
  UART_MIDI.setHandleNoteOff(handleNoteOff);
  UART_MIDI.setHandleControlChange(handleControlChange);
  UART_MIDI.setHandleProgramChange(handleProgramChange);
  UART_MIDI.setHandlePitchBend(handlePitchBend);
  UART_MIDI.setHandleAfterTouchPoly(handleAfterTouchPoly);
  UART_MIDI.setHandleAfterTouchChannel(handleAfterTouchChannel);
  UART_MIDI.begin(MIDI_CHANNEL_OMNI);
  UART_MIDI.turnThruOff();
#endif  // defined(PRA32_U2_USE_UART_MIDI)

#if defined(BOARD_HAS_PSRAM)
  s_wave_tables_in_psram = move_wave_tables_to_psram();
#endif  // defined(BOARD_HAS_PSRAM)

  g_synth.initialize();
  g_sub_1_synth.initialize();
  g_sub_2_synth.initialize();
  g_sub_3_synth.initialize();

  start_audio();

#if defined(ARDUINO_M5STACK_ATOMS3) && defined(PRA32_U2_M5STACK_ATOMS3_LITE) && !defined(BOARD_HAS_PSRAM)
  rgbLedWrite(RGB_BUILTIN, PRA32_U2_LED_LEVEL_R, PRA32_U2_LED_LEVEL_G, PRA32_U2_LED_LEVEL_B);
#endif  // defined(ARDUINO_M5STACK_ATOMS3) && defined(PRA32_U2_M5STACK_ATOMS3_LITE) && !defined(BOARD_HAS_PSRAM)

  // The synth task gets core 1 to itself (except loopTask, which only prints), as the primary
  // core on the RP2350, and blocks in i2s_channel_write for the rest of each buffer. The
  // secondary task goes to core 0, whose idle task the task watchdog watches, so it waits for
  // a notification between the buffers instead of polling
  xTaskCreatePinnedToCore(secondary_task, "pra32_u2_sub", PRA32_U2_SECONDARY_TASK_STACK_SIZE, NULL, configMAX_PRIORITIES - 2, &s_secondary_task, 0);
  xTaskCreatePinnedToCore(synth_task,     "pra32_u2",     PRA32_U2_SYNTH_TASK_STACK_SIZE,     NULL, configMAX_PRIORITIES - 2, &s_synth_task,     1);
}

void loop() {
#if defined(PRA32_U2_USE_DEBUG_PRINT)
  PRA32_U2_DEBUG_PRINT_SERIAL.print("\e[1;1H\e[K");
  PRA32_U2_DEBUG_PRINT_SERIAL.print("min(audio) ");
  PRA32_U2_DEBUG_PRINT_SERIAL.print(s_debug_measurement_min_us);
  PRA32_U2_DEBUG_PRINT_SERIAL.print("\e[2;1H\e[K");
  PRA32_U2_DEBUG_PRINT_SERIAL.print("max(audio) ");
  PRA32_U2_DEBUG_PRINT_SERIAL.print(s_debug_measurement_max_us);
  PRA32_U2_DEBUG_PRINT_SERIAL.print("\e[4;1H\e[K");
  PRA32_U2_DEBUG_PRINT_SERIAL.print("stack free ");
  PRA32_U2_DEBUG_PRINT_SERIAL.print(uxTaskGetStackHighWaterMark(s_synth_task));
  PRA32_U2_DEBUG_PRINT_SERIAL.print(" ");
  PRA32_U2_DEBUG_PRINT_SERIAL.print(uxTaskGetStackHighWaterMark(s_secondary_task));
  PRA32_U2_DEBUG_PRINT_SERIAL.print("\e[5;1H\e[K");
  PRA32_U2_DEBUG_PRINT_SERIAL.print("wave tables ");
  PRA32_U2_DEBUG_PRINT_SERIAL.print(s_wave_tables_in_psram ? "PSRAM" : "flash");
  PRA32_U2_DEBUG_PRINT_SERIAL.println();
  s_debug_measurement_min_us = UINT32_MAX;
  s_debug_measurement_max_us = 0;
  delay(1000);
#else  // defined(PRA32_U2_USE_DEBUG_PRINT)
  vTaskDelete(NULL);
#endif  // defined(PRA32_U2_USE_DEBUG_PRINT)
}

void __not_in_flash_func(handleNoteOn)(byte channel, byte pitch, byte velocity)
{
  if ((channel - 1) == g_midi_ch) {
    g_synth.note_on(pitch, velocity);
  } else if ((channel - 1) == ((g_midi_ch + 1) & 0x0F)) {
    g_sub_1_synth.note_on(pitch, velocity);
  } else if ((channel - 1) == ((g_midi_ch + 2) & 0x0F)) {
    g_sub_2_synth.note_on(pitch, velocity);
  } else if ((channel - 1) == ((g_midi_ch + 3) & 0x0F)) {
    g_sub_3_synth.note_on(pitch, velocity);
#if defined(PRA32_U2_ENABLE_LAYERING)
  } else if ((channel - 1) == ((g_midi_ch - 3) & 0x0F)) {
    g_synth.note_on(pitch, velocity);
    g_sub_1_synth.note_on(pitch, velocity);
  } else if ((channel - 1) == ((g_midi_ch - 2) & 0x0F)) {
    g_sub_2_synth.note_on(pitch, velocity);
    g_sub_3_synth.note_on(pitch, velocity);
  } else if ((channel - 1) == ((g_midi_ch - 1) & 0x0F)) {
    g_synth.note_on(pitch, velocity);
    g_sub_1_synth.note_on(pitch, velocity);
    g_sub_2_synth.note_on(pitch, velocity);
    g_sub_3_synth.note_on(pitch, velocity);
#endif  // defined(PRA32_U2_ENABLE_LAYERING)
  }
}

void __not_in_flash_func(handleNoteOff)(byte channel, byte pitch, byte velocity)
{
  if ((channel - 1) == g_midi_ch) {
    g_synth.note_off(pitch, velocity);
  } else if ((channel - 1) == ((g_midi_ch + 1) & 0x0F)) {
    g_sub_1_synth.note_off(pitch, velocity);
  } else if ((channel - 1) == ((g_midi_ch + 2) & 0x0F)) {
    g_sub_2_synth.note_off(pitch, velocity);
  } else if ((channel - 1) == ((g_midi_ch + 3) & 0x0F)) {
    g_sub_3_synth.note_off(pitch, velocity);
#if defined(PRA32_U2_ENABLE_LAYERING)
  } else if ((channel - 1) == ((g_midi_ch - 3) & 0x0F)) {
    g_synth.note_off(pitch, velocity);
    g_sub_1_synth.note_off(pitch, velocity);
  } else if ((channel - 1) == ((g_midi_ch - 2) & 0x0F)) {
    g_sub_2_synth.note_off(pitch, velocity);
    g_sub_3_synth.note_off(pitch, velocity);
  } else if ((channel - 1) == ((g_midi_ch - 1) & 0x0F)) {
    g_synth.note_off(pitch, velocity);
    g_sub_1_synth.note_off(pitch, velocity);
    g_sub_2_synth.note_off(pitch, velocity);
    g_sub_3_synth.note_off(pitch, velocity);
#endif  // defined(PRA32_U2_ENABLE_LAYERING)
  }
}

void __not_in_flash_func(handleControlChange)(byte channel, byte number, byte value)
{
  if ((channel - 1) == g_midi_ch) {
    g_synth.control_change(number, value);
  } else if ((channel - 1) == ((g_midi_ch + 1) & 0x0F)) {
    g_sub_1_synth.control_change(number, value);
  } else if ((channel - 1) == ((g_midi_ch + 2) & 0x0F)) {
    g_sub_2_synth.control_change(number, value);
  } else if ((channel - 1) == ((g_midi_ch + 3) & 0x0F)) {
    g_sub_3_synth.control_change(number, value);
#if defined(PRA32_U2_ENABLE_LAYERING)
  } else if ((channel - 1) == ((g_midi_ch - 3) & 0x0F)) {
    g_synth.control_change(number, value);
    g_sub_1_synth.control_change(number, value);
  } else if ((channel - 1) == ((g_midi_ch - 2) & 0x0F)) {
    g_sub_2_synth.control_change(number, value);
    g_sub_3_synth.control_change(number, value);
  } else if ((channel - 1) == ((g_midi_ch - 1) & 0x0F)) {
    g_synth.control_change(number, value);
    g_sub_1_synth.control_change(number, value);
    g_sub_2_synth.control_change(number, value);
    g_sub_3_synth.control_change(number, value);
#endif  // defined(PRA32_U2_ENABLE_LAYERING)
  }
}

void __not_in_flash_func(handleProgramChange)(byte channel, byte number)
{
  if ((channel - 1) == g_midi_ch) {
    g_synth.program_change(number);
  } else if ((channel - 1) == ((g_midi_ch + 1) & 0x0F)) {
    g_sub_1_synth.program_change(number);
  } else if ((channel - 1) == ((g_midi_ch + 2) & 0x0F)) {
    g_sub_2_synth.program_change(number);
  } else if ((channel - 1) == ((g_midi_ch + 3) & 0x0F)) {
    g_sub_3_synth.program_change(number);
#if defined(PRA32_U2_ENABLE_LAYERING)
  } else if ((channel - 1) == ((g_midi_ch - 3) & 0x0F)) {
    g_synth.program_change(number);
    g_sub_1_synth.program_change((number + 1) & 0x3F);
  } else if ((channel - 1) == ((g_midi_ch - 2) & 0x0F)) {
    g_sub_2_synth.program_change(number);
    g_sub_3_synth.program_change((number + 1) & 0x3F);
  } else if ((channel - 1) == ((g_midi_ch - 1) & 0x0F)) {
    g_synth.program_change(number);
    g_sub_1_synth.program_change((number + 1) & 0x3F);
    g_sub_2_synth.program_change((number + 2) & 0x3F);
    g_sub_3_synth.program_change((number + 3) & 0x3F);
#endif  // defined(PRA32_U2_ENABLE_LAYERING)
  }
}

void __not_in_flash_func(handlePitchBend)(byte channel, int bend)
{
  if ((channel - 1) == g_midi_ch) {
    g_synth.pitch_bend((bend + 8192) & 0x7F, (bend + 8192) >> 7);
  } else if ((channel - 1) == ((g_midi_ch + 1) & 0x0F)) {
    g_sub_1_synth.pitch_bend((bend + 8192) & 0x7F, (bend + 8192) >> 7);
  } else if ((channel - 1) == ((g_midi_ch + 2) & 0x0F)) {
    g_sub_2_synth.pitch_bend((bend + 8192) & 0x7F, (bend + 8192) >> 7);
  } else if ((channel - 1) == ((g_midi_ch + 3) & 0x0F)) {
    g_sub_3_synth.pitch_bend((bend + 8192) & 0x7F, (bend + 8192) >> 7);
#if defined(PRA32_U2_ENABLE_LAYERING)
  } else if ((channel - 1) == ((g_midi_ch - 3) & 0x0F)) {
    g_synth.pitch_bend((bend + 8192) & 0x7F, (bend + 8192) >> 7);
    g_sub_1_synth.pitch_bend((bend + 8192) & 0x7F, (bend + 8192) >> 7);
  } else if ((channel - 1) == ((g_midi_ch - 2) & 0x0F)) {
    g_sub_2_synth.pitch_bend((bend + 8192) & 0x7F, (bend + 8192) >> 7);
    g_sub_3_synth.pitch_bend((bend + 8192) & 0x7F, (bend + 8192) >> 7);
  } else if ((channel - 1) == ((g_midi_ch - 1) & 0x0F)) {
    g_synth.pitch_bend((bend + 8192) & 0x7F, (bend + 8192) >> 7);
    g_sub_1_synth.pitch_bend((bend + 8192) & 0x7F, (bend + 8192) >> 7);
    g_sub_2_synth.pitch_bend((bend + 8192) & 0x7F, (bend + 8192) >> 7);
    g_sub_3_synth.pitch_bend((bend + 8192) & 0x7F, (bend + 8192) >> 7);
#endif  // defined(PRA32_U2_ENABLE_LAYERING)
  }
}

void __not_in_flash_func(handleAfterTouchPoly)(byte channel, byte note, byte pressure)
{
  if ((channel - 1) == g_midi_ch) {
    g_synth.after_touch_poly(note, pressure);
  } else if ((channel - 1) == ((g_midi_ch + 1) & 0x0F)) {
    g_sub_1_synth.after_touch_poly(note, pressure);
  } else if ((channel - 1) == ((g_midi_ch + 2) & 0x0F)) {
    g_sub_2_synth.after_touch_poly(note, pressure);
  } else if ((channel - 1) == ((g_midi_ch + 3) & 0x0F)) {
    g_sub_3_synth.after_touch_poly(note, pressure);
#if defined(PRA32_U2_ENABLE_LAYERING)
  } else if ((channel - 1) == ((g_midi_ch - 3) & 0x0F)) {
    g_synth.after_touch_poly(note, pressure);
    g_sub_1_synth.after_touch_poly(note, pressure);
  } else if ((channel - 1) == ((g_midi_ch - 2) & 0x0F)) {
    g_sub_2_synth.after_touch_poly(note, pressure);
    g_sub_3_synth.after_touch_poly(note, pressure);
  } else if ((channel - 1) == ((g_midi_ch - 1) & 0x0F)) {
    g_synth.after_touch_poly(note, pressure);
    g_sub_1_synth.after_touch_poly(note, pressure);
    g_sub_2_synth.after_touch_poly(note, pressure);
    g_sub_3_synth.after_touch_poly(note, pressure);
#endif  // defined(PRA32_U2_ENABLE_LAYERING)
  }
}

void __not_in_flash_func(handleAfterTouchChannel)(byte channel, byte pressure)
{
  if ((channel - 1) == g_midi_ch) {
    g_synth.after_touch_channel(pressure);
  } else if ((channel - 1) == ((g_midi_ch + 1) & 0x0F)) {
    g_sub_1_synth.after_touch_channel(pressure);
  } else if ((channel - 1) == ((g_midi_ch + 2) & 0x0F)) {
    g_sub_2_synth.after_touch_channel(pressure);
  } else if ((channel - 1) == ((g_midi_ch + 3) & 0x0F)) {
    g_sub_3_synth.after_touch_channel(pressure);
#if defined(PRA32_U2_ENABLE_LAYERING)
  } else if ((channel - 1) == ((g_midi_ch - 3) & 0x0F)) {
    g_synth.after_touch_channel(pressure);
    g_sub_1_synth.after_touch_channel(pressure);
  } else if ((channel - 1) == ((g_midi_ch - 2) & 0x0F)) {
    g_sub_2_synth.after_touch_channel(pressure);
    g_sub_3_synth.after_touch_channel(pressure);
  } else if ((channel - 1) == ((g_midi_ch - 1) & 0x0F)) {
    g_synth.after_touch_channel(pressure);
    g_sub_1_synth.after_touch_channel(pressure);
    g_sub_2_synth.after_touch_channel(pressure);
    g_sub_3_synth.after_touch_channel(pressure);
#endif  // defined(PRA32_U2_ENABLE_LAYERING)
  }
}
