/*
    ==============================================================================

    This file contains the startup code for a PIP.

    ==============================================================================
    */

#include "Standalone.h"



class Application    : public juce::JUCEApplication
{
public:
    //==============================================================================
    Application() = default;

    const juce::String getApplicationName() override       { return "Mixer Trainer"; }
    const juce::String getApplicationVersion() override    { return "0.0.1"; }

    void initialise (const juce::String&) override
    {
        mainWindow = std::make_unique<MainWindow>("Mixer Trainer", new Main_Component, *this);
    }

    void shutdown() override                         { mainWindow = nullptr; }

private:
    class MainWindow    : public juce::DocumentWindow
    {
    public:
        MainWindow (const juce::String& name, juce::Component* c, JUCEApplication& a)
        : DocumentWindow (name, juce::Desktop::getInstance().getDefaultLookAndFeel()
                          .findColour (ResizableWindow::backgroundColourId),
                          juce::DocumentWindow::allButtons),
                          app (a)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (c, true);

            setResizable (true, false);
            setResizeLimits (300, 250, 10000, 10000);
            centreWithSize (getWidth(), getHeight());

            setVisible (true);
        }

        void closeButtonPressed() override
        {
            app.systemRequestedQuit();
        }

    private:
        JUCEApplication& app;

        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

    std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
START_JUCE_APPLICATION (Application)
