
#pragma once
#include "../shared/pch.h"
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


//------------------------------------------------------------------------
struct Channel_DSP_Callback : public juce::AudioSource
{
    Channel_DSP_Callback(juce::AudioSource* inputSource) : input_source(inputSource)
    {
    }
    
    virtual ~Channel_DSP_Callback() override = default;

    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        assert(bufferToFill.buffer != nullptr);
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
            assert(new_coefficients);
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


//------------------------------------------------------------------------
struct FilePlayer {
    FilePlayer(juce::AudioFormatManager &formatManager)
    :   formatManager(formatManager),
        dsp_callback(&transportSource)
    {
        // audio setup
        readAheadThread.startThread ();

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

    juce::AudioFormatManager &formatManager;
    Transport_State transport_state;
    
    juce::AudioDeviceManager audioDeviceManager;
    juce::TimeSliceThread readAheadThread  { "audio file preview" };
    
    //juce::URL currentAudioFile;
    juce::AudioSourcePlayer audioSourcePlayer;
    juce::AudioTransportSource transportSource;
    std::unique_ptr<juce::AudioFormatReaderSource> currentAudioFileSource;
    Channel_DSP_Callback dsp_callback;
};





//==============================================================================
class DemoThumbnailComp  :
    public juce::Component,
    public juce::ChangeListener,
    public juce::ChangeBroadcaster,
    private juce::ScrollBar::Listener,
    private juce::Timer
{
public:
    DemoThumbnailComp (juce::AudioFormatManager& formatManager,
                       juce::AudioTransportSource& source)
    : transportSource (source),
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

    void removeFile()
    {
        thumbnail.setSource(nullptr);
    }

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
            {
                //Set value directly
                //zoomSlider.setValue (zoomSlider.getValue() - wheel.deltaY);
            }

            repaint();
        }
    }

private:
    juce::AudioTransportSource& transportSource;
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




//------------------------------------------------------------------------
class AudioFileList : 
    public juce::Component,
    public juce::ListBoxModel,
    public juce::DragAndDropTarget,
    public juce::FileDragAndDropTarget
{
public:
    AudioFileList(FilePlayer &player, 
                  std::vector<Audio_File> &files,
                  juce::Component *dropSource) : 
        player(player), 
        files(files),
        dropSource(dropSource)
    {
        fileListComp.setMultipleSelectionEnabled(true);
        fileListComp.setColour (juce::ListBox::outlineColourId, juce::Colours::grey);      // [2]
        fileListComp.setOutlineThickness (2);
        addAndMakeVisible(fileListComp);
    }

    virtual ~AudioFileList() override = default;

    void resized() override
    {
        fileListComp.setBounds(getLocalBounds());
    }

    void paintOverChildren(juce::Graphics& g) override
    {
        if (files.empty())
        {
            auto r = getLocalBounds();
            g.setColour(juce::Colours::white);
            g.drawText("Drag and drop audio files here", r.toFloat(), juce::Justification::centred);
        }
    }

    int getNumRows() override { return (int)files.size(); }

    void paintListBoxItem (int rowNumber,
                           juce::Graphics& g,
                           int width, int height,
                           bool rowIsSelected) override
    {
        if (rowNumber < files.size())
        {
            g.setColour(juce::Colours::white);
            auto bounds = juce::Rectangle { 0, 0, width, height };
            g.drawText(files[rowNumber].title, bounds.reduced(2), juce::Justification::centredLeft);
            if (rowIsSelected)
            {
                g.drawRect(bounds);
            }
        }
    }

    bool isInterestedInFileDrag (const juce::StringArray&) override { return true; }

    bool isInterestedInDragSource (const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) override 
    {
        assert(dragSourceDetails.sourceComponent == dropSource);
        return true;
    }

    void insertFile(juce::File file)
    {
        //can't have the same file twice
        if (auto result = std::find_if(files.begin(), files.end(), [&] (const Audio_File &in) { return in.file == file; }); result == files.end())
        {
            if (auto * reader = player.formatManager.createReaderFor(file)) //expensive
            {
                /*
                for (const auto &key : reader->metadataValues.getAllKeys())
                {
                DBG(key);
                }
                */
                //DBG(file.getFullPathName());
                Audio_File new_audio_file = {
                    .file = file,
                    .title = file.getFileNameWithoutExtension(),
                    .loop_bounds = { 0, reader->lengthInSamples }
                };
                files.emplace_back(std::move(new_audio_file));
                fileListComp.updateContent();
                delete reader;
            }
        }
    }
    
    void filesDropped (const juce::StringArray& dropped_files, int, int) override
    {
        for (const auto &file : dropped_files)
        {
            insertFile(juce::File(file));
        }
    }
    
    void itemDropped (const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) override
    {
        assert(dragSourceDetails.sourceComponent == dropSource);
        juce::FileTreeComponent *tree = (juce::FileTreeComponent*)dropSource;
        auto dropped_file_count = tree->getNumSelectedFiles();
        for (auto i = 0; i < dropped_file_count; i++)
        {
            juce::File file = tree->getSelectedFile(i);
            insertFile(file);
        }
    }

    
    void listBoxItemClicked (int, const juce::MouseEvent&) override {}

    void listBoxItemDoubleClicked (int, const juce::MouseEvent &) override {}
    
    void deleteKeyPressed (int lastRowSelected) override
    {
        auto num_selected = fileListComp.getNumSelectedRows();
        if ( num_selected > 1)
        {
            auto selected_rows = fileListComp.getSelectedRows();
            fileListComp.deselectAllRows();
            for (int i = getNumRows(); --i >= 0;)
            {   
                if(selected_rows.contains(i))
                    files.erase(files.begin() + i);
            }
            fileListComp.updateContent();
        }
        else if (num_selected == 1)
        {

            files.erase(files.begin() + lastRowSelected);
            fileListComp.updateContent();
        }
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

    
    void selectedRowsChanged (int lastRowSelected) override
    {
        if (lastRowSelected != -1)
        {
            assert(lastRowSelected < files.size());
            const auto &file = files[lastRowSelected].file;
            auto ret = player.post_command( { .type = Audio_Command_Load, .value_file = file });
            assert(ret.value_b); //file still exists on drive ?
            player.post_command( { .type = Audio_Command_Play });

            file_changed_callback(file);
        }
        else
        {
            player.post_command( { .type = Audio_Command_Stop });
            file_changed_callback({});
        }
    }
    std::function < void(juce::File) > file_changed_callback;

private:
    FilePlayer &player;
    std::vector<Audio_File> &files;
    juce::ListBox fileListComp = { {}, this};

    juce::Component *dropSource;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioFileList)
};





//------------------------------------------------------------------------
class FileSelector_Panel : 
    public juce::Component,
    private juce::FileBrowserListener,
    public juce::DragAndDropContainer
{
public:
    
    FileSelector_Panel(FilePlayer &player, 
                       std::vector<Audio_File> &files,
                       std::function<void()> onClickBack) 
    : fileList(player, files, &explorer), 
      player(player)
    {
        {
            header.onBackClicked = [click = std::move(onClickBack)] {
                click();
            };
            game_ui_header_update(&header, "Audio Files", -1);
            addAndMakeVisible(header);
        }

        {
            collapseExplorer.setToggleState(false, juce::dontSendNotification);
            collapseExplorer.onStateChange = [&] {
                auto is_on = collapseExplorer.getToggleState();
                explorer.setVisible(is_on);
                resized();
            };
            addAndMakeVisible(collapseExplorer);
        }
        {
            fileExplorerThread.startThread ();

            directoryList.setDirectory (juce::File::getSpecialLocation (juce::File::userDesktopDirectory), true, true);
            directoryList.setIgnoresHiddenFiles(true);

            explorer.setTitle ("Files");
            explorer.setColour (juce::FileTreeComponent::backgroundColourId, juce::Colours::lightgrey.withAlpha (0.6f));
            explorer.setDragAndDropDescription("drag");
            explorer.addListener (this);
            explorer.setMultiSelectEnabled(true);
            addChildComponent(explorer);
        }

        {
            fileList.file_changed_callback = [&] (juce::File new_file)
            {
                if (new_file != juce::File{})
                    thumbnail.setFile(new_file);
                else
                    thumbnail.removeFile();
                //frequency_bounds_slider.setMinAndMaxValues();
                //nochekin;
            };
            addAndMakeVisible(fileList);
        }

        {
            addAndMakeVisible(thumbnail);
        }
        {
            addAndMakeVisible(frequency_bounds_slider);
        }
    }

    ~FileSelector_Panel() override
    {
        explorer.removeListener (this);
    }

    void resized() override 
    {
        auto r = getLocalBounds().reduced (4);
        auto header_bounds = r.removeFromTop(game_ui_header_height);
        auto bottom_bounds = r.removeFromBottom(100);
        
        header.setBounds(header_bounds);

        auto sub_header_bounds = r.removeFromTop(40);
        auto left_bounds = r.getProportion<float>( { .0f, .0f, 0.5f, 1.0f }).withTrimmedRight(5);
        auto right_bounds = r.getProportion<float>( { 0.5f, .0f, 0.5f, 1.0f }).withTrimmedLeft(5);

        auto collapse_bounds = sub_header_bounds.removeFromLeft(100);
        collapseExplorer.setBounds(collapse_bounds);

        bool explorer_is_on = collapseExplorer.getToggleState();
        if (explorer_is_on)
        {
            explorer.setBounds (left_bounds);
            fileList.setBounds (right_bounds);
        }
        else
        {
            fileList.setBounds (r);
        }
        
        frequency_bounds_slider.setBounds(bottom_bounds.removeFromBottom(20).reduced(50, 0));
        thumbnail.setBounds(bottom_bounds);
    }
private:
    FilePlayer &player;
    GameUI_Header header;
    AudioFileList fileList;
    
    juce::ToggleButton collapseExplorer { "Show file explorer" };
    juce::TimeSliceThread fileExplorerThread  { "File Explorer thread" };
    juce::DirectoryContentsList directoryList {nullptr, fileExplorerThread};
    juce::FileTreeComponent explorer { directoryList };

    DemoThumbnailComp thumbnail { player.formatManager, player.transportSource };
    Frequency_Bounds_Widget frequency_bounds_slider;

    void selectionChanged() override
    {
        thumbnail.setFile(explorer.getSelectedFile());
        auto ret = player.post_command( { .type = Audio_Command_Load, .value_file = explorer.getSelectedFile() });
        if (ret.value_b)
            player.post_command({ .type = Audio_Command_Play });
    }

    void fileClicked (const juce::File&, const juce::MouseEvent&) override          {}
    void fileDoubleClicked (const juce::File&) override                       {}
    void browserRootChanged (const juce::File&) override                      {}

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FileSelector_Panel)
};


