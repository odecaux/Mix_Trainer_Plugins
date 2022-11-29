
#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "../shared/shared.h"
#include "../Game/Game.h"
#include "FrequencyWidget.h"

inline juce::Colour getUIColourIfAvailable (juce::LookAndFeel_V4::ColourScheme::UIColour uiColour, juce::Colour fallback = juce::Colour (0xff4d4d4d)) noexcept
{
    if (auto* v4 = dynamic_cast<juce::LookAndFeel_V4*> (&juce::LookAndFeel::getDefaultLookAndFeel()))
        return v4->getCurrentColourScheme().getUIColour (uiColour);

    return fallback;
}

inline std::unique_ptr<juce::InputSource> makeInputSource (const juce::File& file)
{
    return std::make_unique<juce::FileInputSource> (file);
}

juce::dsp::IIR::Coefficients<float>::Ptr make_coefficients(DSP_Filter_Type type, double sample_rate, float frequency, float quality, float gain)
{
    switch (type) {
        case Filter_None:
            return new juce::dsp::IIR::Coefficients<float> (1, 0, 1, 0);
        case Filter_Low_Pass:
            return juce::dsp::IIR::Coefficients<float>::makeLowPass (sample_rate, frequency, quality);
        case Filter_LowPass1st:
            return juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass (sample_rate, frequency);
        case Filter_LowShelf:
            return juce::dsp::IIR::Coefficients<float>::makeLowShelf (sample_rate, frequency, quality, gain);
        case Filter_BandPass:
            return juce::dsp::IIR::Coefficients<float>::makeBandPass (sample_rate, frequency, quality);
        case Filter_AllPass:
            return juce::dsp::IIR::Coefficients<float>::makeAllPass (sample_rate, frequency, quality);
        case Filter_AllPass1st:
            return juce::dsp::IIR::Coefficients<float>::makeFirstOrderAllPass (sample_rate, frequency);
        case Filter_Notch:
            return juce::dsp::IIR::Coefficients<float>::makeNotch (sample_rate, frequency, quality);
        case Filter_Peak:
            return juce::dsp::IIR::Coefficients<float>::makePeakFilter (sample_rate, frequency, quality, gain);
        case Filter_HighShelf:
            return juce::dsp::IIR::Coefficients<float>::makeHighShelf (sample_rate, frequency, quality, gain);
        case Filter_HighPass1st:
            return juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass (sample_rate, frequency);
        case Filter_HighPass:
            return juce::dsp::IIR::Coefficients<float>::makeHighPass (sample_rate, frequency, quality);
        case Filter_LastID:
        default:
            return nullptr;
    }
}


struct Channel_DSP_Callback : public juce::AudioSource
{
    Channel_DSP_Callback(juce::AudioSource* inputSource) : input_source(inputSource)
    {
    }
    
    virtual ~Channel_DSP_Callback() {}

    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        jassert(bufferToFill.buffer != nullptr);
        input_source->getNextAudioBlock (bufferToFill);

        juce::ScopedNoDenormals noDenormals;

        juce::dsp::AudioBlock<float> block(*bufferToFill.buffer, (size_t) bufferToFill.startSample);
        juce::dsp::ProcessContextReplacing<float> context (block);
        {
            juce::ScopedLock processLock (lock);
            dsp_chain.process (context);
        }
    }
    
    void prepareToPlay (int blockSize, double sampleRate) override
    {
        sample_rate = sampleRate;
        input_source->prepareToPlay (blockSize, sampleRate);
        dsp_chain.prepare ({ sampleRate, (juce::uint32) blockSize, 2 }); //TODO always stereo ?
        updateDspChain();
    }

    void releaseResources() override 
    {
        input_source->releaseResources();
    };

    void push_new_dsp_state(Channel_DSP_State new_state)
    {
        state = new_state;
        if(sample_rate != -1.0)
            updateDspChain();
    }

    Channel_DSP_State state;

    using FilterBand = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    using Gain       = juce::dsp::Gain<float>;
    juce::dsp::ProcessorChain<FilterBand, Gain> dsp_chain;
    double sample_rate = -1.0;
    juce::CriticalSection lock;
    juce::AudioSource *input_source;

private:
    void updateDspChain()
    {
        for (auto i = 0; i < 1 /* une seule bande pour l'instant */; ++i) {
            auto new_coefficients = make_coefficients(state.bands[i].type, sample_rate, state.bands[i].frequency, state.bands[i].quality, state.bands[i].gain);
            jassert(new_coefficients);
            // minimise lock scope, get<0>() needs to be a  compile time constant
            {
                juce::ScopedLock processLock (lock);
                if (i == 0)
                    *dsp_chain.get<0>().state = *new_coefficients;
            }
        }
        //gain
        dsp_chain.get<1>().setGainLinear ((float)state.gain);
    }
};

