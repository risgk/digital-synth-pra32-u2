#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <iterator>

namespace {

const char* const STATE_TAG     = "PRA32U2Native";
const int         STATE_VERSION = 1;

juce::String parameterIdFor(uint8_t controlNumber) {
  return "cc" + juce::String(controlNumber);
}

}  // namespace

PRA32U2NativeAudioProcessor::PRA32U2NativeAudioProcessor()
: AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
  const int numParameters = PRA32U2Engine::getNumParameters();

  m_parameters.resize(static_cast<size_t>(numParameters), nullptr);
  m_hostValues.resize(static_cast<size_t>(numParameters));
  m_synthValues.resize(static_cast<size_t>(numParameters));
  m_knownValues.resize(static_cast<size_t>(numParameters));
  m_pendingValues = std::make_unique<std::atomic<int>[]>(static_cast<size_t>(numParameters));

  // The index of the sound parameter for each control number (-1: not a sound parameter)
  int indexForControlNumber[128];
  std::fill(std::begin(indexForControlNumber), std::end(indexForControlNumber), -1);
  for (int i = 0; i < numParameters; ++i) {
    indexForControlNumber[PRA32U2Engine::getParameterInfo(i).controlNumber] = i;
  }

  // The parameters are added in the order of the control numbers (CC#0-127), so that the parameter
  // numbers (indices) are the same as the control numbers; with JUCE_FORCE_USE_LEGACY_PARAM_IDS,
  // they are also the VST3 parameter IDs. The control numbers that are not the sound parameters
  // have the parameters named "---" (e.g. "CC#1 ---"), which do nothing (these controls work by MIDI)
  for (int controlNumber = 0; controlNumber < 128; ++controlNumber) {
    const int i = indexForControlNumber[controlNumber];
    if (i < 0) {
      addParameter(new PlaceholderParameter(juce::ParameterID { parameterIdFor(static_cast<uint8_t>(controlNumber)), 1 },
                                            "CC#" + juce::String(controlNumber) + " ---"));
      continue;
    }

    const PRA32U2Engine::ParameterInfo& info = PRA32U2Engine::getParameterInfo(i);
    const int defaultValue = m_renderer.getEngine().getControllerValue(info.controlNumber);

    const auto attributes = juce::AudioParameterIntAttributes()
      .withStringFromValueFunction([controlNumber](int value, int) {
        return juce::String(PRA32U2Engine::getValueText(static_cast<uint8_t>(controlNumber),
                                                        static_cast<uint8_t>(juce::jlimit(0, 127, value))));
      })
      .withValueFromStringFunction([](const juce::String& text) {
        return juce::jlimit(0, 127, text.getIntValue());  // The controller value before " [...]"
      });

    // The name begins with the control number, for the hosts that show only the beginning of the names.
    // The supplement in [] (e.g. " [Saw|Sqr|Tri|Sin|WT|Pls]") is only in the Standalone, so that the names in
    // the hosts are not too long (e.g. Cubase omits the middle of a long name); the values show it (e.g. "0 [Saw]")
    juce::String name(info.name);
    if (wrapperType != wrapperType_Standalone) {
      name = name.upToFirstOccurrenceOf(" [", false, false);
    }

    // The default value is the value of Program #0 (the synth is initialized with Program #0)
    auto* parameter = new juce::AudioParameterInt(juce::ParameterID { parameterIdFor(info.controlNumber), 1 },
                                                  "CC#" + juce::String(controlNumber) + " " + name,
                                                  0, 127, defaultValue, attributes);
    addParameter(parameter);

    const size_t index = static_cast<size_t>(i);
    m_parameters [index] = parameter;
    m_hostValues [index] = defaultValue;
    m_synthValues[index] = defaultValue;
    m_knownValues[index] = defaultValue;
    m_pendingValues[index].store(-1);
  }
}

PRA32U2NativeAudioProcessor::~PRA32U2NativeAudioProcessor() {
  cancelPendingUpdate();
}

void PRA32U2NativeAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
  m_renderer.prepare(sampleRate);
  m_scratch.assign(static_cast<size_t>(juce::jmax(samplesPerBlock, 1)), 0.0f);
  setLatencySamples(m_renderer.getLatencySamples());
}

void PRA32U2NativeAudioProcessor::releaseResources() {
}

bool PRA32U2NativeAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
  const auto& output = layouts.getMainOutputChannelSet();
  return (output == juce::AudioChannelSet::stereo()) || (output == juce::AudioChannelSet::mono());
}

