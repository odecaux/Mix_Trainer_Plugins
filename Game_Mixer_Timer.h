Effects mixer_game_timer_update(MixerGame_State *state, Event event);
void game_ui_bottom_update_timer(GameUI_Bottom *bottom, GameStep new_step,  juce::String button_text, Mix mix);
void game_ui_update_timer(Effect_UI &new_ui, MixerGameUI &ui);

static std::unique_ptr<MixerGame_State> mixer_game_init_timer(
    std::unordered_map<int, ChannelInfos> &channel_infos,
    int timeout_ms,
    std::vector<double> db_slider_values,
    Application *app)
{
    MixerGame_State state = {
        .channel_infos = channel_infos,
        .timeout_ms = timeout_ms,
        .db_slider_values = db_slider_values,
        .app = app,
        .timer = Timer{}
    };
    return std::make_unique < MixerGame_State > (std::move(state));
}


//TODO mutex ? pour les timeout