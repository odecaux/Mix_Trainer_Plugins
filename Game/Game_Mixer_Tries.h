Effects mixer_game_tries_update(MixerGame_State *state, Event event);

std::unique_ptr<MixerGame_State> mixer_game_init_tries(
    std::unordered_map<int, ChannelInfos> &channel_infos,
    int tries,
    std::vector<double> db_slider_values,
    Application *app);


//TODO mutex ? pour les timeout