void PRA32U2NativeAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
  juce::ScopedNoDenormals noDenormals;

  const int numSamples  = buffer.getNumSamples();
  const int numChannels = buffer.getNumChannels();
  const int numParameters = static_cast<int>(m_parameters.size());

  // Parameters changed by the host -> the synth (as CCs at the start of the block)
  for (int i = 0; i < numParameters; ++i) {
    const size_t index = static_cast<size_t>(i);
    const int value = m_parameters[index]->get();
    if (value != m_hostValues[index]) {
      m_hostValues[index] = value;
      if (value != m_knownValues[index]) {
        m_knownValues[index] = value;
        const uint8_t message[3] = { 0xB0, PRA32U2Engine::getParameterInfo(i).controlNumber, static_cast<uint8_t>(value) };
        m_renderer.addMidiEvent(0, message, 3);
      }
    }
  }

  // Program Change requested by sendProgramChange() (after the parameters, so that it takes effect)
  const int requestedProgram = m_requestedProgram.exchange(-1);
  if (requestedProgram >= 0) {
    const uint8_t message[2] = { 0xC0, static_cast<uint8_t>(requestedProgram & 0x7F) };
    m_renderer.addMidiEvent(0, message, 2);
  }

  for (const auto metadata : midiMessages) {
    m_renderer.addMidiEvent(metadata.samplePosition, metadata.data, metadata.numBytes);
  }
  midiMessages.clear();

  if (numChannels <= 0 || numSamples <= 0) {
    return;
  }

  float* left = buffer.getWritePointer(0);
  float* right;
  if (numChannels >= 2) {
    right = buffer.getWritePointer(1);
  } else {
    if (m_scratch.size() < static_cast<size_t>(numSamples)) {
      m_scratch.resize(static_cast<size_t>(numSamples));  // Should not happen
    }
    right = m_scratch.data();
  }

  m_renderer.render(left, right, numSamples);

  if (numChannels == 1) {
    for (int n = 0; n < numSamples; ++n) {
      left[n] = (left[n] + right[n]) * 0.5f;
    }
  }

  for (int channel = 2; channel < numChannels; ++channel) {
    buffer.clear(channel, 0, numSamples);
  }

  // Values changed by the synth (MIDI CCs, Program Changes, etc.) -> the parameters
  bool changed = false;
  for (int i = 0; i < numParameters; ++i) {
    const size_t index = static_cast<size_t>(i);
    const int value = m_renderer.getEngine().getControllerValue(PRA32U2Engine::getParameterInfo(i).controlNumber);
    if (value != m_synthValues[index]) {
      m_synthValues[index] = value;
      if (value != m_knownValues[index]) {
        m_knownValues[index] = value;
        m_pendingValues[index].store(value);
        changed = true;
      }
    }
  }

  if (changed) {
    triggerAsyncUpdate();
  }
}

void PRA32U2NativeAudioProcessor::handleAsyncUpdate() {
  for (size_t index = 0; index < m_parameters.size(); ++index) {
    const int value = m_pendingValues[index].exchange(-1);
    if (value >= 0) {
      auto* parameter = m_parameters[index];
      parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(value)));
    }
  }
}

void PRA32U2NativeAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
  juce::XmlElement xml(STATE_TAG);
  xml.setAttribute("version", STATE_VERSION);

  for (size_t index = 0; index < m_parameters.size(); ++index) {
    const uint8_t controlNumber = PRA32U2Engine::getParameterInfo(static_cast<int>(index)).controlNumber;
    xml.setAttribute(parameterIdFor(controlNumber), m_parameters[index]->get());
  }

  copyXmlToBinary(xml, destData);
}

void PRA32U2NativeAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
  if (xml == nullptr || !xml->hasTagName(STATE_TAG)) {
    return;
  }

  for (size_t index = 0; index < m_parameters.size(); ++index) {
    const uint8_t controlNumber = PRA32U2Engine::getParameterInfo(static_cast<int>(index)).controlNumber;
    const juce::String id = parameterIdFor(controlNumber);
    if (xml->hasAttribute(id)) {
      const int value = juce::jlimit(0, 127, xml->getIntAttribute(id));
      auto* parameter = m_parameters[index];
      parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(value)));
    }
  }
}

juce::String PRA32U2NativeAudioProcessor::getFactoryPresetName(int programNumber) {
  // The same as PRA32-U2 Editor
  static const char* const NAMES[kNumFactoryPresets] = {
    "Synth Pad" , "M Saw Pad" , "FM Piano"  , "Simple"    ,
    "Saw Lead"  , "Sync Lead" , "Synth Bass", "Initial"   ,
    "Synth Brs" , "Synth Str" , "WT Pad"    , "Elec Organ",
    "Fifth Lead", "Sqr Lead"  , "PWM Lead"  , "---"       ,
  };

  const int index = programNumber - kFactoryPresetFirst;
  if (index < 0 || index >= kNumFactoryPresets) {
    return "#" + juce::String(programNumber);
  }
  return "#" + juce::String(programNumber) + " " + NAMES[index];
}

juce::String PRA32U2NativeAudioProcessor::getSoundParameterFullName(int index) {
  const PRA32U2Engine::ParameterInfo& info = PRA32U2Engine::getParameterInfo(index);
  return "CC#" + juce::String(info.controlNumber) + " " + juce::String(info.name);
}

juce::AudioProcessorEditor* PRA32U2NativeAudioProcessor::createEditor() {
  return new PRA32U2NativeAudioProcessorEditor(*this);
}

void PRA32U2NativeAudioProcessor::sendProgramChange(int programNumber) {
  m_requestedProgram.store(juce::jlimit(0, 127, programNumber));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new PRA32U2NativeAudioProcessor();
}
