#include <JuceHeader.h>
#include "MainComponent.h"

class MelaApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override    { return "Mela"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override          { return false; }

    void initialise(const juce::String& commandLine) override
    {
        mainWindow = std::make_unique<MainWindow>(
            getApplicationName(), ! commandLine.contains("--windowed"));

        const auto splashImage = juce::ImageFileFormat::loadFrom(
            BinaryData::mela_splash_cartoon_png,
            static_cast<size_t>(BinaryData::mela_splash_cartoon_pngSize));
        if (splashImage.isValid())
        {
            splashScreen = new juce::SplashScreen("Mela", splashImage, false);
            splashScreen->deleteAfterDelay(juce::RelativeTime::seconds(2.5), false);
        }
    }

    void shutdown() override
    {
       #if JUCE_LINUX
        juce::Desktop::getInstance().setKioskModeComponent(nullptr);
       #endif
        mainWindow.reset();
    }

    void systemRequestedQuit() override
    {
        quit();
    }

private:
    class MainWindow final : public juce::DocumentWindow
    {
    public:
        MainWindow(const juce::String& name, bool useKioskMode)
            : DocumentWindow(name,
                             MelaColours::aubergine,
                            #if JUCE_LINUX
                             useKioskMode ? 0 : DocumentWindow::allButtons)
                            #else
                             DocumentWindow::allButtons)
                            #endif
        {
           #if JUCE_LINUX
            setUsingNativeTitleBar(! useKioskMode);
           #else
            setUsingNativeTitleBar(true);
           #endif
            setContentOwned(new MainComponent(), true);
            setResizable(! useKioskMode, ! useKioskMode);
            centreWithSize(getWidth(), getHeight());
            setVisible(true);
           #if JUCE_LINUX
            if (useKioskMode)
            {
                setMouseCursor(juce::MouseCursor(juce::MouseCursor::NoCursor));
                juce::Desktop::getInstance().setKioskModeComponent(this, false);
            }
           #else
            juce::ignoreUnused(useKioskMode);
           #endif
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };

    std::unique_ptr<MainWindow> mainWindow;
    juce::SplashScreen* splashScreen = nullptr;
};

START_JUCE_APPLICATION(MelaApplication)
