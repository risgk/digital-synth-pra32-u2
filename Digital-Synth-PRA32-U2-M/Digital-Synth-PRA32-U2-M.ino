/*
 * Digital Synth PRA32-U2/M
 */

#define PRA32_U2_VERSION                       "v3.20.0   "

//#define PRA32_U2_USE_DEBUG_PRINT

#define PRA32_U2_USE_USB_MIDI                  // Select USB Stack: "Adafruit TinyUSB" in the Arduino IDE "Tools" menu

#define PRA32_U2_USE_UART_MIDI

#define PRA32_U2_DEBUG_PRINT_USE_USB_SERIAL     // Use USB Serial (CDC) instead of UART for debug print

#if defined(PRA32_U2_DEBUG_PRINT_USE_USB_SERIAL)
#define PRA32_U2_DEBUG_PRINT_SERIAL            g_debug_print_buffer  // Transferred to USB Serial on the primary core
#else  // defined(PRA32_U2_DEBUG_PRINT_USE_USB_SERIAL)
#define PRA32_U2_DEBUG_PRINT_SERIAL            Serial1
#define PRA32_U2_DEBUG_PRINT_TX_PIN            (0)
#define PRA32_U2_DEBUG_PRINT_RX_PIN            (1)
#endif  // defined(PRA32_U2_DEBUG_PRINT_USE_USB_SERIAL)

#define PRA32_U2_UART_MIDI_SPEED               (31250)
//#define PRA32_U2_UART_MIDI_SPEED               (38400)

#define PRA32_U2_UART_MIDI_SERIAL              Serial2
#define PRA32_U2_UART_MIDI_TX_PIN              (4)
#define PRA32_U2_UART_MIDI_RX_PIN              (5)

#define PRA32_U2_MIDI_CH                       (0)  // 0-based

// for Pimoroni Pico Audio Pack (PIM544)
#define PRA32_U2_I2S_DAC_MUTE_OFF_PIN          (22)
#define PRA32_U2_I2S_DATA_PIN                  (9)
#define PRA32_U2_I2S_BCLK_PIN                  (10)  // LRCLK Pin is PRA32_U2_I2S_BCLK_PIN + 1
#define PRA32_U2_I2S_SWAP_BCLK_AND_LRCLK_PINS  (false)
#define PRA32_U2_I2S_SWAP_LEFT_AND_RIGHT       (false)

#define PRA32_U2_I2S_BUFFERS                   (2)   // Output buffer (maximum latency): 2 * 64 = 128 frames (2.7 ms)
#define PRA32_U2_I2S_BUFFER_WORDS              (64)  // Frames per buffer (= frames processed in each loop)

//#define PRA32_U2_USE_PWM_AUDIO_INSTEAD_OF_I2S

// for Pimoroni Pico VGA Demo Base (PIM553)
#define PRA32_U2_PWM_AUDIO_L_PIN               (28)
#define PRA32_U2_PWM_AUDIO_R_PIN               (27)

#define PRA32_U2_USE_2_CORES_FOR_SIGNAL_PROCESSING
//#define PRA32_U2_ENABLE_POLY_ON_1_CORE

#define PRA32_U2_USE_EMULATED_EEPROM

//#define PRA32_U2_LIMIT_DELAY_TIME_TO_SAVE_MEM

//#define PRA32_U2_USE_LEGACY_OUTPUT_LIMITER     // Use the Output Limiter (until v3.15.0) instead of the Output Compressor

//#define PRA32_U2_DISABLE_DISTORTION_FX         // The Distortion FX passes its input through (Dst as Cho), for the load
//#define PRA32_U2_DISABLE_REVERB                // Delay Mode Rev is the Stereo Delay (Ste), for the load

#define PRA32_U2_ENABLE_LAYERING                 // The Layering and the Round Robin (the Basic Channel + 4, + 5, and + 15; see README.md)

#if defined(PRA32_U2_USE_2_CORES_FOR_SIGNAL_PROCESSING)
#define PRA32_U2_SECONDARY_CORE_IN_POLY_ONLY     // The 4 Synths without waiting at every frame (see loop())
#endif  // defined(PRA32_U2_USE_2_CORES_FOR_SIGNAL_PROCESSING)

// The channels that the panel can select: the 4 synths, the Basic Channel + 4 and + 5 (the Voice Group Mode), and + 15 (the Layering)
#if defined(PRA32_U2_ENABLE_LAYERING)
#define PRA32_U2_NUMBER_OF_GROUP_CHS           (3)
#else
#define PRA32_U2_NUMBER_OF_GROUP_CHS           (0)
#endif
#define PRA32_U2_NUMBER_OF_SYNTHS              (4 + PRA32_U2_NUMBER_OF_GROUP_CHS)

////////////////////////////////////////////////////////////////

//#define PRA32_U2_USE_CONTROL_PANEL               // PRA32-U2/P (PRA32-U2 with Panel)
#define PRA32_U2_USE_PANEL_CHANNEL               // The Panel Channel (the Basic Channel + 14) of PRA32-U2/P (Experimental, see README-PRA32-U2-P.md)

