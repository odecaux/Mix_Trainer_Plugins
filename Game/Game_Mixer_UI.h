void mixer_game_ui_transitions(MixerGameUI &ui, Game_Mixer_Effect_Transition transition);
void game_ui_update(const Game_Mixer_Effect_UI &new_ui, MixerGameUI &ui);


struct MixerGameUI : public juce::Component
{
    MixerGameUI(const std::unordered_map<uint32_t, Game_Channel>& channelInfo,
                const std::vector<double> &SliderValuesdB,
                MixerGame_IO *gameIO) 
    :
    fader_row(faders),
    db_slider_values(SliderValuesdB),
    io(gameIO)
    {
        auto f =
            [io = io, &fader_row = fader_row, &db_slider_values = db_slider_values] (const auto &a)->std::pair < int, std::unique_ptr<FaderComponent> > {
            const int id = a.first;
            
            auto onFaderMoved = [id, io] (int new_pos) {
                Event event = {
                    .type = Event_Slider,
                    .id = id,
                    .value_i = new_pos
                };
                mixer_game_post_event(io, event);
            };
            
            auto new_fader = std::make_unique < FaderComponent > (db_slider_values,
                a.second.name,
                std::move(onFaderMoved));
            fader_row.addAndMakeVisible(new_fader.get());
            return { id, std::move(new_fader)};
        };
        
        std::transform(channelInfo.begin(), channelInfo.end(), 
                       std::inserter(faders, faders.end()), 
                       f);
        fader_row.adjustWidth();
        fader_viewport.setScrollBarsShown(false, true);
        fader_viewport.setViewedComponent(&fader_row, false);
        
        bottom.onNextClicked = [io = io] (Event_Type e){
            Event event = {
                .type = e
            };
            
            mixer_game_post_event(io, event);
        };
        header.onBackClicked = [io = io] {
            Event event = {
                .type = Event_Click_Back
            };
            mixer_game_post_event(io, event);
        };
        bottom.onToggleClicked = [io = io] (bool a){
            Event event = {
                .type = Event_Toggle_Input_Target,
                .value_b = a
            };
            mixer_game_post_event(io, event);
        };
        
        bottom.target_mix_button.setButtonText("Target mix");
        bottom.user_mix_button.setButtonText("Your mix");

        addAndMakeVisible(header);
        addAndMakeVisible(fader_viewport);
        addAndMakeVisible(bottom);
    }
    
    void resized() override 
    {
        auto bounds = getLocalBounds();
        
        auto header_bounds = bounds.removeFromTop(header.getHeight());
        header.setBounds(header_bounds);

        auto bottom_bounds = bounds.removeFromBottom(bottom.getHeight());
        bottom.setBounds(bottom_bounds);
        
        auto game_bounds = bounds;
        fader_viewport.setBounds(game_bounds);
        
        fader_row.setSize(fader_row.getWidth(),
                          fader_viewport.getHeight() - fader_viewport.getScrollBarThickness());
    }
    
    std::unordered_map < int, std::unique_ptr<FaderComponent>> faders;
    FaderRowComponent fader_row;
    juce::Viewport fader_viewport;
    const std::vector < double > &db_slider_values;
    GameUI_Header header;
    GameUI_Bottom bottom;
    MixerGame_IO *io;
};
