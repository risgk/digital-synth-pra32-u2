#include "PluginEditor.h"

#include <cmath>

namespace {

const int NAME_WIDTH_MIN = 200;  // The width of the names is measured with the font (it differs among the OSes)
const int SLIDER_WIDTH   = 220;
const int VALUE_WIDTH    = 90;
const int ROW_HEIGHT     = 32;   // 40 in JUCE's generic editor; lower, so that 20 rows fit in a 768 px high screen
const int VISIBLE_ROWS   = 20;
const int HEADER_HEIGHT  = 28;   // The header (the name, the version, and the "Program Change" button)
const int HEADER_MARGIN  = 8;
const float HEADER_FONT_HEIGHT = 15.0f;

}  // namespace

// A slider with the markers (short lines below the track), the same as PRA32-U2 Editor
class MarkedSlider : public juce::Slider {
public:
  MarkedSlider() : juce::Slider(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight) {}

  void setMarkers(const uint8_t* values, int numValues) {
    m_markers.assign(values, values + numValues);
    repaint();
  }

  void paint(juce::Graphics& g) override {
    juce::Slider::paint(g);

    if (m_markers.empty()) {
      return;
    }

    const auto layout = getLookAndFeel().getSliderLayout(*this);
    const float centreY = static_cast<float>(layout.sliderBounds.getCentreY());
    g.setColour(findColour(juce::Slider::textBoxTextColourId).withAlpha(0.6f));
    for (const uint8_t value : m_markers) {
      const float x = static_cast<float>(getPositionOfValue(static_cast<double>(value)));
      g.drawLine(x, centreY + 5.0f, x, centreY + 9.0f, 1.0f);
    }
  }

private:
  std::vector<uint8_t> m_markers;
};

// A row of a parameter: the name, the slider, and the value text (e.g. "26 [Sqr]"), which can be edited
class PRA32U2NativeAudioProcessorEditor::ParameterRow : public juce::Component {
public:
  ParameterRow(juce::RangedAudioParameter& parameter, uint8_t controlNumber, const juce::String& name,
               juce::LookAndFeel& lookAndFeel)
  : m_nameWidth(NAME_WIDTH_MIN)
  , m_attachment(parameter, m_slider)
  {
    m_name.setText(name, juce::dontSendNotification);
    m_name.setJustificationType(juce::Justification::centredLeft);  // centredRight in JUCE's generic editor
    m_name.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(m_name);

    m_slider.setLookAndFeel(&lookAndFeel);  // The value text is left-justified
    m_slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, VALUE_WIDTH, 20);
    m_slider.setScrollWheelEnabled(false);
    const uint8_t* markerValues = nullptr;
    const int numMarkers = PRA32U2Engine::getMarkers(controlNumber, &markerValues);
    m_slider.setMarkers(markerValues, numMarkers);
    addAndMakeVisible(m_slider);
  }

  // The width needed to show the name in full (with the borders of the label)
  int getNeededNameWidth() const {
    const auto border = m_name.getBorderSize();
    const float textWidth = juce::GlyphArrangement::getStringWidth(m_name.getFont(), m_name.getText());
    return static_cast<int>(std::ceil(textWidth)) + border.getLeftAndRight() + 4;
  }

  void setNameWidth(int nameWidth) {
    m_nameWidth = nameWidth;
    resized();
  }

  void resized() override {
    auto area = getLocalBounds();
    m_name.setBounds(area.removeFromLeft(m_nameWidth));
    area.removeFromLeft(8);
    m_slider.setBounds(area.reduced(0, (ROW_HEIGHT - 20) / 2));
  }

private:
  int                             m_nameWidth;
  juce::Label                     m_name;
  MarkedSlider                    m_slider;
  juce::SliderParameterAttachment m_attachment;
};