//------------------------------------------------------------------------
class Config_List : 
    public juce::Component,
    public juce::ListBoxModel
{
   
public:
    Config_List(std::vector<FrequencyGame_Config> &config,
                std::function<void(int)> onClick) : 
        config(config),
        onClick(std::move(onClick))
    {
        list_comp.setMultipleSelectionEnabled(false);
        list_comp.setColour (juce::ListBox::outlineColourId, juce::Colours::grey);      // [2]
        list_comp.setOutlineThickness (2);
        addAndMakeVisible(list_comp);
        list_comp.updateContent();
    }

    virtual ~Config_List() override = default;

    void resized() override
    {
        list_comp.setBounds(getLocalBounds());
    }

    void paintOverChildren(juce::Graphics& g) override
    {
        if (config.empty())
        {
            auto r = getLocalBounds();
            g.setColour(juce::Colours::white);
            g.drawText("Create game config", r.toFloat(), juce::Justification::centred);
        }
    }

    int getNumRows() override { return (int)config.size() + 1; }

    void paintListBoxItem (int row,
                           juce::Graphics& g,
                           int width, int height,
                           bool rowIsSelected) override
    {
        if (rowIsSelected && row < config.size())
        {
            g.setColour(juce::Colours::white);
            auto bounds = juce::Rectangle { 0, 0, width, height };
            g.drawRect(bounds);
        }
    }

    juce::Component *refreshComponentForRow (int rowNumber,
                                             bool,
                                             Component *existingComponentToUpdate) override
    {
        //assert (existingComponentToUpdate == nullptr || dynamic_cast<EditableTextCustomComponent*> (existingComponentToUpdate) != nullptr);
        //unused row
        if (rowNumber > config.size())
        {
            if (existingComponentToUpdate != nullptr)
                delete existingComponentToUpdate;
            return nullptr;
        }
        //"Insert config" row
        else if (rowNumber == config.size()) 
        {
            EditableTextCustomComponent *label = existingComponentToUpdate != nullptr ?
                dynamic_cast<EditableTextCustomComponent*>(existingComponentToUpdate) :
                new EditableTextCustomComponent(*this);
            label->setRow (rowNumber);
            return label;
        }
        else
        {
            EditableTextCustomComponent *label = existingComponentToUpdate != nullptr ?
                dynamic_cast<EditableTextCustomComponent*>(existingComponentToUpdate) :
                new EditableTextCustomComponent(*this);
            label->setRow (rowNumber);
            return label;
        } 
    }
    
    void listBoxItemClicked (int, const juce::MouseEvent&) override 
    {
    }

    void listBoxItemDoubleClicked (int, const juce::MouseEvent &) override {}
    
    void deleteKeyPressed (int) override
    {
        auto selected_row = list_comp.getSelectedRow();
        if(selected_row == -1) 
            return;
        config.erase(config.begin() + selected_row);
        auto row_to_select = selected_row == 0 ? 0 : selected_row - 1;
        list_comp.selectRow(row_to_select);
        list_comp.updateContent();
    }

    bool keyPressed (const juce::KeyPress &key) override
    {
        if (key != key.escapeKey)
            return false;
        list_comp.deselectAllRows();
        return true;
    }

    void selectedRowsChanged (int lastRowSelected) override
    {   
        if(lastRowSelected != -1 && lastRowSelected != config.size())
            onClick(lastRowSelected);
    }

    void selectRow(int new_row)
    {
        list_comp.selectRow(new_row);
    }

private:
    std::vector<FrequencyGame_Config> &config;
    juce::ListBox list_comp = { {}, this };
    std::function < void(int) > onClick;
    
    
    class EditableTextCustomComponent  : public juce::Label
    {
    public:
        EditableTextCustomComponent (Config_List& list)  : owner (list)
        {
            // double click to edit the label text; single click handled below
            setEditable (false, true, true);
        }

        void mouseDown (const juce::MouseEvent& event) override
        {   
            // single click on the label should simply select the row
            owner.list_comp.selectRowsBasedOnModifierKeys (row, event.mods, false);

            juce::Label::mouseDown (event);
        }

        void editorShown (juce::TextEditor * new_editor) override
        {
            new_editor->setText("", juce::dontSendNotification);
        }

        void textWasEdited() override
        {
            bool restore_text = getText() == "";

            if (row < owner.config.size())
            {
                if (restore_text)
                {
                    setText(owner.config[row].title, juce::dontSendNotification);
                    return;
                }
                owner.config[row].title = getText();
            }
            else //
            {
                
                if (restore_text)
                {
                    setText("Insert new config", juce::dontSendNotification);
                    return;
                }
                owner.config.push_back(frequency_game_config_default(getText()));
                owner.onClick(row);
                owner.list_comp.updateContent();
            }
        }

        // Our demo code will call this when we may need to update our contents
        void setRow (const int newRow)
        {
            row = newRow;
            if (row < owner.config.size())
            {
                setJustificationType(juce::Justification::left);
                setText (owner.config[newRow].title, juce::dontSendNotification);
                setColour(juce::Label::textColourId, juce::Colours::white);
            }
            else
            {
                setJustificationType(juce::Justification::centred);
                setText ("Insert new config", juce::dontSendNotification);
                setColour(juce::Label::textColourId, juce::Colours::lightgrey);
            }
        }

        void paint (juce::Graphics& g) override
        {
            auto& lf = getLookAndFeel();
            if (! dynamic_cast<juce::LookAndFeel_V4*>(&lf))
                lf.setColour (textColourId, juce::Colours::black);
    
            juce::Label::paint (g);
        }
    
    private:
        Config_List& owner;
        int row;
        juce::Colour textColour;
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Config_List)
};


