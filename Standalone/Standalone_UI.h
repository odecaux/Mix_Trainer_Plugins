

inline juce::Colour getUIColourIfAvailable (juce::LookAndFeel_V4::ColourScheme::UIColour uiColour, juce::Colour fallback = juce::Colour (0xff4d4d4d)) noexcept
{
    if (auto* v4 = dynamic_cast<juce::LookAndFeel_V4*>(&juce::LookAndFeel::getDefaultLookAndFeel()))
        return v4->getCurrentColourScheme().getUIColour (uiColour);

    return fallback;
}

inline std::unique_ptr<juce::InputSource> makeInputSource (const juce::File& file)
{
    return std::make_unique < juce::FileInputSource > (file);
}


//==============================================================================
class Thumbnail  :
    public juce::Component,
public juce::ChangeListener,
public juce::ChangeBroadcaster,
private juce::ScrollBar::Listener,
private juce::Timer
{
public:
    Thumbnail (juce::AudioFormatManager *formatManager,
               juce::AudioTransportSource *source)
    : transport_source (source),
      thumbnail (512, *formatManager, thumbnail_cache)
    {
        thumbnail.addChangeListener (this);

        addAndMakeVisible (scrollbar);
        scrollbar.setRangeLimits (visible_range);
        scrollbar.setAutoHide (false);
        scrollbar.addListener (this);
        
        current_position_marker.setFill (juce::Colours::white.withAlpha (0.85f));
        addAndMakeVisible (current_position_marker);
        
        current_bounds_marker.setFill (juce::Colours::yellow.withAlpha (0.3f));
        addAndMakeVisible (current_bounds_marker);
    }

    ~Thumbnail() override
    {
        scrollbar.removeListener (this);
        thumbnail.removeChangeListener (this);
    }

    void setFile (Audio_File *audio_file)
    {
        if (auto inputSource = makeInputSource (audio_file->file))
        {
            thumbnail.setSource (inputSource.release());

            juce::Range < double > newVisibleRange (0.0, thumbnail.getTotalLength());
            scrollbar.setRangeLimits (newVisibleRange);
            loop_bounds_ms = audio_file->loop_bounds_ms;
            file_length_ms = audio_file->file_length_ms;
            setVisibleRange (newVisibleRange);
            startTimerHz (40);
        }
    }

    void removeFile()
    {
        thumbnail.setSource(nullptr);
        //triggers changed
        loop_bounds_ms = { -1, -1 };
        file_length_ms = -1;
    }

    void setZoomFactor (double amount)
    {
        if (thumbnail.getTotalLength() > 0)
        {
            auto newScale = juce::jmax (0.001, thumbnail.getTotalLength() * (1.0 - juce::jlimit (0.0, 0.99, amount)));
            auto timeAtCentre = xToTime ((float) getWidth() / 2.0f);

            setVisibleRange ( { timeAtCentre - newScale * 0.5, timeAtCentre + newScale * 0.5 });
        }
    }

    void setVisibleRange (juce::Range < double > new_visible_range)
    {
        visible_range = new_visible_range;
        scrollbar.setCurrentRange (visible_range);
        updateCursorPosition();
        updateLoopBoundsPosition();
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::darkgrey);
        g.setColour (juce::Colours::lightblue);

        if (thumbnail.getTotalLength() > 0.0)
        {
            auto thumbnail_area = getLocalBounds();

            thumbnail_area.removeFromBottom (scrollbar.getHeight() + 4);
            thumbnail.drawChannels (g, thumbnail_area.reduced (2),
                                    visible_range.getStart(), visible_range.getEnd(), 1.0f);
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

    void mouseDown(const juce::MouseEvent& e) override
    {
        mouseDrag(e);
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (file_length_ms == -1) return;
        float ratio = e.x / (float)getWidth();
        ratio = std::clamp(ratio, 0.0f, 1.0f);
        uint64_t position_ms = uint64_t(ratio * (float)file_length_ms); //todo arithmetics
        position_ms = std::min(position_ms, checked_cast<uint64_t>(file_length_ms));
        if (e.mods.isLeftButtonDown())
        {
            position_ms = std::min(position_ms, checked_cast<uint64_t>(loop_bounds_ms.getEnd()));
            loop_bounds_ms.setStart(position_ms);
        }
        else if (e.mods.isRightButtonDown())
        {
            position_ms = std::max(position_ms, checked_cast<uint64_t>(loop_bounds_ms.getStart()));
            loop_bounds_ms.setEnd(position_ms);
        }
        else return;

        loop_bounds_changed(loop_bounds_ms);
        updateLoopBoundsPosition();
        repaint();
        //transport_source.setPosition(std::max (0.0, xToTime((float) e.x)));
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        //transport_source.start();
    }

    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
    {
        if (thumbnail.getTotalLength() > 0.0)
        {
            auto newStart = visible_range.getStart() - wheel.deltaX * (visible_range.getLength()) / 10.0;
            newStart = juce::jlimit (0.0, juce::jmax (0.0, thumbnail.getTotalLength() - (visible_range.getLength())), newStart);

            setVisibleRange ( { newStart, newStart + visible_range.getLength() });

            if (wheel.deltaY != 0.0f)
            {
                zoom_value = zoom_value - wheel.deltaY;
                zoom_value = std::clamp(zoom_value, 0.0f, 1.0f);
                setZoomFactor (zoom_value);
            }

            repaint();
        }
    }
    
    std::function<void(juce::Range<int64_t>)> loop_bounds_changed = {};

private:
    float zoom_value = 0.0F;
    juce::AudioTransportSource *transport_source;
    juce::ScrollBar scrollbar { false };
    juce::Range < int64_t > loop_bounds_ms = { -1, -1 };
    int64_t file_length_ms = -1;

    juce::AudioThumbnailCache thumbnail_cache { 5 };
    juce::AudioThumbnail thumbnail;
    juce::Range < double > visible_range;

    juce::DrawableRectangle current_position_marker;
    juce::DrawableRectangle current_bounds_marker;

    float timeToX (const double time) const
    {
        if (visible_range.getLength() <= 0)
            return 0;

        return (float) getWidth() * (float) ((time - visible_range.getStart()) / visible_range.getLength());
    }

    double xToTime (const float x) const
    {
        return (x / (float) getWidth()) * (visible_range.getLength()) + visible_range.getStart();
    }

    void scrollBarMoved (juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart) override
    {
        if (scrollBarThatHasMoved == &scrollbar)
            setVisibleRange (visible_range.movedToStartAt (newRangeStart));
    }

    void timerCallback() override
    {
        updateCursorPosition();
    }

    void updateCursorPosition()
    {
        if (!(transport_source->isPlaying() || isMouseButtonDown()))
        {
            current_position_marker.setVisible (false);
            return;
        }

        float cursor_width = 1.5f;
        auto time = transport_source->getCurrentPosition();
        float cursor_left_x = timeToX (time) - cursor_width / 2.0f;
        int cursor_height = getHeight() - scrollbar.getHeight();
        current_position_marker.setVisible (true);
        current_position_marker.setRectangle (juce::Rectangle < float > (cursor_left_x, 0, cursor_width, (float) cursor_height));
    }

    void updateLoopBoundsPosition()
    {
        auto start_time_ms = loop_bounds_ms.getStart();
        auto end_time_ms = loop_bounds_ms.getEnd();
        auto start_ratio = (double)start_time_ms / (double)file_length_ms;
        auto end_ratio = (double)end_time_ms / (double)file_length_ms;
        double start_x = start_ratio * (double)getWidth() - 0.75;
        double end_x = end_ratio * (double)getWidth() + 0.75;
        start_x = std::max(0.0, start_x);
        end_x = std::min((double)getWidth(), end_x);

        if (end_x < 0.0f || start_x > getWidth())
        {
            current_bounds_marker.setVisible(false);
            return;
        }

        current_bounds_marker.setVisible(true);
        int cursor_height = getHeight() - scrollbar.getHeight();
        auto rect = juce::Rectangle < double > (start_x, 0.0, end_x - start_x, (double) cursor_height);
        current_bounds_marker.setRectangle (rect.toFloat());

    }
};


class Audio_File_Chooser : public juce::Component
{
public:
    Audio_File_Chooser(std::function<void()> onBackClicked,
                       std::function<void()> onBeginClicked,
                       Audio_File_List *audio_file_list)
    : list_comp(true)
    {
        auto validate_next_button = [&] (const std::vector<bool> & new_selection)
        {
            if (std::any_of(new_selection.begin(), new_selection.end(), [](bool val) { return val; }))
                bottom.next_button.setEnabled(true);
            else
                bottom.next_button.setEnabled(false);
        };
        
        std::vector<juce::String> file_names{};
        std::vector<bool> initial_selection{};
        for (uint64_t hash : audio_file_list->order)
        {
            file_names.push_back(audio_file_list->files.at(hash).title);
            initial_selection.push_back(audio_file_list->selected.at(hash));
        }
        validate_next_button(initial_selection);

        list_comp.selection_changed_callback =
            [audio_file_list, selection_changed = std::move(validate_next_button)] (std::vector<bool> *new_selection)
        {
            assert(new_selection->size() == audio_file_list->order.size());
            for (uint32_t i = 0; i < new_selection->size(); i++)
            {
                uint64_t hash = audio_file_list->order[i];
                audio_file_list->selected.at(hash) = new_selection->at(i);
            }
            selection_changed(*new_selection);
        };
        list_comp.set_rows(file_names, initial_selection);
        
        header.onBackClicked = std::move(onBackClicked);
        
        game_ui_header_update(&header, "Select active files", "");
        game_ui_bottom_update(&bottom, true, "Next", Mix_Hidden, {});
        bottom.next_button.onClick = std::move(onBeginClicked);
        //TODO pas foufifou, ça devrait faire partie de l'API non ?
        addAndMakeVisible(header);
        addAndMakeVisible(list_comp);
        addAndMakeVisible(bottom);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced(4);
        auto header_bounds = r.removeFromTop(header.getHeight());
        header.setBounds(header_bounds);
        auto bottom_bounds = r.removeFromBottom(bottom.getHeight());
        bottom.setBounds(bottom_bounds);

        list_comp.setBounds(r);
    }
    

private:
    GameUI_Header header;
    Selection_List list_comp;
    GameUI_Bottom bottom;
};


//------------------------------------------------------------------------
class Audio_Files_ListBox :
    public juce::Component,
public juce::ListBoxModel,
public juce::FileDragAndDropTarget
{
public:
    Audio_Files_ListBox()
    {
        list_comp.setMultipleSelectionEnabled(true);
        list_comp.setColour (juce::ListBox::outlineColourId, juce::Colours::grey);      // [2]
        list_comp.setOutlineThickness (2);
        addAndMakeVisible(list_comp);
    }

    virtual ~Audio_Files_ListBox() override = default;

    void resized() override
    {
        list_comp.setBounds(getLocalBounds());
    }

    void paintOverChildren(juce::Graphics& g) override
    {
        if (getNumRows() == 0)
        {
            auto r = getLocalBounds();
            g.setColour(juce::Colours::white);
            g.drawText("Drag and drop audio files here", r.toFloat(), juce::Justification::centred);
        }
    }

    int getNumRows() override { return (int)row_texts.size(); }

    void paintListBoxItem (int rowNumber,
                           juce::Graphics& g,
                           int width, int height,
                           bool rowIsSelected) override
    {
        auto alternateColour = getLookAndFeel().findColour (juce::ListBox::backgroundColourId)
            .interpolatedWith (getLookAndFeel().findColour (juce::ListBox::textColourId), 0.03f);
        if (rowNumber % 2)
            g.fillAll (alternateColour);
        
        if (rowNumber >= getNumRows()) 
            return;
        auto bounds = juce::Rectangle { 0, 0, width, height };
        g.setColour(juce::Colours::white);
        g.drawText(row_texts[checked_cast<size_t>(rowNumber)], bounds.reduced(2), juce::Justification::centredLeft);
        if (rowIsSelected)
        {
            g.drawRect(bounds);
        }
    }

    bool isInterestedInFileDrag (const juce::StringArray&) override { return true; }
    
    void filesDropped (const juce::StringArray& dropped_files_names, int, int) override
    {
        std::vector<juce::File> files{};
        for (const auto &filename : dropped_files_names)
        {
            files.emplace_back(filename);
        }
        file_dropped_callback(files);
    }

    
    void listBoxItemClicked (int, const juce::MouseEvent&) override {}

    void listBoxItemDoubleClicked (int, const juce::MouseEvent &) override {}
    
    void deleteKeyPressed (int) override
    {
        auto num_rows = getNumRows();
        auto num_selected = list_comp.getNumSelectedRows();
        if (num_selected == 0)
            return;
        
        delete_pressed_callback();
#if 0
        list_comp.updateContent();
        
        if (num_selected > 1)
        {
            list_comp.deselectAllRows();
        }
        else if (num_selected == 1)
        {
            auto row_to_delete = selected_rows[0];
            if (row_to_delete == -1)
                return;

            uint32_t row_to_select = 0;
            if (row_to_delete == 0)
                row_to_select = 0;
            else if (row_to_delete == num_rows - 1)
                row_to_select = row_to_delete - 1;
            else
                row_to_select = row_to_delete;
            bool should_trigger_manually = row_to_select == checked_cast<uint32_t>(list_comp.getSelectedRow());
            list_comp.selectRow(row_to_select);
            if (should_trigger_manually)
                selectedRowsChanged(row_to_select);
        }
#endif
    }

    bool keyPressed (const juce::KeyPress &key) override
    {
        if (key == key.escapeKey)
        {
            list_comp.deselectAllRows();
            return true;
        }
        return false;
    }
   
    void selectedRowsChanged (int last_row_selected) override
    {
        //selection_changed_callback();
        row_clicked_callback(last_row_selected);
    }

    std::vector<int> getSelectedRows()
    {
        auto juce_selection = list_comp.getSelectedRows();
        std::vector<int> std_selection{};
        for (auto i = 0; i < juce_selection.size(); i++)
        {
            int selected_idx = juce_selection[i];
            std_selection.emplace_back(selected_idx);
        }
        return std_selection;
    }

    void set_rows(const std::vector<std::string> &rowTexts,
                  const std::vector<bool> &selection)
    {
        assert(selection.size() == rowTexts.size());
        row_texts = std::move(rowTexts);

        juce::SparseSet < int > selected_juce{};
        for (uint32_t i = 0; i < selection.size(); i++)
        {
            if(selection[i])
                selected_juce.addRange(juce::Range<int>(i, i + 1));
        }
        list_comp.updateContent();
        list_comp.setSelectedRows(selected_juce, juce::dontSendNotification);
    }
    

    std::function<void(std::vector<juce::File> files)> file_dropped_callback;
    std::function<void()> delete_pressed_callback;
    //std::function<void()> selection_changed_callback;
    std::function<void(int)> row_clicked_callback;

private:
    std::vector<std::string> row_texts;
    juce::ListBox list_comp = { {}, this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Audio_Files_ListBox)
};


//------------------------------------------------------------------------
class Audio_File_Settings_Panel :
    public juce::Component,
public juce::DragAndDropContainer
{
public:
    
    Audio_File_Settings_Panel(File_Player *filePlayer,
                              Audio_File_List *audioFileList,
                              std::function<void()> onClickBack)
    :  player(filePlayer),
       audio_file_list(audioFileList),
       file_list_component()
    {
        {
            header.onBackClicked = [click = std::move(onClickBack)] {
                click();
            };
            game_ui_header_update(&header, "Audio Files", {});
            addAndMakeVisible(header);
        }

        {
            file_list_component.row_clicked_callback = [&] (int row_idx)
            {
                if (row_idx != -1)
                {
                    auto hash = audio_file_list->order[row_idx];
                    auto &selected_file = audio_file_list->files.at(hash);
                    assert(hash == selected_file.hash);
                    auto player_state = file_player_post_command(player, { .type = Audio_Command_Load, .value_file = selected_file });
                    if(player_state.step == Transport_Loading_Failed)
                        return;
                    //TODO file does not exist anymore ?
                    file_player_post_command(player, { .type = Audio_Command_Play });
                    thumbnail.setFile(&selected_file);
                    file_is_selected = true;
                    selected_file_hash = selected_file.hash;
                    frequency_bounds_slider.setMinAndMaxValues((float)selected_file.freq_bounds.getStart(), (float)selected_file.freq_bounds.getEnd());
                }
                else
                {
                    file_player_post_command(player, { .type = Audio_Command_Stop });
                    thumbnail.removeFile();
                    file_is_selected = false;
                    frequency_bounds_slider.setMinAndMaxValues(20.0f, 20000.0f);
                }
            };
            file_list_component.file_dropped_callback =
                [&, &format_manager = filePlayer->format_manager]
                (auto files_dropped)
            {
                for(auto &file : files_dropped)
                    insert_file(audio_file_list, file, format_manager);
                auto [titles, selection] = generate_titles_and_selection_lists(audio_file_list);
                file_list_component.set_rows(titles, selection);
            };

            file_list_component.delete_pressed_callback = [&] ()
            {
                auto files_to_remove = file_list_component.getSelectedRows();
                remove_files(audio_file_list, files_to_remove);
                auto [titles, selection] = generate_titles_and_selection_lists(audio_file_list);
                file_list_component.set_rows(titles, selection);
            };
            auto [titles, selection] = generate_titles_and_selection_lists(audio_file_list);
            file_list_component.set_rows(titles, selection);
            addAndMakeVisible(file_list_component);
        }
        
        thumbnail.loop_bounds_changed = [&] (juce::Range < int64_t > new_loop_bounds_ms){
            audio_file_list->files.at(selected_file_hash).loop_bounds_ms = new_loop_bounds_ms;
            Audio_Command command = {
                .type = Audio_Command_Update_Loop,
                .loop_start_ms = new_loop_bounds_ms.getStart(),
                .loop_end_ms = new_loop_bounds_ms.getEnd()
            };
            file_player_post_command(player, command);
        };
        addAndMakeVisible(thumbnail);
        frequency_bounds_slider.on_mix_max_changed = 
            [&] (float begin, float end, float) {
            if(file_is_selected)
                audio_file_list->files.at(selected_file_hash).freq_bounds = { (uint32_t)begin, (uint32_t)end };
        };
        addAndMakeVisible(frequency_bounds_slider);
        
        {
            {
                juce::Path plus_path;
                plus_path.addRectangle( 40, 0, 20, 100 );
                plus_path.addRectangle( 0, 40, 100, 20 ); 
                auto click_plus = [&]
                {
                    open_file_dialog = std::make_unique < juce::FileChooser > ("Select audio files", juce::File::getSpecialLocation (juce::File::userHomeDirectory), "*.wav");
                    auto flag = juce::FileBrowserComponent::openMode;
                    auto callback = [this] (const juce::FileChooser& chooser)
                    {
                        auto files_chosen = chooser.getResults();
                        for(auto &file : files_chosen)
                            insert_file(audio_file_list, file, player->format_manager);
                        auto [titles, selection] = generate_titles_and_selection_lists(audio_file_list);
                        file_list_component.set_rows(titles, selection);
                    };
                    open_file_dialog->launchAsync (flag, std::move(callback));
                };
                column.add_button(plus_path, click_plus);
            }
            {
                juce::Path delete_path;
                juce::AffineTransform transform;
                transform = transform.rotated(juce::degreesToRadians(45.0f));
                delete_path.addRectangle( 40, 0, 20, 100 );
                delete_path.addRectangle( 0, 40, 100, 20 );
                delete_path.applyTransform(transform);
                auto click_delete = [&] ()
                {
                    auto files_to_remove = file_list_component.getSelectedRows();
                    remove_files(audio_file_list, files_to_remove);
                    auto [titles, selection] = generate_titles_and_selection_lists(audio_file_list);
                    file_list_component.set_rows(titles, selection);
                };
                column.add_button(delete_path, std::move(click_delete));
            }
            {
                juce::Path up_arrow;
                up_arrow.addArrow ( { 50.0f, 100.0f, 50.0f, 0.0f }, 40.0f, 100.0f, 50.0f);
                column.add_button(up_arrow);
            }
            {
                juce::Path down_arrow;
                down_arrow.addArrow ( { 50.0f, 0.0f, 50.0f, 100.0f }, 40.0f, 100.0f, 50.0f);
                column.add_button(down_arrow);
            }
            addAndMakeVisible(column);
        }
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (4);
        auto header_bounds = r.removeFromTop(header.getHeight());
        header.setBounds(header_bounds);

        auto bottom_bounds = r.removeFromBottom(100);
        
        auto file_list_bounds = r.withTrimmedRight(30);
        auto column_bounds = r.withTrimmedLeft(r.getWidth() - 30);
        file_list_component.setBounds (file_list_bounds); 
        column.setBounds(column_bounds);
        
        frequency_bounds_slider.setBounds(bottom_bounds.removeFromBottom(20).reduced(50, 0));
        thumbnail.setBounds(bottom_bounds);
    }
    

    bool keyPressed (const juce::KeyPress &key) override
    {
        if (key == key.spaceKey)
        {
            file_player_post_command(player, { .type = Audio_Command_Stop });
            return true;
        }
        return false;
    }

private:
    File_Player *player;
    Audio_File_List *audio_file_list;
    GameUI_Header header;
    Audio_Files_ListBox file_list_component;

    Thumbnail thumbnail { player->format_manager, &player->transport_source };
    Frequency_Bounds_Widget frequency_bounds_slider;
    bool file_is_selected = false;
    uint64_t selected_file_hash;
    Add_Delete_Move_Column column;

    std::unique_ptr<juce::FileChooser> open_file_dialog;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Audio_File_Settings_Panel)
};


