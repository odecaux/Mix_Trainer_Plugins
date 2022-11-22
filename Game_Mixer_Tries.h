Effects mixer_game_tries_update(MixerGame_State *state, Event event);
void game_ui_bottom_update_tries(GameUI_Bottom *bottom, GameStep new_step, int remaining_listens,  juce::String button_text);
void game_ui_update_tries(Effect_UI &new_ui, MixerGameUI &ui);

static std::unique_ptr<MixerGame_State> mixer_game_init_tries(
    std::unordered_map<int, ChannelInfos> &channel_infos,
    int tries,
    std::vector<double> db_slider_values,
    Application *app)
{
    MixerGame_State state = {
        .channel_infos = channel_infos,
        .listens = tries,
        .db_slider_values = db_slider_values,
        .app = app
    };
    return std::make_unique < MixerGame_State > (std::move(state));
}


//TODO mutex ? pour les timeout