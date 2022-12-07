
struct CompressorGame_UI : public juce::Component
{
    
    CompressorGame_UI(
#if 0
        FrequencyGame_State *game_state, 
        FrequencyGame_IO *game_io
#endif
    ) 
#if 0
    : 
        game_io(game_io),
        game_state(game_state)
#endif
    {
#if 0
        bottom.onNextClicked = [game_state, game_io] (Event_Type e){
            Event event = {
                .type = e
            };
            frequency_game_post_event(game_state, game_io, event);
        };
        header.onBackClicked = [game_state, game_io] {
            Event event = {
                .type = Event_Click_Back
            };
            //frequency_game_post_event(game_state, game_io, event);
        };
        bottom.onToggleClicked = [game_state, game_io] (bool a){
            Event event = {
                .type = Event_Toggle_Input_Target,
                .value_b = a
            };
            frequency_game_post_event(game_state, game_io, event);
        };
#endif
        addAndMakeVisible(header);
        addAndMakeVisible(bottom);
    }

    void resized() override 
    {
        auto bounds = getLocalBounds().reduced(4);
        auto bottom_height = 50;

        auto header_bounds = bounds.removeFromTop(game_ui_header_height);
        auto bottom_bounds = bounds.removeFromBottom(bottom_height);
        auto game_bounds = bounds.withTrimmedTop(4).withTrimmedBottom(4);

        header.setBounds(header_bounds);
#if 0
        if(frequency_widget)
            frequency_widget->setBounds(game_bounds);
        else if(results_panel)
            results_panel->setBounds(game_bounds);
#endif
        bottom.setBounds(bottom_bounds);
    }

    GameUI_Header header;
#if 0
    std::unique_ptr<FrequencyWidget> frequency_widget;
    std::unique_ptr<FrequencyGame_Results_Panel> results_panel;
#endif
    GameUI_Bottom bottom;
#if 0
    FrequencyGame_IO *game_io;
    FrequencyGame_State *game_state;
#endif
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorGame_UI)
};