#define PRA32_U2_USE_CONTROL_PANEL_KEY_INPUT     // Use tactile switches
#define PRA32_U2_KEY_INPUT_ACTIVE_LEVEL          (LOW)
#define PRA32_U2_KEY_INPUT_PIN_MODE              (INPUT_PULLUP)
#define PRA32_U2_KEY_INPUT_PREV_KEY_PIN          (16)
#define PRA32_U2_KEY_INPUT_NEXT_KEY_PIN          (18)
#define PRA32_U2_KEY_INPUT_PLAY_KEY_PIN          (20)
//#define PRA32_U2_KEY_INPUT_PROG_MINUS_KEY_PIN    (17)
//#define PRA32_U2_KEY_INPUT_PROG_PLUS_KEY_PIN     (19)
//#define PRA32_U2_KEY_INPUT_SHIFT_KEY_PIN         (21)
#define PRA32_U2_KEY_PRESS_STABLE_WAIT           (15)
#define PRA32_U2_KEY_RELEASE_STABLE_WAIT         (30)
#define PRA32_U2_KEY_LONG_PRESS_WAIT             (375)

#define PRA32_U2_USE_CONTROL_PANEL_ANALOG_INPUT  // Use ADC0, ADC1, and ADC2
#define PRA32_U2_ANALOG_INPUT_REVERSED           (true)
#define PRA32_U2_ANALOG_INPUT_CORRECTION         (-2040)
#define PRA32_U2_ANALOG_INPUT_DENOMINATOR        (480)
#define PRA32_U2_ANALOG_INPUT_THRESHOLD          (PRA32_U2_ANALOG_INPUT_DENOMINATOR)

#define PRA32_U2_USE_CONTROL_PANEL_OLED_DISPLAY  // Use SSD1306 monochrome 128x64 OLED
#define PRA32_U2_OLED_DISPLAY_I2C                (i2c1)
#define PRA32_U2_OLED_DISPLAY_I2C_SDA_PIN        (6)
#define PRA32_U2_OLED_DISPLAY_I2C_SCL_PIN        (7)
#define PRA32_U2_OLED_DISPLAY_I2C_ADDRESS        (0x3C)
#define PRA32_U2_OLED_DISPLAY_CONTRAST           (0xFF)
#define PRA32_U2_OLED_DISPLAY_ROTATION           (true)

#define PRA32_U2_DISABLE_USB_MIDI_TRANSMITTION

////////////////////////////////////////////////////////////////

uint8_t g_midi_ch = PRA32_U2_MIDI_CH;

#include "hardware/adc.h"

#include "pra32-u2-common.h"
#include "pra32-u2-synth.h"

// Written by the primary core at the start of a loop, and read by the secondary core every frame
volatile boolean g_synth_is_in_polyphonic_mode = true;
PRA32_U2_Synth<false, false, true, 0, false> g_synth;
PRA32_U2_Synth<true,  false, true, 1, true>  g_sub_1_synth;
PRA32_U2_Synth<true,  false, true, 2, true>  g_sub_2_synth;
PRA32_U2_Synth<true,  false, true, 3, true>  g_sub_3_synth;

#include <MIDI.h>
struct MySettings : public midi::DefaultSettings {
  static const long BaudRate = PRA32_U2_UART_MIDI_SPEED;
  static const bool HandleNullVelocityNoteOnAsNoteOff = false;
};
#if defined(PRA32_U2_USE_USB_MIDI)
#include <Adafruit_TinyUSB.h>
#include "pico/mutex.h"
Adafruit_USBD_MIDI usbd_midi;
// NOTE: __usb_mutex is an internal (not public API) variable of the Adafruit TinyUSB Library
// (defined in src/arduino/ports/rp2040/Adafruit_TinyUSB_rp2040.cpp, since v1.0.0).
// The library runs tud_task() in an IRQ on core 0 only when mutex_try_enter(&__usb_mutex) succeeds,
// but Adafruit_USBD_MIDI calls TinyUSB without this mutex. If the IRQ interrupts USB_MIDI.read()
// or USB_MIDI.sendRealTime() while it holds a TinyUSB internal mutex, tud_task() waits for it
// forever (freeze). So hold __usb_mutex while calling USB_MIDI in loop() to let the IRQ skip.
// If a future version of the library renames this variable, the build fails with a link error
extern mutex_t __usb_mutex;
MIDI_CREATE_CUSTOM_INSTANCE(Adafruit_USBD_MIDI, usbd_midi, USB_MIDI, MySettings);
#endif  // defined(PRA32_U2_USE_USB_MIDI)

#if defined(PRA32_U2_USE_UART_MIDI)
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, PRA32_U2_UART_MIDI_SERIAL, UART_MIDI, MySettings);
#endif

#if defined(PRA32_U2_USE_DEBUG_PRINT) && defined(PRA32_U2_DEBUG_PRINT_USE_USB_SERIAL)
#include "pra32-u2-debug-print-buffer.h"
PRA32_U2_DebugPrintBuffer g_debug_print_buffer;
#endif  // defined(PRA32_U2_USE_DEBUG_PRINT) && defined(PRA32_U2_DEBUG_PRINT_USE_USB_SERIAL)

#include "pra32-u2-control-panel.h"

#if defined(PRA32_U2_USE_PWM_AUDIO_INSTEAD_OF_I2S)
#include "pra32-u2-pwm-audio.h"
PRA32_U2_PWMAudioOutput g_pwm_output(PRA32_U2_PWM_AUDIO_L_PIN, PRA32_U2_PWM_AUDIO_R_PIN);
#endif  // defined(PRA32_U2_USE_PWM_AUDIO_INSTEAD_OF_I2S)

#include "pra32-u2-i2s.h"
PRA32_U2_I2SOutput g_i2s_output;

#include "pra32-u2-output-fader.h"
#if defined(PRA32_U2_USE_PWM_AUDIO_INSTEAD_OF_I2S)
PRA32_U2_OutputFader g_output_fader(g_pwm_output);
#else  // defined(PRA32_U2_USE_PWM_AUDIO_INSTEAD_OF_I2S)
PRA32_U2_OutputFader g_output_fader(g_i2s_output);
#endif  // defined(PRA32_U2_USE_PWM_AUDIO_INSTEAD_OF_I2S)

