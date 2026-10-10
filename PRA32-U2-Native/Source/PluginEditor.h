#pragma once

#include <memory>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"

// The editor of the VST3 and the Standalone: the same as JUCE's GenericAudioProcessorEditor (sliders with the
// value texts), except that the names are as wide as needed (measured with the font), so that they
// are shown in full as in PRA32-U2 Editor (e.g. "CC#102 Osc 1 Wave [Saw|Sqr|Tri|Sin|WT|Pls]"); the names and the values are left-justified.
// Only the sound parameters are shown, in the same order as PRA32-U2 Editor (not the parameters named "---").
// The header at the top shows the name and the version, and the
// "Program Change" button (the same style as the "Options" button of the Standalone), whose menu sends
// Program Change #16-31 (the Factory Presets) to
// the synth itself (not the hosts' presets, so that the hosts do not change the sound when they load the projects),
// and the "Randomize Synth" and "Randomize FX" buttons (the same as PRA32-U2 Editor).
// (The hosts' generic editors list the parameters in the order of the control numbers, with the ones named "---")
class PRA32U2NativeAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
  explicit PRA32U2NativeAudioProcessorEditor(PRA32U2NativeAudioProcessor& processor);
  ~PRA32U2NativeAudioProcessorEditor() override;

  void paint(juce::Graphics& g) override;
  void resized() override;

private:
  class ParameterRow;

  // The same as JUCE's default look and feel, except that the value texts of the sliders are left-justified,
  // with the font of the rows
  class LookAndFeel : public juce::LookAndFeel_V4 {
  public:
    static constexpr float ROW_FONT_HEIGHT = 14.0f;  // 15 in JUCE's Label; a little smaller, for the lower rows

    juce::Label* createSliderTextBox(juce::Slider& slider) override {
      auto* label = juce::LookAndFeel_V4::createSliderTextBox(slider);
      label->setJustificationType(juce::Justification::centredLeft);
      label->setFont(juce::FontOptions(ROW_FONT_HEIGHT));
      return label;
    }
  };

  // The same as juce::Viewport, except that the vertical scroll bar leaves a margin at the bottom for the corner resizer
  class Viewport : public juce::Viewport {
  public:
    static constexpr int BOTTOM_MARGIN = 18;  // The size of the corner resizer of juce::AudioProcessorEditor

    void resized() override {
      juce::Viewport::resized();
      trimScrollBar();
    }

    // juce::Viewport sets the bounds of the scroll bar before calling this (e.g. when scrolled)
    void visibleAreaChanged(const juce::Rectangle<int>&) override {
      trimScrollBar();
    }

  private:
    void trimScrollBar() {
      auto& scrollBar = getVerticalScrollBar();
      scrollBar.setBounds(scrollBar.getX(), 0, scrollBar.getWidth(), juce::jmax(0, getHeight() - BOTTOM_MARGIN));
    }
  };

  void showProgramChangeMenu();

  PRA32U2NativeAudioProcessor&                m_processor;
  LookAndFeel                                 m_lookAndFeel;  // Declared before the rows, so that it outlives them
  bool                                        m_hasHeader;
  juce::String                                m_nameAndVersion;
  int                                         m_nameAndVersionWidth = 0;
  juce::TextButton                            m_programChangeButton;
  juce::TextButton                            m_randomizeSynthButton;
  juce::TextButton                            m_randomizeFxButton;
  Viewport                                    m_viewport;
  juce::Component                             m_content;
  std::vector<std::unique_ptr<ParameterRow>>  m_rows;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PRA32U2NativeAudioProcessorEditor)
};