//------------------------------------------------------------------------
struct Config_Panel : public juce::Component
{
    using slider_and_label_t = std::vector<std::tuple < juce::Slider&, juce::Label&, juce::Range<double>, double> >;
    using slider_and_toggle_t = std::vector<std::tuple < juce::Slider&, juce::ToggleButton&, juce::Range<double>, double> >;
    
    class Scroller : public juce::Component
    {
    public:
        Scroller(std::function<void(juce::Rectangle<int>)> onResize) 
        : onResizeCallback(std::move(onResize))
        {}

        void resized() override
        {
            onResizeCallback(getLocalBounds());
        }
    private:
        std::function < void(juce::Rectangle<int>) > onResizeCallback;
    };

    Config_Panel(std::vector<FrequencyGame_Config> &configs,
                   int &current_config_idx,
                   std::function < void() > onClickBack,
                   std::function < void() > onClickNext) :
        configs(configs), 
        current_config_idx(current_config_idx),
        config_list_comp { configs, [&] (int idx) { selectConfig(idx); } }
    {
        {
            header.onBackClicked = [click = std::move(onClickBack)] {
                click();
            };
            game_ui_header_update(&header, "Config", -1);
            addAndMakeVisible(header);
        }
        
        addAndMakeVisible(config_list_comp);
        
        FrequencyGame_Config initial_config = configs[current_config_idx];

        eq_gain.setTextValueSuffix (" dB");
        eq_gain.onValueChange = [&] {
            configs[current_config_idx].eq_gain = juce::Decibels::decibelsToGain((float) eq_gain.getValue());
        };

        eq_quality.onValueChange = [&] {
            configs[current_config_idx].eq_quality = (float) eq_quality.getValue();
        };

        initial_correct_answer_window.onValueChange = [&] {
            configs[current_config_idx].initial_correct_answer_window = (float) initial_correct_answer_window.getValue();
        };

        
        question_timeout_ms.setTextValueSuffix (" ms");
        question_timeout_ms.onValueChange = [&] {
            configs[current_config_idx].question_timeout_ms = (int) question_timeout_ms.getValue();
        };
        question_timeout_ms.setNumDecimalPlacesToDisplay(0);
        
        question_timeout_enabled.onClick = [&] {
            bool new_toggle_state = question_timeout_enabled.getToggleState();
            configs[current_config_idx].question_timeout_enabled = new_toggle_state;
            question_timeout_ms.setEnabled(new_toggle_state);
        };

        result_timeout_ms.setTextValueSuffix (" ms");
        result_timeout_ms.onValueChange = [&] {
            configs[current_config_idx].result_timeout_ms = (int) result_timeout_ms.getValue();
        };
        result_timeout_ms.setNumDecimalPlacesToDisplay(0);
        
        result_timeout_enabled.onClick = [&] {
            bool new_toggle_state = result_timeout_enabled.getToggleState();
            configs[current_config_idx].result_timeout_enabled = new_toggle_state;
            result_timeout_ms.setEnabled(new_toggle_state);
        };

        
        slider_and_label_t slider_and_label = { 
            { eq_gain, eq_gain_label, { -15.0, 15.0 }, 3.0}, 
            { eq_quality, eq_quality_label, { 0.5, 4 }, 0.1 }, 
            { initial_correct_answer_window, initial_correct_answer_window_label, { 0.01, 0.4 }, 0.01 }
        };

        for (auto &[slider, label, range, interval] : slider_and_label)
        {
            slider.setScrollWheelEnabled(false);
            slider.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 50, 20);
            slider.setRange(range, interval);

            label.setBorderSize( juce::BorderSize<int>{ 0 });
            label.setJustificationType(juce::Justification::left);
            
            scroller.addAndMakeVisible(slider);
            scroller.addAndMakeVisible(label);
        }

