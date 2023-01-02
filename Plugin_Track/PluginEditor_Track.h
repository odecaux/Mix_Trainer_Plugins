/*
  ==============================================================================

    PluginEditor_Track.h
    Created: 16 Sep 2022 9:09:15am
    Author:  Octave

  ==============================================================================
*/

#pragma once
class EditorTrack : public juce::AudioProcessorEditor
{
    public:
    
    EditorTrack(ProcessorTrack& p,
                const std::vector<Game_Channel> &gameChannels,
                int64_t &gameTrackId
                #if 0
                const juce::String &name, 
                float minFrequency, 
                float maxFrequency
                #endif
                )
    : AudioProcessorEditor(p),
      audioProcessor(p),
      game_channels(gameChannels),
      game_track_id(gameTrackId)
    {
#if 0
        track_name_label.setSize(200, 30);
        track_name_label.setJustificationType(juce::Justification::centred);
        track_name_label.setText(name, juce::dontSendNotification);
        addAndMakeVisible(track_name_label);

        
        frequency_bounds_widget.setMinAndMaxValues(minFrequency, maxFrequency);
        frequency_bounds_widget.setSize(200, 100);
        frequency_bounds_widget.on_mix_max_changed = [&] 
            (float new_min_frequency, float new_max_frequency, bool done_dragging) 
            {
                if(done_dragging)
                    audioProcessor.frequencyRangeChanged(new_min_frequency, new_max_frequency);
            };
            
        addAndMakeVisible(frequency_bounds_widget);

#endif
        game_track_combo.setSize(150, 30);
        game_track_combo.setEditableText(false);
        game_track_combo.setJustificationType(juce::Justification::left);
        game_track_combo.onChange = [&] { 
            auto selected_idx = game_track_combo.getSelectedId();
            if (selected_idx == 0)
                game_track_id = -1;
            else
                game_track_id = gameChannels[selected_idx - 1].id;
            audioProcessor.broadcast_selected_game_channel();
        };
        addAndMakeVisible(game_track_combo);

        setSize(400, 300);
        setResizable(true, false);
        update_track_list();
    }
    
    void paint(juce::Graphics& g) override
    {
        // (Our component is opaque, so we must completely fill the background with a solid colour)
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        auto center = bounds.getCentre();
        game_track_combo.setCentrePosition(center);
#if 0
        track_name_label.setCentrePosition(center.x, 40);
        frequency_bounds_widget.setCentrePosition(center.x, center.y);
#endif
    }

#if 0
    void renameTrack(const juce::String &new_name)
    {
        track_name_label.setText(new_name, juce::dontSendNotification);
    }
#endif

    void update_track_list()
    {
        game_track_combo.clear(juce::dontSendNotification);
        for (auto i = 0; i < game_channels.size(); i++)
        {
            const Game_Channel& track = game_channels[i];
            game_track_combo.addItem(track.name, i + 1);
            if(track.id == game_track_id)
                game_track_combo.setSelectedId(i + 1, juce::dontSendNotification);
        }
    }
    
    private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ProcessorTrack& audioProcessor;
    
    const std::vector<Game_Channel> &game_channels;
    int64_t &game_track_id;
    juce::ComboBox game_track_combo;
#if 0
    juce::Label track_name_label;
    Frequency_Bounds_Widget frequency_bounds_widget;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditorTrack)
};

