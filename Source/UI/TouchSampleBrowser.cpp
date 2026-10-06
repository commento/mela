#include "TouchSampleBrowser.h"
#include "MelaLookAndFeel.h"

TouchSampleBrowser::TouchSampleBrowser()
    : fileList("Sample nella Inbox", this)
{
    setOpaque(true);
    addAndMakeVisible(titleLabel);
    addAndMakeVisible(emptyLabel);
    addAndMakeVisible(fileList);
    addAndMakeVisible(cancelButton);
    addAndMakeVisible(loadButton);

    titleLabel.setText("SCEGLI UN SAMPLE DALLA INBOX", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setFont(juce::FontOptions(28.0f, juce::Font::bold));
    emptyLabel.setText("La Inbox e' vuota", juce::dontSendNotification);
    emptyLabel.setJustificationType(juce::Justification::centred);
    emptyLabel.setFont(juce::FontOptions(24.0f));

    fileList.setRowHeight(72);
    fileList.setMultipleSelectionEnabled(false);
    fileList.setColour(juce::ListBox::backgroundColourId, MelaColours::background);
    fileList.setColour(juce::ListBox::outlineColourId, MelaColours::border);
    fileList.setOutlineThickness(1);
    if (auto* viewport = fileList.getViewport())
    {
        viewport->setScrollBarThickness(58);
        viewport->setScrollOnDragMode(juce::Viewport::ScrollOnDragMode::all);
        auto& scrollBar = viewport->getVerticalScrollBar();
        scrollBar.setColour(juce::ScrollBar::backgroundColourId,
                            MelaColours::background);
        scrollBar.setColour(juce::ScrollBar::trackColourId,
                            MelaColours::background.withAlpha(0.55f));
        scrollBar.setColour(juce::ScrollBar::thumbColourId,
                            MelaColours::muted);
    }

    loadButton.setColour(juce::TextButton::buttonColourId, MelaColours::active);
    cancelButton.onClick = [this]
    {
        setVisible(false);
        if (onCancel)
            onCancel();
    };
    loadButton.onClick = [this] { chooseSelectedFile(); };
}

void TouchSampleBrowser::showFiles(const std::vector<juce::File>& newFiles)
{
    files = newFiles;
    fileList.updateContent();
    fileList.scrollToEnsureRowIsOnscreen(0);
    fileList.setVisible(! files.empty());
    emptyLabel.setVisible(files.empty());
    loadButton.setEnabled(! files.empty());

    if (! files.empty())
        fileList.selectRow(0);

    setVisible(true);
    toFront(true);
}

void TouchSampleBrowser::paint(juce::Graphics& graphics)
{
    graphics.fillAll(MelaColours::background);
    auto panel = getLocalBounds().toFloat().reduced(48.0f);
    graphics.setColour(MelaColours::surface);
    graphics.fillRoundedRectangle(panel, 6.0f);
    graphics.setColour(MelaColours::border);
    graphics.drawRoundedRectangle(panel, 6.0f, 1.0f);
}

void TouchSampleBrowser::resized()
{
    auto area = getLocalBounds().reduced(70, 54);
    titleLabel.setBounds(area.removeFromTop(76));
    area.removeFromTop(14);

    auto buttons = area.removeFromBottom(92);
    area.removeFromBottom(16);
    fileList.setBounds(area);
    emptyLabel.setBounds(area);

    const auto buttonWidth = juce::jmin(310, (buttons.getWidth() - 24) / 2);
    cancelButton.setBounds(buttons.removeFromLeft(buttonWidth));
    loadButton.setBounds(buttons.removeFromRight(buttonWidth));

    const auto rowHeight = juce::jmax(72, getHeight() / 11);
    fileList.setRowHeight(rowHeight);
    if (auto* viewport = fileList.getViewport())
        viewport->setSingleStepSizes(0, rowHeight);
}

int TouchSampleBrowser::getNumRows()
{
    return static_cast<int>(files.size());
}

void TouchSampleBrowser::paintListBoxItem(int row, juce::Graphics& graphics,
                                          int width, int height, bool rowIsSelected)
{
    if (! juce::isPositiveAndBelow(row, static_cast<int>(files.size())))
        return;

    graphics.fillAll(rowIsSelected ? MelaColours::active : MelaColours::background);
    graphics.setColour(MelaColours::text);
    graphics.setFont(juce::FontOptions(juce::jlimit(
                                           22.0f, 34.0f, static_cast<float>(height) * 0.38f),
                                       juce::Font::bold));
    graphics.drawText(files[static_cast<size_t>(row)].getFileName(),
                      juce::Rectangle<int>(24, 0, width - 48, height),
                      juce::Justification::centredLeft, true);
    graphics.setColour(MelaColours::border);
    graphics.drawHorizontalLine(height - 1, 12.0f, static_cast<float>(width - 12));
}

void TouchSampleBrowser::selectedRowsChanged(int lastRowSelected)
{
    loadButton.setEnabled(
        juce::isPositiveAndBelow(lastRowSelected, static_cast<int>(files.size())));
}

void TouchSampleBrowser::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    if (juce::isPositiveAndBelow(row, static_cast<int>(files.size())))
    {
        fileList.selectRow(row);
        chooseSelectedFile();
    }
}

void TouchSampleBrowser::chooseSelectedFile()
{
    const auto row = fileList.getSelectedRow();
    if (! juce::isPositiveAndBelow(row, static_cast<int>(files.size())))
        return;

    const auto file = files[static_cast<size_t>(row)];
    setVisible(false);
    if (onFileChosen)
        onFileChosen(file);
}
