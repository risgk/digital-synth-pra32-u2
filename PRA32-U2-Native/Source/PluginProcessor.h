#pragma once

#include <memory>
#include <atomic>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

#include "PRA32U2Renderer.h"

// PRA32-U2 Native (VST3 / Standalone).
// The parameters are the same as the Control Changes of PRA32-U2 (0-127, i.e. 127 = 1.0).
// The current values of the synth (changed by MIDI CCs or Program Changes) are reflected to the parameters
class PRA32U2NativeAudioProcessor : public juce::AudioProcessor,
                                    private juce::AsyncUpdater {
public:
  PRA32U2NativeAudioProcessor();
  ~PRA32U2NativeAudioProcessor() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
  void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
  using juce::AudioProcessor::processBlock;

  // The editor (see PluginEditor.h), for the VST3 and the Standalone
  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override { return true; }

  const juce::String getName() const override { return JucePlugin_Name; }
  bool acceptsMidi() const override { return true; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 0.0; }

  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const juce::String getProgramName(int) override { return {}; }
  void changeProgramName(int, const juce::String&) override {}

  void getStateInformation(juce::MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

  // Sends a Program Change to the synth itself (thread safe; handled at the start of the next block)
  void sendProgramChange(int programNumber);

  // The Factory Presets (#16-31), with the names of PRA32-U2 Editor (e.g. "#16 Synth Pad")
  static constexpr int kFactoryPresetFirst = 16;
  static constexpr int kNumFactoryPresets  = 16;
  static juce::String getFactoryPresetName(int programNumber);

  // The sound parameters, in the order of the parameters of the programs (the same as PRA32-U2 Editor).
  // The parameters of the processor (getParameters()) are in the order of the control numbers (CC#0-127)
  const std::vector<juce::AudioParameterInt*>& getSoundParameters() const { return m_parameters; }

  // The full name of a sound parameter, with the supplement in [] (e.g. "CC#102 Osc 1 Wave [Saw|Sqr|Tri|Sin|WT|Pls]");
  // the names of the VST3 parameters do not have the supplements
  static juce::String getSoundParameterFullName(int index);

private:
  // A parameter named "---" for a control number that is not a sound parameter (does nothing, not automatable;
  // the control works by MIDI), so that the parameter numbers are the same as the control numbers
  class PlaceholderParameter : public juce::AudioParameterInt {
  public:
    PlaceholderParameter(const juce::ParameterID& parameterId, const juce::String& name)
    : juce::AudioParameterInt(parameterId, name, 0, 127, 0,
                              juce::AudioParameterIntAttributes().withStringFromValueFunction([](int, int) { return juce::String("-"); }))
    {}

    bool isAutomatable() const override { return false; }
  };

  void handleAsyncUpdate() override;

  PRA32U2Renderer                          m_renderer;
  std::vector<juce::AudioParameterInt*>    m_parameters;  // The sound parameters (see getSoundParameters())

  std::vector<float>                       m_scratch;  // For a mono output

  // Used on the audio thread only (indexed by the parameter index)
  std::vector<int>                         m_hostValues;   // The last values of the parameters
  std::vector<int>                         m_synthValues;  // The last values of the synth
  std::vector<int>                         m_knownValues;  // The values the synth has (or will have)

  // The values changed by the synth, to be set to the parameters on the message thread (-1: none)
  std::unique_ptr<std::atomic<int>[]>      m_pendingValues;

  std::atomic<int>                         m_requestedProgram { -1 };  // -1: none

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PRA32U2NativeAudioProcessor)
};
