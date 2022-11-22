Effects mixer_game_tries_update(MixerGame_State *state, Event event);

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