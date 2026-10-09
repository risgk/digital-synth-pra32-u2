#pragma once

// A wrapper of the PRA32-U2 core, without JUCE.
// The core runs at 48 kHz only (see PRA32U2Resampler for other sampling rates)

#include <cstdint>
#include <memory>
#include <string>

class PRA32U2Engine {
public:
  static constexpr int kSamplingRate = 48000;

  struct ParameterInfo {
    uint8_t     controlNumber;
    const char* name;
  };

  // The sound parameters (the same as the parameters of the programs, s_program_table_parameters of the core)
  static int                  getNumParameters();
  static const ParameterInfo& getParameterInfo(int index);

  // The markers of the slider of a sound parameter, the same as PRA32-U2 Editor (e.g. 0, 26, 51, 77, 102, and 127
  // for "Osc 1 Wave"). Returns the number of the markers (0 if none), and sets markerValues to the values
  static int getMarkers(uint8_t controlNumber, const uint8_t** markerValues);

  // Whether a sound parameter is randomized by "Randomize FX", the same as PRA32-U2 Editor (e.g. "Chorus Mix")
  static bool isRandomizedAsFx(uint8_t controlNumber);

  // The text of a controller value, the same as PRA32-U2 Editor (e.g. "26 [Sqr]", "67 [+3]", or "100")
  static std::string getValueText(uint8_t controlNumber, uint8_t value);

  PRA32U2Engine();
  ~PRA32U2Engine();

  PRA32U2Engine(const PRA32U2Engine&) = delete;
  PRA32U2Engine& operator=(const PRA32U2Engine&) = delete;

  // Handles a MIDI channel voice message (all channels are received, i.e. Omni On).
  // System messages (including SysEx) are ignored
  void handleMidiMessage(const uint8_t* data, int size);

  void controlChange(uint8_t controlNumber, uint8_t value);
  uint8_t getControllerValue(uint8_t controlNumber) const;

  // Processes 1 sample at 48 kHz (-1.0 to +1.0)
  void process(float& left, float& right);

private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;
};