static volatile uint32_t s_debug_measurement_min0_us     = UINT32_MAX;
static volatile uint32_t s_debug_measurement_max0_us     = 0;
static volatile uint32_t s_debug_measurement_min1_us     = UINT32_MAX;
static volatile uint32_t s_debug_measurement_max1_us     = 0;
static volatile uint32_t s_debug_measurement_counted     = 0;

static volatile uint32_t s_secondary_core_processing_request  = 0;
// The FX inputs, which each synth adds its output to by its FX Routing (on both cores)
static PRA32_U2_FxBusSample s_fx_bus = {};

#if defined(PRA32_U2_USE_DEBUG_PRINT)
static boolean debug_print_begin(uint32_t loop_counter) {
  // Printing is only done when loop_counter is a multiple of 400
  if ((loop_counter % 400) != 0) {
    return false;
  }

#if defined(PRA32_U2_DEBUG_PRINT_USE_USB_SERIAL)
  // Skip printing if the buffer is almost full (e.g. the host is not reading) not to print a partial line
  if (PRA32_U2_DEBUG_PRINT_SERIAL.availableForWrite() < 64) {
    return false;
  }
#endif  // defined(PRA32_U2_DEBUG_PRINT_USE_USB_SERIAL)

  return true;
}
#endif  // defined(PRA32_U2_USE_DEBUG_PRINT)

void handleNoteOn(byte channel, byte pitch, byte velocity);
void handleNoteOff(byte channel, byte pitch, byte velocity);
void handleControlChange(byte channel, byte number, byte value);
void handleProgramChange(byte channel, byte number);
void handlePitchBend(byte channel, int bend);
void handleAfterTouchPoly(byte channel, byte note, byte pressure);
void handleAfterTouchChannel(byte channel, byte pressure);
void handleClock();
void handleStart();
void handleStop();
void writeProgramsToFlashAndEndSketch();

// The MIDI messages of the 4 synths, the Layering, and the Round Robin
#include "pra32-u2-m-midi.h"

void __not_in_flash_func(setup1)() {
  g_synth.initialize_secondary_core();
  g_sub_1_synth.initialize_secondary_core();
  g_sub_2_synth.initialize_secondary_core();
  g_sub_3_synth.initialize_secondary_core();

#if defined(PRA32_U2_USE_CONTROL_PANEL)
  PRA32_U2_ControlPanel_setup();
#else  // defined(PRA32_U2_USE_CONTROL_PANEL)
  delay(100);
#endif  // defined(PRA32_U2_USE_CONTROL_PANEL)

#if defined(PRA32_U2_USE_DEBUG_PRINT) && !defined(PRA32_U2_DEBUG_PRINT_USE_USB_SERIAL)
  pinMode(PRA32_U2_DEBUG_PRINT_RX_PIN, INPUT_PULLUP);
  PRA32_U2_DEBUG_PRINT_SERIAL.setTX(PRA32_U2_DEBUG_PRINT_TX_PIN);
  PRA32_U2_DEBUG_PRINT_SERIAL.setRX(PRA32_U2_DEBUG_PRINT_RX_PIN);
  PRA32_U2_DEBUG_PRINT_SERIAL.begin(115200);
#endif  // defined(PRA32_U2_USE_DEBUG_PRINT) && !defined(PRA32_U2_DEBUG_PRINT_USE_USB_SERIAL)
}

#if defined(PRA32_U2_USE_2_CORES_FOR_SIGNAL_PROCESSING)
// The FX of the frames (the Distortion, the Chorus, the Delay, and the Output Compressor) are processed by the
// secondary core, between the synths that it processes for the primary core, which has less time
// to spare: the primary core puts the FX input of each frame (s_fx_bus, of all the synths) here,
// with the count of the Main Synth for it, and takes the outputs at the end of the loop, when all
// of them are done. Those of a frame wait for the synths of the next one (but the last), which
// the primary core waits for, so that they are not delayed by the FX.
// With the 4 Synths (not in Poly, where the primary core has less time to spare), the secondary core
// has the Sub Synths 1 and 3 too, so it processes only the front of the FX (the Distortion and the
// Chorus, into the input of the Delay, in s_fx_outputs; see processSubSynthsOrFxFront()), and the
// primary core the back (the Delay and the Output Compressor) of a frame whose front is done, one a
// frame, before the Main Synth, and the rest (a few) at the end of the loop
static PRA32_U2_FxBusSample  s_fx_inputs [PRA32_U2_I2S_BUFFER_WORDS];
static uint32_t              s_fx_counts [PRA32_U2_I2S_BUFFER_WORDS];
static PRA32_U2_StereoSample s_fx_outputs[PRA32_U2_I2S_BUFFER_WORDS];
static volatile uint32_t     s_fx_frames_put  = 0;
static volatile uint32_t     s_fx_frames_done = 0;
static volatile boolean      s_fx_front_only  = false;
static uint32_t              s_fx_backs_done  = 0;  // Of the primary core

// The back of the FX of the next frame whose front is done, if any, on the primary core
static INLINE void processFxBackOfNextFrame() {
  uint32_t i = s_fx_backs_done;
  if (i < s_fx_frames_done) {
    __compiler_memory_barrier();
    s_fx_outputs[i] = g_synth.process_fx_back(s_fx_outputs[i], s_fx_inputs[i], s_fx_counts[i]);
    s_fx_backs_done = i + 1;
  }
}