        slider_and_toggle_t slider_and_toggle = {
            { question_timeout_ms , result_timeout_enabled, { 1000, 4000 }, 500 },
            { result_timeout_ms , question_timeout_enabled, { 1000, 4000 }, 500 }
        };

        
        for (auto &[slider, toggle, range, interval] : slider_and_toggle)
        {
            slider.setScrollWheelEnabled(false);
            slider.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 50, 20);
            slider.setRange(range, interval);

            //toggle.setBorderSize( juce::BorderSize<int>{ 0 });
            //toggle.setJustificationType(juce::Justification::left);
            
            scroller.addAndMakeVisible(slider);
            scroller.addAndMakeVisible(toggle);
        }

        scroller.setSize(0, 5 * 60);
        viewport.setScrollBarsShown(true, false);
        viewport.setViewedComponent(&scroller, false);
        addAndMakeVisible(viewport);

        {
            nextButton.onClick = [onClickNext = std::move(onClickNext)] {
                onClickNext();
            };
            addAndMakeVisible(nextButton);
        }

        config_list_comp.selectRow(current_config_idx);
        //selectConfig(current_config_idx);
    }

    void resized()
    {
        auto r = getLocalBounds().reduced (4);

        auto header_bounds = r.removeFromTop(game_ui_header_height);
        header.setBounds(header_bounds);

        auto bottom_bounds = r.removeFromBottom(50);
        auto left_bounds = r.getProportion<float>( { .0f, .0f, 0.5f, 1.0f }).withTrimmedRight(5);
        auto right_bounds = r.getProportion<float>( { 0.5f, .0f, 0.5f, 1.0f }).withTrimmedLeft(5);

        config_list_comp.setBounds(left_bounds);
        scroller.setSize(right_bounds.getWidth(), scroller.getHeight());
        viewport.setBounds(right_bounds);
        auto button_bounds = bottom_bounds.withSizeKeepingCentre(100, 50);
        nextButton.setBounds(button_bounds);
    }

    void selectConfig(int new_config_idx)
    {
        assert(new_config_idx < configs.size());
        current_config_idx = new_config_idx;
        auto &current_config = configs[current_config_idx];
        float gain_db = juce::Decibels::gainToDecibels(current_config.eq_gain);
        eq_gain.setValue(gain_db);
        eq_quality.setValue(current_config.eq_quality);
        initial_correct_answer_window.setValue(current_config.initial_correct_answer_window);
        
        question_timeout_ms.setValue((double)current_config.question_timeout_ms);
        question_timeout_ms.setEnabled(current_config.question_timeout_enabled);
        question_timeout_enabled.setToggleState(current_config.question_timeout_enabled, juce::dontSendNotification);
        
        result_timeout_ms.setValue((double)current_config.result_timeout_ms);
        result_timeout_ms.setEnabled(current_config.result_timeout_enabled);
        result_timeout_enabled.setToggleState(current_config.result_timeout_enabled, juce::dontSendNotification);
    }
    
    void onResizeScroller(juce::Rectangle<int> scroller_bounds)
    {
        auto height = scroller_bounds.getHeight();
        auto param_height = height / 5;

        auto same_bounds = [&] {
            return scroller_bounds.removeFromTop(param_height / 2);
        };

        eq_gain_label.setBounds(same_bounds());
        eq_gain.setBounds(same_bounds());
        
        eq_quality_label.setBounds(same_bounds());
        eq_quality.setBounds(same_bounds());
        
        initial_correct_answer_window_label.setBounds(same_bounds());
        initial_correct_answer_window.setBounds(same_bounds());

        question_timeout_enabled.setBounds(same_bounds());
        question_timeout_ms.setBounds(same_bounds());

        result_timeout_enabled.setBounds(same_bounds());
        result_timeout_ms.setBounds(same_bounds());
    }

    std::vector<FrequencyGame_Config> &configs;
    int &current_config_idx;
    Config_List config_list_comp;
    
    GameUI_Header header;

    juce::Slider eq_gain;
    juce::Label eq_gain_label { {}, "Gain"};
    
    juce::Slider eq_quality;
    juce::Label eq_quality_label { {}, "Q" };
    
    juce::Slider initial_correct_answer_window;
    juce::Label initial_correct_answer_window_label { {}, "Initial answer window" };
    
    juce::ToggleButton question_timeout_enabled { "Question timeout" };
    juce::Slider question_timeout_ms;

    juce::ToggleButton result_timeout_enabled { "Post answer timeout" };
    juce::Slider result_timeout_ms;

    juce::Viewport viewport;
    Scroller scroller { [this] (auto bounds) { onResizeScroller(bounds); } };
   
    juce::TextButton nextButton { "Next" };
   
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Config_Panel)
};