PRA32U2NativeAudioProcessorEditor::PRA32U2NativeAudioProcessorEditor(PRA32U2NativeAudioProcessor& processor)
: AudioProcessorEditor(processor)
, m_processor(processor)
, m_hasHeader(true)
, m_programChangeButton("Program Change")
{
  if (m_hasHeader) {
    m_nameAndVersion = juce::String(JucePlugin_Name) + " v" + JucePlugin_VersionString;
    m_nameAndVersionWidth = static_cast<int>(std::ceil(
        juce::GlyphArrangement::getStringWidth(juce::FontOptions(HEADER_FONT_HEIGHT), m_nameAndVersion)));

    // The same as the "Options" button of the Standalone (StandaloneFilterWindow)
    m_programChangeButton.setTriggeredOnMouseDown(true);
    m_programChangeButton.onClick = [this] { showProgramChangeMenu(); };
    addAndMakeVisible(m_programChangeButton);
  }

  const auto& parameters = processor.getSoundParameters();
  for (size_t i = 0; i < parameters.size(); ++i) {
    m_rows.push_back(std::make_unique<ParameterRow>(*parameters[i],
                                                    PRA32U2Engine::getParameterInfo(static_cast<int>(i)).controlNumber,
                                                    PRA32U2NativeAudioProcessor::getSoundParameterFullName(static_cast<int>(i)),
                                                    m_lookAndFeel));
    m_content.addAndMakeVisible(*m_rows.back());
  }

  int nameWidth = NAME_WIDTH_MIN;
  for (const auto& row : m_rows) {
    nameWidth = juce::jmax(nameWidth, row->getNeededNameWidth());
  }
  const int rowWidth = nameWidth + 8 + SLIDER_WIDTH + VALUE_WIDTH;

  m_content.setSize(rowWidth, ROW_HEIGHT * static_cast<int>(m_rows.size()));
  for (size_t i = 0; i < m_rows.size(); ++i) {
    m_rows[i]->setNameWidth(nameWidth);
    m_rows[i]->setBounds(0, ROW_HEIGHT * static_cast<int>(i), rowWidth, ROW_HEIGHT);
  }

  m_viewport.setViewedComponent(&m_content, false);
  m_viewport.setScrollBarsShown(true, false);
  addAndMakeVisible(m_viewport);

  setOpaque(true);
  setSize(rowWidth + m_viewport.getScrollBarThickness(),
          (m_hasHeader ? HEADER_HEIGHT : 0) + juce::jmin(ROW_HEIGHT * VISIBLE_ROWS, m_content.getHeight()));
}

PRA32U2NativeAudioProcessorEditor::~PRA32U2NativeAudioProcessorEditor() = default;

void PRA32U2NativeAudioProcessorEditor::paint(juce::Graphics& g) {
  g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

  if (m_hasHeader) {
    auto header = getLocalBounds().removeFromTop(HEADER_HEIGHT);
    g.setColour(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId).darker(0.3f));
    g.fillRect(header);
    g.setColour(getLookAndFeel().findColour(juce::Label::textColourId));
    g.setFont(juce::FontOptions(HEADER_FONT_HEIGHT));
    g.drawText(m_nameAndVersion, header.withTrimmedLeft(HEADER_MARGIN), juce::Justification::centredLeft, false);
  }
}

void PRA32U2NativeAudioProcessorEditor::resized() {
  auto area = getLocalBounds();
  if (m_hasHeader) {
    auto header = area.removeFromTop(HEADER_HEIGHT);
    header.removeFromLeft(HEADER_MARGIN + m_nameAndVersionWidth + HEADER_MARGIN * 2);
    m_programChangeButton.setBounds(header.withWidth(110).reduced(0, 5));
  }
  m_viewport.setBounds(area);
}

void PRA32U2NativeAudioProcessorEditor::showProgramChangeMenu() {
  // The items refer to the processor, which outlives the editor
  PRA32U2NativeAudioProcessor& processor = m_processor;

  juce::PopupMenu menu;
  for (int i = 0; i < PRA32U2NativeAudioProcessor::kNumFactoryPresets; ++i) {
    const int program = PRA32U2NativeAudioProcessor::kFactoryPresetFirst + i;
    menu.addItem(PRA32U2NativeAudioProcessor::getFactoryPresetName(program), [&processor, program] {
      processor.sendProgramChange(program);
    });
  }

  menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&m_programChangeButton));
}
