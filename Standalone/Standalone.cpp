#include "Standalone_UI.h"

class Application  : public juce::JUCEApplication
{
public:
    //==============================================================================
    Application() = default;

    const juce::String getApplicationName() override       { return "Mix Trainer"; }
    const juce::String getApplicationVersion() override    { return "0.0.1"; }

    void initialise (const juce::String&) override
    {
        formatManager.registerBasicFormats();
        auto *main_component = new Main_Component(&formatManager);
        auto window_title = juce::String("Mix Trainer");
        mainWindow = std::make_unique < MainWindow > (&window_title, main_component, this);
    }

    void shutdown() override { mainWindow = nullptr; }

private:
    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow (juce::String* name, juce::Component* c, juce::JUCEApplication *a)
        : DocumentWindow (*name, juce::Desktop::getInstance().getDefaultLookAndFeel()
                          .findColour (ResizableWindow::backgroundColourId),
                          juce::DocumentWindow::allButtons),
                          app (a)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (c, true);

            setResizable (true, false);
            setResizeLimits (400, 300, 10000, 10000);
            centreWithSize (getWidth(), getHeight());

            setVisible (true);
        }

        void closeButtonPressed() override
        {
            app->systemRequestedQuit();
        }

    private:
        juce::JUCEApplication *app;

        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

    std::unique_ptr<MainWindow> mainWindow;
    juce::AudioFormatManager formatManager;
};

//==============================================================================
START_JUCE_APPLICATION (Application)