struct FilePlayer {
    FilePlayer()
    : dsp_callback(&transportSource)
    {
        // audio setup
        formatManager.registerBasicFormats();

        readAheadThread.startThread (3);

        audioDeviceManager.initialise (0, 2, nullptr, true, {}, nullptr);
        
        dsp_callback.push_new_dsp_state(ChannelDSP_on());
        audioSourcePlayer.setSource (&dsp_callback);
        audioDeviceManager.addAudioCallback (&audioSourcePlayer);
    }

    ~FilePlayer()
    {
        transportSource  .setSource (nullptr);
        audioSourcePlayer.setSource (nullptr);

        audioDeviceManager.removeAudioCallback (&audioSourcePlayer);
    }

    bool loadFileIntoTransport (const juce::File& audio_file)
    {
        // unload the previous file source and delete it..
        transportSource.stop();
        transportSource.setSource (nullptr);
        currentAudioFileSource.reset();

        const auto source = makeInputSource (audio_file);

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
        transportSource.setLooping(true);
        return true;
    }

    Return_Value post_command(Audio_Command command)
    {
        switch (command.type)
        {
            case Audio_Command_Play :
            {
                DBG("Play");
                transportSource.start();
                transport_state.step = Transport_Playing;
            } break;
            case Audio_Command_Pause :
            {
                DBG("Pause");
                transportSource.stop();
                transport_state.step = Transport_Paused;
            } break;
            case Audio_Command_Stop :
            {
                DBG("Stop");
                transportSource.stop();
                transportSource.setPosition(0);
                transport_state.step = Transport_Stopped;
            } break;
            case Audio_Command_Seek :
            {
                DBG("Seek : "<<command.value_f);
                transportSource.setPosition(command.value_f);
            } break;
            case Audio_Command_Load :
            {
                DBG("Load : "<<command.value_file.getFileName());
                bool success = loadFileIntoTransport(command.value_file);
                transport_state.step = Transport_Stopped;
                return { .value_b = success };
            } break;
        }
        return { .value_b = true };
    }

    void push_new_dsp_state(Channel_DSP_State new_dsp_state)
    {
        dsp_callback.push_new_dsp_state(new_dsp_state);
    }

    Transport_State transport_state;
    
    juce::AudioDeviceManager audioDeviceManager;
    juce::AudioFormatManager formatManager;
    juce::TimeSliceThread readAheadThread  { "audio file preview" };
    
    //juce::URL currentAudioFile;
    juce::AudioSourcePlayer audioSourcePlayer;
    juce::AudioTransportSource transportSource;
    std::unique_ptr<juce::AudioFormatReaderSource> currentAudioFileSource;
    Channel_DSP_Callback dsp_callback;

    std::vector<juce::File> file_list;
};

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

    void setFile (const juce::File& file)
    {
        if (auto inputSource = makeInputSource (file))
        {
            thumbnail.setSource (inputSource.release());

            juce::Range<double> newRange (0.0, thumbnail.getTotalLength());
            scrollbar.setRangeLimits (newRange);
            setRange (newRange);

            startTimerHz (40);
        }
    }

    juce::File getLastDroppedFile() const noexcept { return lastFileDropped; }

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
        lastFileDropped = juce::File (files[0]);
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
    juce::File lastFileDropped;

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