// The FX of the frame i can be processed once the next frame is put (after the synths of it,
// which the primary core has waited for), or all the frames are
static INLINE boolean isFxOfFrameReady(uint32_t i, uint32_t frames_put) {
  boolean next_frame_put = (i + 1 < frames_put);
  boolean all_frames_put = (frames_put == PRA32_U2_I2S_BUFFER_WORDS) && (i < frames_put);
  return next_frame_put || all_frames_put;
}

// The front and the back of the FX of a frame in two calls, so that a request of the primary core
// in between waits for half of them at most
static void __not_in_flash_func(processFxOfNextFrame)() {
  static boolean s_back_next = false;  // The front of the frame is done, and not the back
  uint32_t i = s_fx_frames_done;
  if (isFxOfFrameReady(i, s_fx_frames_put)) {
    __compiler_memory_barrier();
    if (s_back_next) {
      s_fx_outputs[i] = g_synth.process_fx_back(s_fx_outputs[i], s_fx_inputs[i], s_fx_counts[i]);
      s_back_next = false;
      __compiler_memory_barrier();
      s_fx_frames_done = i + 1;
    } else {
      s_fx_outputs[i] = g_synth.process_fx_front(s_fx_inputs[i], s_fx_counts[i]);
      if (s_fx_front_only) {
        __compiler_memory_barrier();
        s_fx_frames_done = i + 1;
      } else {
        s_back_next = true;
      }
    }
  }
}

// With the 4 Synths, the two cores do not wait for each other at every frame: the secondary core
// processes the Sub Synths 1 and 3 of the frames one after another into s_sub_fx_buses (it does
// not depend on the primary core, as the Main Synth does not request its Voices 2 and 3 when not in
// Poly), a few frames ahead of the primary core at most, and the front of the FX of the frames
// that the primary core has put in between; the primary core processes the Sub Synth 2 and the
// Main Synth, adds the Sub Synths 1 and 3 of the frame (waiting for them only if they are not done
// yet), puts the FX input, and processes the back of the FX of a frame whose front is done
static PRA32_U2_FxBusSample  s_sub_fx_buses[PRA32_U2_I2S_BUFFER_WORDS];
static volatile uint32_t     s_sub_frames_done    = 0;
static volatile boolean      s_sub_frames_pending = false;

// The frames of the Sub Synths 1 and 3 that the secondary core may be ahead of the front of the FX
static const uint32_t SUB_FRAMES_AHEAD = 2;

static INLINE void addFxBus(PRA32_U2_FxBusSample& fx_bus, const PRA32_U2_FxBusSample& other) {
  for (uint32_t k = 0; k < 5; k++) {
    fx_bus.input[k].left  += other.input[k].left;
    fx_bus.input[k].right += other.input[k].right;
  }
}

// One step of the secondary core with the 4 Synths: the Sub Synths 1 and 3 of the next frame, or the
// front of the FX of the next frame put; true if a frame of the Sub Synths is processed
static boolean __not_in_flash_func(processSubSynthsOrFxFront)() {
  uint32_t i = s_sub_frames_done;
  uint32_t f = s_fx_frames_done;
  boolean front_ready = (f < s_fx_frames_put);
  if ((i < PRA32_U2_I2S_BUFFER_WORDS) && ((front_ready == false) || (i < f + SUB_FRAMES_AHEAD))) {
    PRA32_U2_StereoSample sub_3_synth_output = g_sub_3_synth.process(0, 0);
    PRA32_U2_StereoSample sub_1_synth_output = g_sub_1_synth.process(0, 0);
    clear_fx_bus(s_sub_fx_buses[i]);
    g_sub_3_synth.add_to_fx_bus(sub_3_synth_output, s_sub_fx_buses[i]);
    g_sub_1_synth.add_to_fx_bus(sub_1_synth_output, s_sub_fx_buses[i]);
    __compiler_memory_barrier();
    s_sub_frames_done = i + 1;
    return true;
  }
  if (front_ready) {
    __compiler_memory_barrier();
    s_fx_outputs[f] = g_synth.process_fx_front(s_fx_inputs[f], s_fx_counts[f]);
    // Done with the loop before the last front is told done (the Sub Synths of all the frames are
    // done before that is put), as the primary core may start the next loop right after that
    s_sub_frames_pending = (f + 1 < PRA32_U2_I2S_BUFFER_WORDS);
    __compiler_memory_barrier();
    s_fx_frames_done = f + 1;
  }
  return false;
}
#endif  // defined(PRA32_U2_USE_2_CORES_FOR_SIGNAL_PROCESSING)

