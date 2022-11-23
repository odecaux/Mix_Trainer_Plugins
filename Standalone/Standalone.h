
#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>

inline juce::Colour getUIColourIfAvailable (juce::LookAndFeel_V4::ColourScheme::UIColour uiColour, juce::Colour fallback = juce::Colour (0xff4d4d4d)) noexcept
{
    if (auto* v4 = dynamic_cast<juce::LookAndFeel_V4*> (&juce::LookAndFeel::getDefaultLookAndFeel()))
        return v4->getCurrentColourScheme().getUIColour (uiColour);

    return fallback;
}

inline juce::File getExamplesDirectory() noexcept
{
    auto currentFile = juce::File::getSpecialLocation (juce::File::SpecialLocationType::currentApplicationFile);
    auto exampleDir = currentFile.getParentDirectory().getChildFile ("examples");

    if (exampleDir.exists())
        return exampleDir;

    // keep track of the number of parent directories so we don't go on endlessly
    for (int numTries = 0; numTries < 15; ++numTries)
    {
        if (currentFile.getFileName() == "examples")
            return currentFile;

        const auto sibling = currentFile.getSiblingFile ("examples");

        if (sibling.exists())
            return sibling;

        currentFile = currentFile.getParentDirectory();
    }

    return currentFile;
}

inline std::unique_ptr<juce::InputStream> createAssetInputStream (const char* resourcePath)
{
   #if JUCE_MAC
    auto assetsDir = juce::File::getSpecialLocation (juce::File::currentExecutableFile)
        .getParentDirectory().getParentDirectory().getChildFile ("Resources").getChildFile ("Assets");

    if (! assetsDir.exists())
        assetsDir = getExamplesDirectory().getChildFile ("Assets");
   #else
    auto assetsDir = getExamplesDirectory().getChildFile ("Assets");
   #endif

    auto resourceFile = assetsDir.getChildFile (resourcePath);
    jassert (resourceFile.existsAsFile());

    return resourceFile.createInputStream();
}


inline std::unique_ptr<juce::InputSource> makeInputSource (const juce::URL& url)
{
    return std::make_unique<juce::URLInputSource> (url);
}

