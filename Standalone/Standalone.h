
#pragma once
#include "../shared/pch.h"
#include "../shared/shared.h"
#include "../Game/Game.h"
#include "FrequencyWidget.h"
#include "FrequencyGame.h"

class Main_Component;

#include "Application_Standalone.h"

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


class Main_Component : public juce::Component
{
public :
    
    Main_Component(juce::AudioFormatManager &formatManager)
    : application(formatManager, this)
    {
        setSize (500, 300);
    }

    void resized() override 
    {
        auto r = getLocalBounds();
        auto panelBounds = r;
        if(panel)
            panel->setBounds(panelBounds);
    }

    void changePanel(std::unique_ptr<juce::Component> new_panel)
    {
        panel = std::move(new_panel);
        if(panel)
            addAndMakeVisible(*panel);
        resized();
    }

    private :
    std::unique_ptr<juce::Component> panel;
    Application_Standalone application;

    juce::TooltipWindow tooltip_window {this, 300};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Main_Component)
};

#include "Application_Standalone.cpp"