static void __not_in_flash_func(loop1_once)() {
  boolean processed = false;

  if (s_secondary_core_processing_request == 1) {
    PRA32_U2_StereoSample sub_3_synth_output = { 0, 0 };
    PRA32_U2_StereoSample sub_1_synth_output = { 0, 0 };
    if (g_synth_is_in_polyphonic_mode == false) {
      sub_3_synth_output = g_sub_3_synth.process(0, 0);
      sub_1_synth_output = g_sub_1_synth.process(0, 0);
    }

    // In Poly, where the Sub Synths are not processed and it waits long for the request of the Main
    // Synth's Voices 2 and 3, the FX go on meanwhile, a half of those of a frame at a time (with the
    // 4 Synths, the wait is short, and the FX there would only delay the Voices 2 and 3)
    while (processed == false) {
      processed = g_synth.secondary_core_process();
#if defined(PRA32_U2_USE_2_CORES_FOR_SIGNAL_PROCESSING)
      if ((processed == false) && g_synth_is_in_polyphonic_mode) {
        processFxOfNextFrame();
      }
#endif  // defined(PRA32_U2_USE_2_CORES_FOR_SIGNAL_PROCESSING)
    }

    // Added after the Main Synth's Voices 2 and 3, which the primary core waits for in
    // the middle of g_synth.process(), so that it is done while that goes on
    g_sub_3_synth.add_to_fx_bus(sub_3_synth_output, s_fx_bus);
    g_sub_1_synth.add_to_fx_bus(sub_1_synth_output, s_fx_bus);

    __compiler_memory_barrier();
    s_secondary_core_processing_request = 0;
    processed = true;
  }
#if defined(PRA32_U2_USE_2_CORES_FOR_SIGNAL_PROCESSING)
  else if (s_sub_frames_pending) {
    processed = processSubSynthsOrFxFront();
  }
  else {
    processFxOfNextFrame();
  }
#endif  // defined(PRA32_U2_USE_2_CORES_FOR_SIGNAL_PROCESSING)

  if (processed) {
    static uint32_t s_loop_counter = 0;
    s_loop_counter++;
    if (s_loop_counter >= 16 * 400) {
      s_loop_counter = 0;
    }

    PRA32_U2_ControlPanel_update_analog_inputs(s_loop_counter);
    PRA32_U2_ControlPanel_update_display_buffer(s_loop_counter);
    PRA32_U2_ControlPanel_update_display(s_loop_counter);

#if defined(PRA32_U2_USE_DEBUG_PRINT)
    if (debug_print_begin(s_loop_counter)) {
      switch (s_loop_counter) {
      case  1 * 400:
        PRA32_U2_DEBUG_PRINT_SERIAL.print("\e[1;1H\e[K");
        PRA32_U2_DEBUG_PRINT_SERIAL.print("min(audio) ");
        PRA32_U2_DEBUG_PRINT_SERIAL.print(s_debug_measurement_min1_us);
        break;
      case  2 * 400:
        PRA32_U2_DEBUG_PRINT_SERIAL.print("\e[2;1H\e[K");
        PRA32_U2_DEBUG_PRINT_SERIAL.print("max(audio) ");
        PRA32_U2_DEBUG_PRINT_SERIAL.print(s_debug_measurement_max1_us);
        break;
      case  3 * 400:
        PRA32_U2_DEBUG_PRINT_SERIAL.print("\e[4;1H\e[K");
        PRA32_U2_DEBUG_PRINT_SERIAL.print("min(loop)  ");
        PRA32_U2_DEBUG_PRINT_SERIAL.print(s_debug_measurement_min0_us);
        break;
      case  4 * 400:
        PRA32_U2_DEBUG_PRINT_SERIAL.print("\e[5;1H\e[K");
        PRA32_U2_DEBUG_PRINT_SERIAL.print("max(loop)  ");
        PRA32_U2_DEBUG_PRINT_SERIAL.print(s_debug_measurement_max0_us);
        s_debug_measurement_min0_us = UINT32_MAX;
        s_debug_measurement_max0_us = 0;
        s_debug_measurement_min1_us = UINT32_MAX;
        s_debug_measurement_max1_us = 0;
        break;
      case  0 * 400:
        PRA32_U2_DEBUG_PRINT_SERIAL.print("\e[6;1H\e[K");
        PRA32_U2_DEBUG_PRINT_SERIAL.print("underruns  ");
#if defined(PRA32_U2_USE_PWM_AUDIO_INSTEAD_OF_I2S)
        PRA32_U2_DEBUG_PRINT_SERIAL.print(g_pwm_output.getUnderruns());
#else  // defined(PRA32_U2_USE_PWM_AUDIO_INSTEAD_OF_I2S)
        PRA32_U2_DEBUG_PRINT_SERIAL.print(g_i2s_output.getUnderruns());
#endif  // defined(PRA32_U2_USE_PWM_AUDIO_INSTEAD_OF_I2S)
        break;
      default:
        PRA32_U2_ControlPanel_debug_print(s_loop_counter);
        break;
      }
    }
#endif  // defined(PRA32_U2_USE_DEBUG_PRINT)
  }
}

// Never returns, so that the secondary core does not go back to main1() of the core
// (in the flash, through a veneer in the flash) between the samples: an XIP cache miss
// there would delay the secondary core, and the primary core waits for it
void __not_in_flash_func(loop1)() {
  for (;;) {
    loop1_once();
  }
}

