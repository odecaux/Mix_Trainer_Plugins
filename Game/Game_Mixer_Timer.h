Effects mixer_game_timer_update(MixerGame_State *state, Event event);

std::unique_ptr<MixerGame_State> mixer_game_init_timer(
    std::unordered_map<int, ChannelInfos> &channel_infos,
    int timeout_ms,
    std::vector<double> db_slider_values,
    Application *app);


//TODO mutex ? pour les timeout