//==============================================================================
class DemoThumbnailComp  :
    public juce::Component,
    public juce::ChangeListener,
    public juce::FileDragAndDropTarget,
    public juce::ChangeBroadcaster,
    private juce::ScrollBar::Listener,
    private juce::Timer
{
public:
    DemoThumbnailComp (juce::AudioFormatManager& formatManager,
                       juce::AudioTransportSource& source,
                       juce::Slider& slider)
    : transportSource (source),
      zoomSlider (slider),
      thumbnail (512, formatManager, thumbnailCache)
    {
        thumbnail.addChangeListener (this);

        addAndMakeVisible (scrollbar);
        scrollbar.setRangeLimits (visibleRange);
        scrollbar.setAutoHide (false);
        scrollbar.addListener (this);

        currentPositionMarker.setFill (juce::Colours::white.withAlpha (0.85f));
        addAndMakeVisible (currentPositionMarker);
    }

    ~DemoThumbnailComp() override
    {
        scrollbar.removeListener (this);
        thumbnail.removeChangeListener (this);
    }

    void setURL (const juce::URL& url)
    {
        if (auto inputSource = makeInputSource (url))
        {
            thumbnail.setSource (inputSource.release());

            juce::Range<double> newRange (0.0, thumbnail.getTotalLength());
            scrollbar.setRangeLimits (newRange);
            setRange (newRange);

            startTimerHz (40);
        }
    }

    juce::URL getLastDroppedFile() const noexcept { return lastFileDropped; }

    void setZoomFactor (double amount)
    {
        if (thumbnail.getTotalLength() > 0)
        {
            auto newScale = juce::jmax (0.001, thumbnail.getTotalLength() * (1.0 - juce::jlimit (0.0, 0.99, amount)));
            auto timeAtCentre = xToTime ((float) getWidth() / 2.0f);

            setRange ({ timeAtCentre - newScale * 0.5, timeAtCentre + newScale * 0.5 });
        }
    }

    void setRange (juce::Range<double> newRange)
    {
        visibleRange = newRange;
        scrollbar.setCurrentRange (visibleRange);
        updateCursorPosition();
        repaint();
    }

    void setFollowsTransport (bool shouldFollow)
    {
        isFollowingTransport = shouldFollow;
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::darkgrey);
        g.setColour (juce::Colours::lightblue);

        if (thumbnail.getTotalLength() > 0.0)
        {
            auto thumbArea = getLocalBounds();

            thumbArea.removeFromBottom (scrollbar.getHeight() + 4);
            thumbnail.drawChannels (g, thumbArea.reduced (2),
                                    visibleRange.getStart(), visibleRange.getEnd(), 1.0f);
        }
        else
        {
            g.setFont (14.0f);
            g.drawFittedText ("(No audio file selected)", getLocalBounds(), juce::Justification::centred, 2);
        }
    }

    void resized() override
    {
        scrollbar.setBounds (getLocalBounds().removeFromBottom (14).reduced (2));
    }

    void changeListenerCallback (juce::ChangeBroadcaster*) override
    {
        // this method is called by the thumbnail when it has changed, so we should repaint it..
        repaint();
    }

    bool isInterestedInFileDrag (const juce::StringArray& /*files*/) override
    {
        return true;
    }

    void filesDropped (const juce::StringArray& files, int /*x*/, int /*y*/) override
    {
        lastFileDropped = juce::URL (juce::File (files[0]));
        sendChangeMessage();
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        mouseDrag (e);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (canMoveTransport())
            transportSource.setPosition (juce::jmax (0.0, xToTime ((float) e.x)));
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        transportSource.start();
    }

    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
    {
        if (thumbnail.getTotalLength() > 0.0)
        {
            auto newStart = visibleRange.getStart() - wheel.deltaX * (visibleRange.getLength()) / 10.0;
            newStart = juce::jlimit (0.0, juce::jmax (0.0, thumbnail.getTotalLength() - (visibleRange.getLength())), newStart);

            if (canMoveTransport())
                setRange ({ newStart, newStart + visibleRange.getLength() });

            if (wheel.deltaY != 0.0f)
                zoomSlider.setValue (zoomSlider.getValue() - wheel.deltaY);

            repaint();
        }
    }

private:
    juce::AudioTransportSource& transportSource;
    juce::Slider& zoomSlider;
    juce::ScrollBar scrollbar  { false };

    juce::AudioThumbnailCache thumbnailCache  { 5 };
    juce::AudioThumbnail thumbnail;
    juce::Range<double> visibleRange;
    bool isFollowingTransport = false;
    juce::URL lastFileDropped;

    juce::DrawableRectangle currentPositionMarker;

    float timeToX (const double time) const
    {
        if (visibleRange.getLength() <= 0)
            return 0;

        return (float) getWidth() * (float) ((time - visibleRange.getStart()) / visibleRange.getLength());
    }

    double xToTime (const float x) const
    {
        return (x / (float) getWidth()) * (visibleRange.getLength()) + visibleRange.getStart();
    }

    bool canMoveTransport() const noexcept
    {
        return ! (isFollowingTransport && transportSource.isPlaying());
    }

    void scrollBarMoved (juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart) override
    {
        if (scrollBarThatHasMoved == &scrollbar)
            if (! (isFollowingTransport && transportSource.isPlaying()))
                setRange (visibleRange.movedToStartAt (newRangeStart));
    }

    void timerCallback() override
    {
        if (canMoveTransport())
            updateCursorPosition();
        else
            setRange (visibleRange.movedToStartAt (transportSource.getCurrentPosition() - (visibleRange.getLength() / 2.0)));
    }

    void updateCursorPosition()
    {
        currentPositionMarker.setVisible (transportSource.isPlaying() || isMouseButtonDown());

        currentPositionMarker.setRectangle (juce::Rectangle<float> (timeToX (transportSource.getCurrentPosition()) - 0.75f, 0,
                                            1.5f, (float) (getHeight() - scrollbar.getHeight())));
    }
};

