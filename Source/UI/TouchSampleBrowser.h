#pragma once

#include <JuceHeader.h>
#include <functional>
#include <vector>

class TouchSampleBrowser final : public juce::Component,
                                 private juce::ListBoxModel
{
public:
    TouchSampleBrowser();

    std::function<void(const juce::File&)> onFileChosen;
    std::function<void()> onCancel;

    void showFiles(const std::vector<juce::File>& newFiles);

    void paint(juce::Graphics& graphics) override;
    void resized() override;

private:
    int getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics& graphics, int width, int height,
                          bool rowIsSelected) override;
    void selectedRowsChanged(int lastRowSelected) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;
    void chooseSelectedFile();

    juce::Label titleLabel;
    juce::Label emptyLabel;
    juce::ListBox fileList;
    juce::TextButton cancelButton { "ANNULLA" };
    juce::TextButton loadButton { "CARICA SAMPLE" };
    std::vector<juce::File> files;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TouchSampleBrowser)
};