class AudioFileList : 
    public juce::Component,
    public juce::ListBoxModel,
    public juce::DragAndDropTarget
{
public:
    AudioFileList(FilePlayer &player, juce::Component *dropSource) : 
        player(player), 
        dropSource(dropSource)
    {
        fileListComp.setColour (juce::ListBox::outlineColourId, juce::Colours::grey);      // [2]
        fileListComp.setOutlineThickness (2);
        addAndMakeVisible(fileListComp);
    }

    void resized()
    {
        fileListComp.setBounds(getLocalBounds());
    }

    int getNumRows() override 
    {
        return (int)player.file_list.size();
    }

    void paintListBoxItem (int rowNumber,
                           juce::Graphics& g,
                           int width, int height,
                           bool rowIsSelected) override
    {
        if (rowNumber < player.file_list.size())
        {
            g.setColour(juce::Colours::white);
            auto bounds = juce::Rectangle { 0, 0, width, height };
            g.drawText(player.file_list[rowNumber].getFileNameWithoutExtension(), bounds.reduced(2), juce::Justification::centredLeft);
            if (rowIsSelected)
            {
                g.drawRect(bounds);
            }
        }
    }

    bool isInterestedInDragSource (const SourceDetails& dragSourceDetails) override 
    {
        jassert(dragSourceDetails.sourceComponent == dropSource);
        return true;
    }

    
    void itemDropped (const SourceDetails& dragSourceDetails) override
    {
        jassert(dragSourceDetails.sourceComponent == dropSource);
        juce::File file = ((juce::FileTreeComponent*)dropSource)->getSelectedFile();
        if (auto * reader = player.formatManager.createReaderFor(file)) //expensive
        {
            //DBG(file.getFullPathName());
            player.file_list.emplace_back(std::move(file));
            fileListComp.updateContent();
            delete reader;
        }
    }

    
    void listBoxItemClicked (int row, const juce::MouseEvent&) override
    {
    }

    void listBoxItemDoubleClicked (int row, const juce::MouseEvent &) override
    {
        jassert(row < player.file_list.size());
        auto file = player.file_list[row];
        auto ret = player.post_command( { .type = Audio_Command_Load, .value_file = file });
        jassert(ret.value_b); //file still exists on drive ?
        player.post_command( { .type = Audio_Command_Play });
    }
    
    void deleteKeyPressed (int lastRowSelected) override
    {
        jassert(lastRowSelected >= 0 && lastRowSelected < player.file_list.size());
        player.file_list.erase(player.file_list.begin() + lastRowSelected);
        fileListComp.deselectAllRows();
        fileListComp.updateContent();
    }

    bool keyPressed (const juce::KeyPress &key) override
    {
        if (key == key.escapeKey)
        {
            fileListComp.deselectAllRows();
            return true;
        }
        else
        {
            return false;
        }
    }

private:
    FilePlayer &player;
    juce::ListBox fileListComp = { {}, this};

    juce::Component *dropSource;
};

class FileSelector_Panel : 
    public juce::Component,
    private juce::FileBrowserListener,
    public juce::DragAndDropContainer
{
public:
    
    FileSelector_Panel(FilePlayer &player, 
                       std::function < void() > onClickNext) 
    : fileList(player, &fileTreeComp), 
      player(player)
    {
        {
            fileExplorerThread.startThread (3);
            addAndMakeVisible (fileTreeComp);

            directoryList.setDirectory (juce::File::getSpecialLocation (juce::File::userHomeDirectory), true, true);
            directoryList.setIgnoresHiddenFiles(true);

            fileTreeComp.setTitle ("Files");
            fileTreeComp.setColour (juce::FileTreeComponent::backgroundColourId, juce::Colours::lightgrey.withAlpha (0.6f));
            fileTreeComp.setDragAndDropDescription("drag");
            fileTreeComp.addListener (this);
        }

        {
            addAndMakeVisible(fileList);
        }

        {
            nextButton.onClick = [onClickNext = std::move(onClickNext)]
            {
                onClickNext();
            };
            nextButton.setButtonText("Next");
            addAndMakeVisible(nextButton);
        }
    }

    ~FileSelector_Panel()
    {
        fileTreeComp.removeListener (this);
    }

    void resized() override 
    {
        auto r = getLocalBounds().reduced (4);
        auto bottom_bounds = r.removeFromBottom(50);
        auto left_bounds = r.getProportion<float>( { .0f, .0f, 0.5f, 1.0f });
        auto right_bounds = r.getProportion<float>( { 0.5f, .0f, 0.5f, 1.0f });

        fileTreeComp.setBounds (left_bounds);
        fileList.setBounds (right_bounds);
        
        auto button_bounds = bottom_bounds.withSizeKeepingCentre(100, 50);
        nextButton.setBounds(button_bounds);
    }
private:
    AudioFileList fileList;
    
    juce::TimeSliceThread fileExplorerThread  { "File Explorer thread" };
    juce::DirectoryContentsList directoryList {nullptr, fileExplorerThread};
    juce::FileTreeComponent fileTreeComp { directoryList };
    juce::TextButton nextButton;

    FilePlayer &player;
    
    void selectionChanged() override
    {
        auto ret = player.post_command( { .type = Audio_Command_Load, .value_file = fileTreeComp.getSelectedFile() });
        if (ret.value_b)
            player.post_command({ .type = Audio_Command_Play });
    }

    void fileClicked (const juce::File&, const juce::MouseEvent&) override          {}
    void fileDoubleClicked (const juce::File&) override                       {}
    void browserRootChanged (const juce::File&) override                      {}
};