//==============================================================================
class Main_Panel  : 
    public juce::Component,
    private juce::FileBrowserListener,
    private juce::ChangeListener
{
public:
    Main_Panel(juce::AudioFormatManager &formatManager,
               juce::AudioTransportSource &transportSource,
               std::function < bool(const juce::URL&) > loadURLIntoTransport,
               std::function < void() > startOrStop) :
        formatManager(formatManager),
        loadURLIntoTransport(std::move(loadURLIntoTransport)),
        startOrStop(std::move(startOrStop))
    {
        
        fileExplorerThread.startThread (3);

        //UI (boring)
        {
            addAndMakeVisible (zoomLabel);
            zoomLabel.setFont (juce::Font (15.00f, juce::Font::plain));
            zoomLabel.setJustificationType (juce::Justification::centredRight);
            zoomLabel.setEditable (false, false, false);
            zoomLabel.setColour (juce::TextEditor::textColourId, juce::Colours::black);
            zoomLabel.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0x00000000));
        }
        {
            addAndMakeVisible (followTransportButton);
            followTransportButton.onClick = [this] { updateFollowTransportState(); };
        }
        {
            addAndMakeVisible (fileTreeComp);

            directoryList.setDirectory (juce::File::getSpecialLocation (juce::File::userHomeDirectory), true, true);

            fileTreeComp.setTitle ("Files");
            fileTreeComp.setColour (juce::FileTreeComponent::backgroundColourId, juce::Colours::lightgrey.withAlpha (0.6f));
            fileTreeComp.addListener (this);
        }
        {
            addAndMakeVisible (explanation);
            explanation.setFont (juce::Font (14.00f, juce::Font::plain));
            explanation.setJustificationType (juce::Justification::bottomRight);
            explanation.setEditable (false, false, false);
            explanation.setColour (juce::TextEditor::textColourId, juce::Colours::black);
            explanation.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0x00000000));
        }
        {
            addAndMakeVisible (zoomSlider);
            zoomSlider.setRange (0, 1, 0);
            zoomSlider.onValueChange = [this] { thumbnail->setZoomFactor (zoomSlider.getValue()); };
            zoomSlider.setSkewFactor (2);
        }
        {
            thumbnail = std::make_unique < DemoThumbnailComp > (formatManager, transportSource, zoomSlider);
            addAndMakeVisible (thumbnail.get());
            thumbnail->addChangeListener (this);
        }
        { 
            addAndMakeVisible (startStopButton);
            startStopButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff79ed7f));
            startStopButton.setColour (juce::TextButton::textColourOffId, juce::Colours::black);
            startStopButton.onClick = [this] { this->startOrStop(); };
        }


        setOpaque (true);
        setSize (500, 500);
    }

    ~Main_Panel() override
    {
        fileTreeComp.removeListener (this);

        thumbnail->removeChangeListener (this);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (getUIColourIfAvailable (juce::LookAndFeel_V4::ColourScheme::UIColour::windowBackground));
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (4);

        auto controls = r.removeFromBottom (90);

        auto controlRightBounds = controls.removeFromRight (controls.getWidth() / 3);

        explanation.setBounds (controlRightBounds);

        auto zoom = controls.removeFromTop (25);
        zoomLabel .setBounds (zoom.removeFromLeft (50));
        zoomSlider.setBounds (zoom);

        followTransportButton.setBounds (controls.removeFromTop (25));
        startStopButton      .setBounds (controls);

        r.removeFromBottom (6);

        thumbnail->setBounds (r.removeFromBottom (140));
        r.removeFromBottom (6);

        fileTreeComp.setBounds (r);
    }