//==============================================================================
class Demo_Panel  : 
    public juce::Component
{
public:
    Demo_Panel(FilePlayer &player) :
        player(player)
    {
#if 0
            followTransportButton.onClick = [this] { thumbnail->setFollowsTransport (true); };  
            zoomSlider.onValueChange = [this] { thumbnail->setZoomFactor (zoomSlider.getValue()); };

            startStopButton.onClick = [&player] { 
                if (player.transport_state.step == Transport_Playing)
                    player.post_command( { .type = Audio_Command_Stop });
                else 
                    player.post_command( { .type = Audio_Command_Play });
            };
            thumbnail = std::make_unique < DemoThumbnailComp > (player.formatManager, player.transportSource, zoomSlider);
#endif   
    }

private:
    FilePlayer &player;
    std::unique_ptr<DemoThumbnailComp> thumbnail;

    void showAudioResource (juce::File resource)
    {
        auto ret = player.post_command( { .type = Audio_Command_Load, .value_file = resource });
        if (ret.value_b)
        {
            //zoomSlider.setValue (0, juce::dontSendNotification);
            thumbnail->setFile (resource);
        }
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Demo_Panel)
};


class Empty : public juce::Component {
public :
    void paint(juce::Graphics &g)
    {
        g.setColour(juce::Colours::red);
        g.fillAll();
    }
};