void __not_in_flash_func(setup)() {
#if defined(PRA32_U2_USE_PWM_AUDIO_INSTEAD_OF_I2S)
  g_pwm_output.setSysClk(SAMPLING_RATE);
  pinMode(PRA32_U2_PWM_AUDIO_L_PIN, OUTPUT_12MA);
  pinMode(PRA32_U2_PWM_AUDIO_R_PIN, OUTPUT_12MA);
  g_pwm_output.setBufferFrames(PRA32_U2_I2S_BUFFERS * PRA32_U2_I2S_BUFFER_WORDS);
  g_pwm_output.begin(SAMPLING_RATE);
#else  // defined(PRA32_U2_USE_PWM_AUDIO_INSTEAD_OF_I2S)
  g_i2s_output.setSysClk(SAMPLING_RATE);
  g_i2s_output.setFrequency(SAMPLING_RATE);
  g_i2s_output.setDATA(PRA32_U2_I2S_DATA_PIN);
  g_i2s_output.setBCLK(PRA32_U2_I2S_BCLK_PIN);
  if (PRA32_U2_I2S_SWAP_BCLK_AND_LRCLK_PINS) {
    g_i2s_output.swapClocks();
  }
  g_i2s_output.setBufferFrames(PRA32_U2_I2S_BUFFERS * PRA32_U2_I2S_BUFFER_WORDS);
  g_i2s_output.begin();
#endif  // defined(PRA32_U2_USE_PWM_AUDIO_INSTEAD_OF_I2S)

#if defined(PRA32_U2_USE_USB_MIDI)
  TinyUSB_Device_Init(0);
#if defined(PRA32_U2_USE_DEBUG_PRINT) && defined(PRA32_U2_DEBUG_PRINT_USE_USB_SERIAL)
  TinyUSBDevice.addInterface(Serial);  // TinyUSB_Device_Init() clears the configuration including Serial
#endif  // defined(PRA32_U2_USE_DEBUG_PRINT) && defined(PRA32_U2_DEBUG_PRINT_USE_USB_SERIAL)
  USBDevice.setManufacturerDescriptor("ISGK Instruments");
#if defined(PRA32_U2_USE_CONTROL_PANEL)
  USBDevice.setProductDescriptor("PRA32-U2/M/P");
#else  // defined(PRA32_U2_USE_CONTROL_PANEL)
  USBDevice.setProductDescriptor("PRA32-U2/M");
#endif  // defined(PRA32_U2_USE_CONTROL_PANEL)
  USB_MIDI.setHandleNoteOn(handleNoteOn);
  USB_MIDI.setHandleNoteOff(handleNoteOff);
  USB_MIDI.setHandleControlChange(handleControlChange);
  USB_MIDI.setHandleProgramChange(handleProgramChange);
  USB_MIDI.setHandlePitchBend(handlePitchBend);
  USB_MIDI.setHandleAfterTouchPoly(handleAfterTouchPoly);
  USB_MIDI.setHandleAfterTouchChannel(handleAfterTouchChannel);
  USB_MIDI.setHandleClock(handleClock);
  USB_MIDI.setHandleStart(handleStart);
  USB_MIDI.setHandleStop(handleStop);
  USB_MIDI.begin(MIDI_CHANNEL_OMNI);
  USB_MIDI.turnThruOff();
#endif  // defined(PRA32_U2_USE_USB_MIDI)

#if defined(PRA32_U2_USE_UART_MIDI)
  pinMode(PRA32_U2_UART_MIDI_RX_PIN, INPUT_PULLUP);
  PRA32_U2_UART_MIDI_SERIAL.setTX(PRA32_U2_UART_MIDI_TX_PIN);
  PRA32_U2_UART_MIDI_SERIAL.setRX(PRA32_U2_UART_MIDI_RX_PIN);
  PRA32_U2_UART_MIDI_SERIAL.setFIFOSize(256);  // To absorb bursts (default: 32 bytes)
  UART_MIDI.setHandleNoteOn(handleNoteOn);
  UART_MIDI.setHandleNoteOff(handleNoteOff);
  UART_MIDI.setHandleControlChange(handleControlChange);
  UART_MIDI.setHandleProgramChange(handleProgramChange);
  UART_MIDI.setHandlePitchBend(handlePitchBend);
  UART_MIDI.setHandleAfterTouchPoly(handleAfterTouchPoly);
  UART_MIDI.setHandleAfterTouchChannel(handleAfterTouchChannel);
  UART_MIDI.setHandleClock(handleClock);
  UART_MIDI.setHandleStart(handleStart);
  UART_MIDI.setHandleStop(handleStop);
  UART_MIDI.begin(MIDI_CHANNEL_OMNI);
  UART_MIDI.turnThruOff();
#endif  // defined(PRA32_U2_USE_UART_MIDI)

#if defined(ARDUINO_RASPBERRY_PI_PICO) || defined(ARDUINO_RASPBERRY_PI_PICO_2)
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  pinMode(23, OUTPUT);  // RT6150 (PMIC) Power Save Pin
  digitalWrite(23, HIGH);
#endif  // defined(ARDUINO_RASPBERRY_PI_PICO) || defined(ARDUINO_RASPBERRY_PI_PICO_2)

#if defined(PRA32_U2_USE_PWM_AUDIO_INSTEAD_OF_I2S)
#else  // defined(PRA32_U2_USE_PWM_AUDIO_INSTEAD_OF_I2S)
#if defined(PRA32_U2_I2S_DAC_MUTE_OFF_PIN)
  pinMode(PRA32_U2_I2S_DAC_MUTE_OFF_PIN, OUTPUT);
  digitalWrite(PRA32_U2_I2S_DAC_MUTE_OFF_PIN, HIGH);
#endif  // defined(PRA32_U2_I2S_DAC_MUTE_OFF_PIN)
#endif  // defined(PRA32_U2_USE_PWM_AUDIO_INSTEAD_OF_I2S)

  g_synth.initialize();
  g_sub_1_synth.initialize();
  g_sub_2_synth.initialize();
  g_sub_3_synth.initialize();

  PRA32_U2_ControlPanel_initialize_parameters();

  delay(100);
}