private:
    juce::AudioFormatManager &formatManager;
    std::function < bool(const juce::URL& audioURL) > loadURLIntoTransport;
    std::function < void() > startOrStop;
    
    juce::TimeSliceThread fileExplorerThread  { "File Explorer thread" };
    juce::DirectoryContentsList directoryList {nullptr, fileExplorerThread};
    juce::FileTreeComponent fileTreeComp {directoryList};
    juce::Label explanation { {}, "Select an audio file in the treeview above, and this page will display its waveform, and let you play it.." };

    std::unique_ptr<DemoThumbnailComp> thumbnail;
    juce::Label zoomLabel                     { {}, "zoom:" };
    juce::Slider zoomSlider                   { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };
    juce::ToggleButton followTransportButton  { "Follow Transport" };
    juce::TextButton startStopButton          { "Play/Stop" };

    //==============================================================================
    void showAudioResource (juce::URL resource)
    {
        if (loadURLIntoTransport (resource))
        {
            zoomSlider.setValue (0, juce::dontSendNotification);
            thumbnail->setURL (resource);
        }
    }

    void updateFollowTransportState()
    {
        thumbnail->setFollowsTransport (followTransportButton.getToggleState());
    }
    
    void selectionChanged() override
    {
        showAudioResource (juce::URL (fileTreeComp.getSelectedFile()));
    }

    void fileClicked (const juce::File&, const juce::MouseEvent&) override          {}
    void fileDoubleClicked (const juce::File&) override                       {}
    void browserRootChanged (const juce::File&) override                      {}

    void changeListenerCallback (juce::ChangeBroadcaster* source) override
    {
        //file drag and drop
        if (source == thumbnail.get())
            showAudioResource (juce::URL (thumbnail->getLastDroppedFile()));
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Main_Panel)
};


class Main_Component : public juce::Component
{
    public :
    
    Main_Component()
    {
        // audio setup
        formatManager.registerBasicFormats();

        readAheadThread.startThread (3);

        audioDeviceManager.initialise (0, 2, nullptr, true, {}, nullptr);

        audioDeviceManager.addAudioCallback (&audioSourcePlayer);
        audioSourcePlayer.setSource (&transportSource);

        panel = std::make_unique < Main_Panel > (
            formatManager,
            transportSource,
            [&] (const auto& audioURL) -> bool { return loadURLIntoTransport(audioURL); },
            [&] { startOrStop(); }
        );
        setSize(panel->getWidth(), panel->getHeight());
        addAndMakeVisible(*panel);
    }
    
    ~Main_Component()
    {
        transportSource  .setSource (nullptr);
        audioSourcePlayer.setSource (nullptr);

        audioDeviceManager.removeAudioCallback (&audioSourcePlayer);
    }

    void resized() override 
    {
        panel->setBounds(getLocalBounds());
    }

    private :
    
    juce::AudioDeviceManager audioDeviceManager;
    juce::AudioFormatManager formatManager;
    juce::TimeSliceThread readAheadThread  { "audio file preview" };
    
    //juce::URL currentAudioFile;
    juce::AudioSourcePlayer audioSourcePlayer;
    juce::AudioTransportSource transportSource;
    std::unique_ptr<juce::AudioFormatReaderSource> currentAudioFileSource;

    std::unique_ptr<Main_Panel> panel;
    
    bool loadURLIntoTransport (const juce::URL& audioURL)
    {
        // unload the previous file source and delete it..
        transportSource.stop();
        transportSource.setSource (nullptr);
        currentAudioFileSource.reset();

        const auto source = makeInputSource (audioURL);

        if (source == nullptr)
            return false;

        auto stream = juce::rawToUniquePtr (source->createInputStream());

        if (stream == nullptr)
            return false;

        auto reader = juce::rawToUniquePtr (formatManager.createReaderFor (std::move (stream)));

        if (reader == nullptr)
            return false;

        currentAudioFileSource = std::make_unique<juce::AudioFormatReaderSource> (reader.release(), true);

        // ..and plug it into our transport source
        transportSource.setSource (currentAudioFileSource.get(),
                                   32768,                   // tells it to buffer this many samples ahead
                                   &readAheadThread,                 // this is the background thread to use for reading-ahead
                                   currentAudioFileSource->getAudioFormatReader()->sampleRate);     // allows for sample rate correction

        return true;
    }

    void startOrStop()
    {
        if (transportSource.isPlaying())
        {
            transportSource.stop();
        }
        else
        {
            transportSource.setPosition (0);
            transportSource.start();
        }
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Main_Component)
};