//------------------------------------------------------------------------
class MainMenu_Panel : public juce::Component
{
public :
    MainMenu_Panel(std::function<void()> &&toGame,
             std::function<void()> &&toFileList,
             std::function<void()> &&toStats)
    {
        game_button.setSize(100, 40);
        game_button.setButtonText("Game");
        game_button.onClick = [click = std::move(toGame)] {
            click();
        };
        addAndMakeVisible(game_button);
        
        file_list_button.setSize(100, 40);
        file_list_button.setButtonText("Audio Files");
        file_list_button.setEnabled(true);
        file_list_button.onClick = [click = std::move(toFileList)] {
            click();
        };
        addAndMakeVisible(file_list_button);
        
        stats_button.setSize(100, 40);
        stats_button.setButtonText("Statistics");
        stats_button.setEnabled(false);
        stats_button.onClick = [click = std::move(toStats)] {
            click();
        };
        addAndMakeVisible(stats_button);
    }
    
    void paint(juce::Graphics &g) override
    {
        juce::ignoreUnused(g);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        game_button.setCentrePosition(bounds.getCentre() + juce::Point<int>(0, -50));
        file_list_button.setCentrePosition(bounds.getCentre());
        stats_button.setCentrePosition(bounds.getCentre() + juce::Point<int>(0, 50));
    }

    juce::TextButton game_button;
    juce::TextButton file_list_button;
    juce::TextButton stats_button;
};

static const juce::Identifier id_config_root = "configs";
static const juce::Identifier id_config = "config";
static const juce::Identifier id_config_title = "title";
static const juce::Identifier id_config_gain = "gain";
static const juce::Identifier id_config_quality = "q";
static const juce::Identifier id_config_window = "window";
static const juce::Identifier id_config_question_timeout_enabled = "question_timeout_enabled";
static const juce::Identifier id_config_question_timeout_ms = "question_timeout_ms";
static const juce::Identifier id_config_result_timeout_enabled = "result_timeout_enabled";
static const juce::Identifier id_config_result_timeout_ms = "result_timeout_ms";

static const juce::Identifier id_results_root = "results_history";
static const juce::Identifier id_result = "result";
static const juce::Identifier id_result_score = "score";
static const juce::Identifier id_result_timestamp = "timestamp";


class Main_Component : public juce::Component
{
    public :
    