void __not_in_flash_func(loop)() {

#if defined(PRA32_U2_USE_DEBUG_PRINT)
  uint32_t debug_measurement_start0_us = micros();
#endif  // defined(PRA32_U2_USE_DEBUG_PRINT)

#if defined(PRA32_U2_USE_USB_MIDI)
  mutex_enter_blocking(&__usb_mutex);
#endif  // defined(PRA32_U2_USE_USB_MIDI)

#if defined(PRA32_U2_USE_USB_MIDI)
  // The MIDI Library parses 1 byte per read(), so read 5 bytes per 64 frames, more than about 4.2
  // bytes that arrive per 64 frames at 31250 bps, to be at least as fast as UART MIDI
  for (uint32_t i = 0; i < ((PRA32_U2_I2S_BUFFER_WORDS + 15) / 16) + 1; i++) {
    USB_MIDI.read();
  }
#endif  // defined(PRA32_U2_USE_USB_MIDI)

#if defined(PRA32_U2_USE_UART_MIDI)
  // The MIDI Library parses 1 byte per read(), and up to about 4.2 bytes arrive per 64 frames
  // at 31250 bps, so read 8 bytes per 64 frames not to overflow the receive FIFO
  for (uint32_t i = 0; i < (PRA32_U2_I2S_BUFFER_WORDS + 7) / 8; i++) {
    UART_MIDI.read();
  }
#endif  // defined(PRA32_U2_USE_UART_MIDI)

  processOnePendingProgramChange();  // Those of the panel (PRA32-U2/P) in the next loops

  PRA32_U2_ControlPanel_update_control();

#if defined(PRA32_U2_USE_USB_MIDI)
  mutex_exit(&__usb_mutex);
#endif  // defined(PRA32_U2_USE_USB_MIDI)

  g_output_fader.begin_block();

  boolean mode = g_synth.is_in_polyphonic_mode();
  if (g_synth_is_in_polyphonic_mode != mode) {
    g_synth_is_in_polyphonic_mode = mode;
    g_sub_1_synth.all_notes_off(true);
    g_sub_2_synth.all_notes_off(true);
    g_sub_3_synth.all_notes_off(true);
#if defined(PRA32_U2_ENABLE_LAYERING)
    roundRobinReset();
#endif  // defined(PRA32_U2_ENABLE_LAYERING)
  }

#if defined(PRA32_U2_USE_DEBUG_PRINT)
  uint32_t debug_measurement_start1_us = micros();
#endif  // defined(PRA32_U2_USE_DEBUG_PRINT)

  int32_t left_buffer[PRA32_U2_I2S_BUFFER_WORDS];
  int32_t right_buffer[PRA32_U2_I2S_BUFFER_WORDS];
#if defined(PRA32_U2_USE_2_CORES_FOR_SIGNAL_PROCESSING)
  // The FX of the frames are processed by the secondary core (see processFxOfNextFrame()),
  // which has done all of those of the last loop, but the back by the primary core (processFxBackOfNextFrame())
  // with the 4 Synths
  s_fx_frames_put = 0;
  s_fx_front_only = (g_synth_is_in_polyphonic_mode == false);
  s_fx_backs_done = s_fx_front_only ? 0 : PRA32_U2_I2S_BUFFER_WORDS;
  __compiler_memory_barrier();
  s_fx_frames_done = 0;

  if (g_synth_is_in_polyphonic_mode == false) {
    s_sub_frames_done = 0;
    __compiler_memory_barrier();
    s_sub_frames_pending = true;

    for (uint32_t i = 0; i < PRA32_U2_I2S_BUFFER_WORDS; i++) {
      PRA32_U2_StereoSample sub_2_synth_output = g_sub_2_synth.process(0, 0);

      processFxBackOfNextFrame();

      PRA32_U2_StereoSample synth_output = g_synth.process<false, true>(0, 0);

      PRA32_U2_FxBusSample fx_bus;
      clear_fx_bus(fx_bus);
      g_sub_2_synth.add_to_fx_bus(sub_2_synth_output, fx_bus);
      g_synth.add_to_fx_bus(synth_output, fx_bus);

      while (s_sub_frames_done <= i) {
        ;
      }
      __compiler_memory_barrier();
      addFxBus(fx_bus, s_sub_fx_buses[i]);

      s_fx_inputs[i] = fx_bus;
      s_fx_counts[i] = g_synth.get_count();
      __compiler_memory_barrier();
      s_fx_frames_put = i + 1;
    }
  } else
#endif  // defined(PRA32_U2_USE_2_CORES_FOR_SIGNAL_PROCESSING)
  for (uint32_t i = 0; i < PRA32_U2_I2S_BUFFER_WORDS; i++) {
    s_secondary_core_processing_request = 1;

    // Each synth adds its output to the FX input selected by its FX Routing.
    // The secondary core adds its synths only after g_synth.process() requests
    // the Main Synth's Voices 2 and 3, so s_fx_bus can be written before that
    PRA32_U2_StereoSample sub_2_synth_output = { 0, 0 };
    if (g_synth_is_in_polyphonic_mode == false) {
      sub_2_synth_output = g_sub_2_synth.process(0, 0);
    }

    clear_fx_bus(s_fx_bus);
    g_sub_2_synth.add_to_fx_bus(sub_2_synth_output, s_fx_bus);
    __compiler_memory_barrier();

#if defined(PRA32_U2_USE_2_CORES_FOR_SIGNAL_PROCESSING)
    processFxBackOfNextFrame();
#endif  // defined(PRA32_U2_USE_2_CORES_FOR_SIGNAL_PROCESSING)

    PRA32_U2_StereoSample synth_output = g_synth.process<false, true>(0, 0);

    while (s_secondary_core_processing_request) {
      ;
    }
    __compiler_memory_barrier();

    g_synth.add_to_fx_bus(synth_output, s_fx_bus);

#if defined(PRA32_U2_USE_2_CORES_FOR_SIGNAL_PROCESSING)
    s_fx_inputs[i] = s_fx_bus;
    s_fx_counts[i] = g_synth.get_count();
    __compiler_memory_barrier();
    s_fx_frames_put = i + 1;
  }

  while (s_fx_frames_done < PRA32_U2_I2S_BUFFER_WORDS) {
    ;
  }
  __compiler_memory_barrier();

  while (s_fx_backs_done < PRA32_U2_I2S_BUFFER_WORDS) {
    processFxBackOfNextFrame();
  }

  for (uint32_t i = 0; i < PRA32_U2_I2S_BUFFER_WORDS; i++) {
    int32_t gain = g_output_fader.next_gain_q8();
    left_buffer[i] = s_fx_outputs[i].left * gain;
    right_buffer[i] = s_fx_outputs[i].right * gain;
  }
#else  // defined(PRA32_U2_USE_2_CORES_FOR_SIGNAL_PROCESSING)
    PRA32_U2_StereoSample synth_fx_output = g_synth.process_fx(s_fx_bus);
    int32_t gain = g_output_fader.next_gain_q8();
    left_buffer[i] = synth_fx_output.left * gain;
    right_buffer[i] = synth_fx_output.right * gain;
  }