//==============================================================================
class Main_Panel  : 
    public juce::Component,
    private juce::FileBrowserListener,
    private juce::ChangeListener
{
public:
    Main_Panel(FilePlayer &player) :
        player(player)
    {
        

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
            fileExplorerThread.startThread (3);
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
            thumbnail = std::make_unique < DemoThumbnailComp > (player.formatManager, player.transportSource, zoomSlider);
            addAndMakeVisible (thumbnail.get());
            thumbnail->addChangeListener (this);
        }
        { 
            addAndMakeVisible (startStopButton);
            startStopButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff79ed7f));
            startStopButton.setColour (juce::TextButton::textColourOffId, juce::Colours::black);
            startStopButton.onClick = [&player] { 
                if (player.transport_state.step == Transport_Playing)
                    player.post_command( { .type = Audio_Command_Stop });
                else 
                    player.post_command( { .type = Audio_Command_Play });
            };
        }


        setOpaque (true);
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
    FilePlayer &player;
    
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
    void showAudioResource (juce::File resource)
    {
        auto ret = player.post_command( { .type = Audio_Command_Load, .value_file = resource });
        if (ret.value_b)
        {
            zoomSlider.setValue (0, juce::dontSendNotification);
            thumbnail->setFile (resource);
        }
    }

    void updateFollowTransportState()
    {
        thumbnail->setFollowsTransport (followTransportButton.getToggleState());
    }
    
    void selectionChanged() override
    {
        showAudioResource (fileTreeComp.getSelectedFile());
    }

    void fileClicked (const juce::File&, const juce::MouseEvent&) override          {}
    void fileDoubleClicked (const juce::File&) override                       {}
    void browserRootChanged (const juce::File&) override                      {}

    void changeListenerCallback (juce::ChangeBroadcaster* source) override
    {
        //file drag and drop
        if (source == thumbnail.get())
            showAudioResource (thumbnail->getLastDroppedFile());
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Main_Panel)
};


class Empty : public juce::Component {
public :
    void paint(juce::Graphics &g)
    {
        g.setColour(juce::Colours::red);
        g.fillAll();
    }
};

class Main_Component : public juce::Component
{
    public :
    
    Main_Component()
    {
#if 0
        panel = std::make_unique < Main_Panel > (
            player
        );
#endif 
        toFileSelector();

#if 0
        addAndMakeVisible (sidePanel);
        sidePanel.setContent(&empty, false);
        addAndMakeVisible(burger);

        burger.onClick = [&] {
            sidePanel.showOrHide(true);
        };
#endif
        setSize (500, 300);
    }

    void toFileSelector()
    {
        jassert(state == nullptr);
        panel = std::make_unique < FileSelector_Panel > (player, [&] { toGame(); } );
        addAndMakeVisible(*panel);
    }

    void toSettings()
    {

    }
    
    void toGame()
    {        
        FrequencyGame_Settings settings = {
            .eq_gain = 4.0f,
            .eq_quality = 0.7f,
            .initial_correct_answer_window = 0.15f,
            .next_question_timeout_ms = 1000
        };
        state = frequency_game_state_init(settings, player.file_list);
        if(state == nullptr)
            return;
        removeChildComponent(panel.get());
        frequency_game_add_audio_observer(
            state.get(), 
            [this] (auto &&effect) { 
                player.push_new_dsp_state(effect.dsp_state);
        });
        frequency_game_add_player_observer(
            state.get(), 
            [this] (auto &&effect){ 
                for(const auto& command : effect.commands)
                    player.post_command(command);
        });
        frequency_game_post_event(state.get(), Event { .type = Event_Init });

        auto game_ui = std::make_unique < FrequencyGame_UI > (state.get());
        frequency_game_add_ui_observer(
            state.get(), 
            [ui = game_ui.get()] (Effect_UI &effect){ 
                frequency_game_ui_update(*ui, effect); 
        });
        frequency_game_post_event(state.get(), Event { .type = Event_Create_UI, .value_ptr = game_ui.get() });
        panel = std::move(game_ui);
        addAndMakeVisible(*panel);
        resized();
    }

    ~Main_Component()
    {
    }

    void resized() override 
    {
        auto r = getLocalBounds();
#if 0
        auto headerBounds = r.withHeight(50);
        {
            auto burgerBounds = headerBounds.withWidth(50);
            burger.setBounds(burgerBounds);
        }
#endif
        auto panelBounds = r.withTrimmedTop(50);
        panel->setBounds(panelBounds);
    }

    private :
    FilePlayer player;
    std::unique_ptr<FrequencyGame_State> state;
    std::unique_ptr<juce::Component> panel;

    //FrequencyGame_Settings settings;
    
#if 0
    juce::TextButton burger { "menu" };
    Empty empty;
    juce::SidePanel sidePanel { "Menu", 150, true };
#endif
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Main_Component)
};