    Main_Component(juce::AudioFormatManager &formatManager)
    :   player(formatManager)
    {
        juce::File app_data = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
        DBG(app_data.getFullPathName());
        auto store_directory = app_data.getChildFile("MixTrainer");

        //load audio file list
        [&] {
            auto file_RENAME = store_directory.getChildFile("audio_file_list.txt");
            if (!file_RENAME.existsAsFile())
            {
                file_RENAME.create();
                return;
            }
            auto stream = file_RENAME.createInputStream();
            if (!stream->openedOk())
            {
                DBG("couldn't open %appdata%/MixTrainer/audio_file_list.txt");
                return;
            }
            while (! stream->isExhausted()) // [3]
            {
                auto line = stream->readNextLine();
                auto audio_file_path = juce::File(line);
                if (!audio_file_path.existsAsFile())
                {
                    DBG(line << " does not exist");
                    continue;
                }
                auto * reader = formatManager.createReaderFor(audio_file_path);
                if (!reader)
                {
                    DBG("invalid audio file " << line);
                    continue;
                }
                
                Audio_File new_audio_file = {
                    .file = audio_file_path,
                    .title = audio_file_path.getFileNameWithoutExtension(),
                    .loop_bounds = { 0, reader->lengthInSamples }
                };
                files.emplace_back(std::move(new_audio_file));
                delete reader;
            }
        }();

        //load config list
        [&] {
            auto file_RENAME = store_directory.getChildFile("configs.xml");
            if (!file_RENAME.existsAsFile())
            {
                file_RENAME.create();
                return;
            }
            auto stream = file_RENAME.createInputStream();
            if (!stream->openedOk())
            {
                DBG("couldn't open %appdata%/MixTrainer/configs.xml");
                return;
            }
            juce::String xml_string = stream->readString();
            juce::ValueTree root_node = juce::ValueTree::fromXml(xml_string);
            if(root_node.getType() != id_config_root)
                return;
            for (int i = 0; i < root_node.getNumChildren(); i++)
            {
                juce::ValueTree node = root_node.getChild(i);
                if(node.getType() != id_config)
                    continue;
                FrequencyGame_Config config = {
                    .title = node.getProperty(id_config_title, ""),
                    .eq_gain = node.getProperty(id_config_gain, -1.0f),
                    .eq_quality = node.getProperty(id_config_quality, -1.0f),
                    .initial_correct_answer_window = node.getProperty(id_config_window, -1.0f),
                    .question_timeout_enabled = node.getProperty(id_config_question_timeout_enabled, false),
                    .question_timeout_ms = node.getProperty(id_config_question_timeout_ms, -1),
                    .result_timeout_enabled = node.getProperty(id_config_result_timeout_enabled, false),
                    .result_timeout_ms = node.getProperty(id_config_result_timeout_ms, -1),
                };
                game_configs.push_back(config);
            }
        }();
        
        if (game_configs.empty())
        {
            game_configs = { 
                frequency_game_config_default("Default")
            };
        }
        jassert(!game_configs.empty());

        
        //load previous results
        [&] {
            auto file_RENAME = store_directory.getChildFile("results.xml");
            if (!file_RENAME.existsAsFile())
            {
                file_RENAME.create();
                return;
            }
            auto stream = file_RENAME.createInputStream();
            if (!stream->openedOk())
            {
                DBG("couldn't open %appdata%/MixTrainer/results.xml");
                return;
            }
            juce::String xml_string = stream->readString();
            juce::ValueTree root_node = juce::ValueTree::fromXml(xml_string);
            if(root_node.getType() != id_results_root)
                return;
            for (int i = 0; i < root_node.getNumChildren(); i++)
            {
                juce::ValueTree node = root_node.getChild(i);
                if(node.getType() != id_result)
                    continue;
                FrequencyGame_Results result = {
                    .score = node.getProperty(id_result_score, "")
                };
                game_results_history.push_back(result);
            }
        }();

        if (game_configs.empty())
        {
            game_configs = { frequency_game_config_default("Default") };
        }

        toMainMenu();
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

    ~Main_Component()
    {
        juce::File app_data = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
        DBG(app_data.getFullPathName());
        assert(app_data.exists() && app_data.isDirectory());
        auto store_directory = app_data.getChildFile("MixTrainer");
        //save audio file list
        [&] {
            auto file_RENAME = store_directory.getChildFile("audio_file_list.txt");;
            auto stream = file_RENAME.createOutputStream();
            if (!stream->openedOk())
            {
                DBG("couldn't open %appdata%/MixTrainer/audio_file_list.txt");
                return;
            }
            stream->setPosition(0);
            stream->truncate();
            for (const Audio_File &audio_file : files)
            {
                *stream << audio_file.file.getFullPathName() << juce::newLine;
            }
        }();

        //save config list
        [&] {
            auto file_RENAME = store_directory.getChildFile("configs.xml");
            auto stream = file_RENAME.createOutputStream();
            if (!stream->openedOk())
            {
                DBG("couldn't open %appdata%/MixTrainer/configs.xml");
                return;
            }
            stream->setPosition(0);
            stream->truncate();
            juce::ValueTree root_node { id_config_root };
            for (const FrequencyGame_Config& config : game_configs)
            {
                juce::ValueTree node = { id_config, {
                    { id_config_title,  config.title },
                    { id_config_gain, config.eq_gain },
                    { id_config_quality, config.eq_quality },
                    { id_config_window, config.initial_correct_answer_window },
                    { id_config_question_timeout_enabled, config.question_timeout_enabled },
                    { id_config_question_timeout_ms, config.question_timeout_ms },
                    { id_config_result_timeout_enabled, config.result_timeout_enabled },
                    { id_config_result_timeout_ms, config.result_timeout_ms },
                }};
                root_node.addChild(node, -1, nullptr);
            }
            auto xml_string = root_node.toXmlString();
            *stream << xml_string;
        }();

        
        //save previous results
        [&] {
            auto file_RENAME = store_directory.getChildFile("results.xml");
            auto stream = file_RENAME.createOutputStream();
            if (!stream->openedOk())
            {
                DBG("couldn't open %appdata%/MixTrainer/results.xml");
                return;
            }
            stream->setPosition(0);
            stream->truncate();
            juce::ValueTree root_node { id_results_root };
            for (const FrequencyGame_Results& result : game_results_history)
            {
                juce::ValueTree node = { id_result, {
                    { id_result_score,  result.score }
                }};
                root_node.addChild(node, -1, nullptr);
            }
            auto xml_string = root_node.toXmlString();
            *stream << xml_string;
        }();
    }

    void toMainMenu()
    {
        game_state.reset();
        auto main_menu_panel = std::make_unique < MainMenu_Panel > (
            [this] { toGameConfig(); },
            [this] { toFileSelector(); },
            [this] {}
        );
        if (files.empty())
        {
            main_menu_panel->game_button.setEnabled(false);
            main_menu_panel->game_button.setTooltip("Please add audio files");
        }
        panel = std::move(main_menu_panel);
        addAndMakeVisible(*panel);
        resized();
    }

    void toFileSelector()
    {
        assert(game_state == nullptr);
        panel = std::make_unique < FileSelector_Panel > (player, files, [&] { toMainMenu(); } );
        addAndMakeVisible(*panel);
        resized();
    }

    void toGameConfig()
    {
        assert(game_state == nullptr);
        panel = std::make_unique < Config_Panel > (game_configs, current_config_idx, [&] { toMainMenu(); }, [&] { toGame(); });
        addAndMakeVisible(*panel);
        resized();
    }
    
    void toGame()
    {
        game_state = frequency_game_state_init(game_configs[current_config_idx], files);
        if(game_state == nullptr)
            return;
        removeChildComponent(panel.get());

        auto observer = [this] (const Effects &effects) { 
            if (effects.transition)
            {
                if (effects.transition->in_transition == GameStep_Begin)
                {
                    panel = std::make_unique < FrequencyGame_UI > (game_state.get(), game_io.get());
                    addAndMakeVisible(*panel);
                }
                auto *game_ui = dynamic_cast<FrequencyGame_UI*>(panel.get());
                if(game_ui)
                    frequency_game_ui_transitions(*game_ui, *effects.transition);
            }
            if (effects.dsp)
            {
                player.push_new_dsp_state(effects.dsp->dsp_state);
            }
            if (effects.player)
            {
                for (const auto& command : effects.player->commands)
                    player.post_command(command);
            }
            if (effects.results)
            {
                game_results_history.push_back(*effects.results);
            }
            if (effects.ui)
            {
                auto *game_ui = dynamic_cast<FrequencyGame_UI*>(panel.get());
                assert(game_ui);
                frequency_game_ui_update(*game_ui, *effects.ui);
            }
        };

        auto debug_observer = [this] (const Effects &effects) {
            juce::ignoreUnused(effects);
        };
        
        game_io = frequency_game_io_init();

        auto on_quit = [this] { 
            game_io->timer.stopTimer();
            player.post_command( { .type = Audio_Command_Stop });
            toMainMenu();
        };
        game_io->on_quit = std::move(on_quit);

        frequency_game_add_observer(game_io.get(), std::move(observer));
        frequency_game_add_observer(game_io.get(), std::move(debug_observer));

        frequency_game_post_event(game_state.get(), game_io.get(), Event { .type = Event_Init });
        frequency_game_post_event(game_state.get(), game_io.get(), Event { .type = Event_Create_UI });
        game_io->timer.callback = [state = game_state.get(), io = game_io.get()] (juce::int64 timestamp) {
            frequency_game_post_event(state, io, Event {.type = Event_Timer_Tick, .value_i64 = timestamp});
        };
        game_io->timer.startTimerHz(60);

        resized();
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
        //auto panelBounds = r.withTrimmedTop(50);
#endif
        auto panelBounds = r;
        panel->setBounds(panelBounds);
    }

    private :
    FilePlayer player;
    std::unique_ptr<FrequencyGame_IO> game_io;
    std::unique_ptr<FrequencyGame_State> game_state;
    std::unique_ptr<juce::Component> panel;
    

    std::vector<Audio_File> files;
    std::vector<FrequencyGame_Config> game_configs = {};
    std::vector<FrequencyGame_Results> game_results_history = {};
    int current_config_idx = 0;

    juce::TooltipWindow tooltip_window {this, 300};
    
#if 0
    juce::TextButton burger { "menu" };
    Empty empty;
    juce::SidePanel sidePanel { "Menu", 150, true };
#endif
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Main_Component)
};