#endif  // defined(PRA32_U2_USE_2_CORES_FOR_SIGNAL_PROCESSING)

#if defined(PRA32_U2_USE_DEBUG_PRINT)
  uint32_t debug_measurement_end_us = micros();
#endif  // defined(PRA32_U2_USE_DEBUG_PRINT)

#if defined(PRA32_U2_USE_PWM_AUDIO_INSTEAD_OF_I2S)
  for (uint32_t i = 0; i < PRA32_U2_I2S_BUFFER_WORDS; i++) {
    g_pwm_output.write24(left_buffer[i], right_buffer[i]);
  }
#else  // defined(PRA32_U2_USE_PWM_AUDIO_INSTEAD_OF_I2S)
  for (uint32_t i = 0; i < PRA32_U2_I2S_BUFFER_WORDS; i++) {
    if (PRA32_U2_I2S_SWAP_LEFT_AND_RIGHT) {
      g_i2s_output.write24(right_buffer[i], left_buffer[i]);
    } else {
      g_i2s_output.write24(left_buffer[i], right_buffer[i]);
    }
  }
#endif  // defined(PRA32_U2_USE_PWM_AUDIO_INSTEAD_OF_I2S)

#if defined(PRA32_U2_USE_DEBUG_PRINT)
#if defined(PRA32_U2_DEBUG_PRINT_USE_USB_SERIAL)
#if defined(PRA32_U2_USE_USB_MIDI)
  mutex_enter_blocking(&__usb_mutex);  // Serial also calls TinyUSB (see the NOTE on __usb_mutex)
#endif  // defined(PRA32_U2_USE_USB_MIDI)
  g_debug_print_buffer.transfer_to(Serial);  // Non-blocking
#if defined(PRA32_U2_USE_USB_MIDI)
  mutex_exit(&__usb_mutex);
#endif  // defined(PRA32_U2_USE_USB_MIDI)
#endif  // defined(PRA32_U2_DEBUG_PRINT_USE_USB_SERIAL)

  uint32_t debug_measurement_elapsed0_us = debug_measurement_end_us - debug_measurement_start0_us;
  s_debug_measurement_min0_us -= s_debug_measurement_counted *
                                 (debug_measurement_elapsed0_us < s_debug_measurement_min0_us) *
                                 (s_debug_measurement_min0_us - debug_measurement_elapsed0_us);
  s_debug_measurement_max0_us += s_debug_measurement_counted *
                                 (debug_measurement_elapsed0_us > s_debug_measurement_max0_us) *
                                 (debug_measurement_elapsed0_us - s_debug_measurement_max0_us);

  uint32_t debug_measurement_elapsed1_us = debug_measurement_end_us - debug_measurement_start1_us;
  s_debug_measurement_min1_us -= s_debug_measurement_counted *
                                 (debug_measurement_elapsed1_us < s_debug_measurement_min1_us) *
                                 (s_debug_measurement_min1_us - debug_measurement_elapsed1_us);
  s_debug_measurement_max1_us += s_debug_measurement_counted *
                                 (debug_measurement_elapsed1_us > s_debug_measurement_max1_us) *
                                 (debug_measurement_elapsed1_us - s_debug_measurement_max1_us);

  s_debug_measurement_counted = 1;
#endif  // defined(PRA32_U2_USE_DEBUG_PRINT)
}

void __not_in_flash_func(handleClock)()
{
#if defined(PRA32_U2_USE_CONTROL_PANEL)
  PRA32_U2_ControlPanel_on_clock();
#endif  // defined(PRA32_U2_USE_CONTROL_PANEL)
}

void __not_in_flash_func(handleStart)()
{
#if defined(PRA32_U2_USE_CONTROL_PANEL)
  if (g_synth.current_controller_value(SEQ_TRX_ST_SP  ) >= 64) {
    PRA32_U2_ControlPanel_on_start();
  }
#endif  // defined(PRA32_U2_USE_CONTROL_PANEL)
}

void __not_in_flash_func(handleStop)()
{
#if defined(PRA32_U2_USE_CONTROL_PANEL)
  if (g_synth.current_controller_value(SEQ_TRX_ST_SP  ) >= 64) {
    PRA32_U2_ControlPanel_on_stop();
  }
#endif  // defined(PRA32_U2_USE_CONTROL_PANEL)
}


// The functions for the control panel (getCurrentControllerValue(), getTargetMIDICh(), etc.) are in "pra32-u2-m-midi.h"