//------------------------------------------------------------------------
class MainMenu_Panel : public juce::Component
{
public :
    MainMenu_Panel(std::function<void()> && toFrequencyGame,
                   std::function<void()> && toCompressorGame,
                   std::function<void()> && toFileList,
                   std::function<void()> && toStats)
    {
        frequency_game_button.setSize(100, 40);
        frequency_game_button.setButtonText("Learn EQs");
        frequency_game_button.onClick = [click = std::move(toFrequencyGame)] {
            click();
        };
        addAndMakeVisible(frequency_game_button);
        
        compressor_game_button.setSize(100, 40);
        compressor_game_button.setButtonText("Learn Compressors");
        compressor_game_button.onClick = [click = std::move(toCompressorGame)] {
            click();
        };
        addAndMakeVisible(compressor_game_button);
        
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
        frequency_game_button.setCentrePosition(bounds.getCentre() + juce::Point < int > (0, -75));
        compressor_game_button.setCentrePosition(bounds.getCentre() + juce::Point < int > (0, -25));
        file_list_button.setCentrePosition(bounds.getCentre() + juce::Point < int > (0, 25));
        stats_button.setCentrePosition(bounds.getCentre() + juce::Point < int > (0, 75));
    }

    juce::TextButton frequency_game_button;
    juce::TextButton compressor_game_button;
    juce::TextButton file_list_button;
    juce::TextButton stats_button;
};


class Main_Component : public juce::Component
{
public :
    
    Main_Component(juce::AudioFormatManager *formatManager)
    : application(formatManager, this)
    {
        addAndMakeVisible(main_fader);
        setSize (500, 300);
#if 0
        auto selection_list = std::make_unique < Selection_List > (
            std::vector<juce::String> { "yes", "no", "maybe", "I don't know" },
            std::vector<int> { 0, 3 },
            true,
            [](const auto &){}
        );
        changePanel(std::move(selection_list));
#endif
    }

    void resized() override
    {
        auto r = getLocalBounds();
        auto main_fader_bounds = r.removeFromRight(60);
        main_fader.setBounds(main_fader_bounds);
        auto panelBounds = r;
        if (panel)
            panel->setBounds(panelBounds);
    }

    void changePanel(std::unique_ptr<juce::Component> new_panel)
    {
        panel = std::move(new_panel);
        if (panel)
            addAndMakeVisible(*panel);
        resized();
    }

    Continuous_Fader main_fader;
private :
    std::unique_ptr<juce::Component> panel;
    Application_Standalone application;

    juce::TooltipWindow tooltip_window { this, 300 };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Main_Component)
};