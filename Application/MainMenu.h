
class MainMenu : public juce::Component
{
public :
    MainMenu(std::function<void()> && toNormalGameButtonClick,
             std::function<void()> && toTimerGameButtonClick,
             std::function<void()> && toTriesGameButtonClick,
             std::function<void()> && toChannelSettingsButtonClick,
             std::function<void()> && toStatsButtonClick,
             std::function<void()> && toSettingsButtonClick,
             MuliTrack_Model *multitrackModel)
    :   multitrack_model { multitrackModel }
    {
        game_normal_button.setSize(100, 40);
        game_normal_button.setButtonText("Normal");
        game_normal_button.onClick = [click = std::move(toNormalGameButtonClick)] {
            click();
        };
        addAndMakeVisible(game_normal_button);
        
        game_timer_button.setSize(100, 40);
        game_timer_button.setButtonText("Timer");
        game_timer_button.onClick = [click = std::move(toTimerGameButtonClick)] {
            click();
        };
        addAndMakeVisible(game_timer_button);
        
        game_tries_button.setSize(100, 40);
        game_tries_button.setButtonText("Tries");
        game_tries_button.onClick = [click = std::move(toTriesGameButtonClick)] {
            click();
        };
        addAndMakeVisible(game_tries_button);

        
        channel_settings_button.setSize(100, 40);
        channel_settings_button.setButtonText("Game Channels Settings");
        channel_settings_button.onClick = [click = std::move(toChannelSettingsButtonClick)] {
            click();
        };
        addAndMakeVisible(channel_settings_button);
        
        stats_button.setSize(100, 40);
        stats_button.setButtonText("Statistics");
        stats_button.setEnabled(false);
        stats_button.onClick = [click = std::move(toStatsButtonClick)] {
            click();
        };
        addAndMakeVisible(stats_button);
        
        settings_button.setSize(100, 40);
        settings_button.setButtonText("Settings");
        settings_button.setEnabled(false);
        settings_button.onClick = [click = std::move(toSettingsButtonClick)] {
            click();
        };
        addAndMakeVisible(settings_button);

        auto model_observer = [&] (MuliTrack_Model *model)
        {
            bool is_valid = true;
            for (auto &[game_channel_id, assigned_count] : model->assigned_daw_track_count)
            {
                if (assigned_count != 1)
                {
                    is_valid = false;
                    break;
                }
            }
            if (is_valid)
            {
                game_normal_button.setEnabled(true);
                game_timer_button.setEnabled(true);
                game_tries_button.setEnabled(true);
            }
            else
            {
                game_normal_button.setEnabled(false);
                game_timer_button.setEnabled(false);
                game_tries_button.setEnabled(false);
            }
        };
        model_observer(multitrack_model);
        multitrack_model_add_observer(multitrack_model, MultiTrack_Observers_Main_Menu, std::move(model_observer));
    }
    ~MainMenu()
    {
        multitrack_model_remove_observer(multitrack_model, MultiTrack_Observers_Main_Menu);
    }

    void paint(juce::Graphics &g) override
    {
        juce::ignoreUnused(g);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto centre = bounds.getCentre();
        game_normal_button.setCentrePosition(centre + juce::Point<int>(0, -125));
        game_timer_button.setCentrePosition(centre + juce::Point<int>(0, -75));
        game_tries_button.setCentrePosition(centre + juce::Point<int>(0, -25));
        channel_settings_button.setCentrePosition(centre + juce::Point<int>(0, 25));
        stats_button.setCentrePosition(centre + juce::Point<int>(0, 75));
        settings_button.setCentrePosition(centre + juce::Point<int>(0, 125));
    }

private :
    juce::TextButton game_normal_button;
    juce::TextButton game_timer_button;
    juce::TextButton game_tries_button;
    juce::TextButton channel_settings_button;
    juce::TextButton stats_button;
    juce::TextButton settings_button;
    MuliTrack_Model *multitrack_model;
};

class ChannelSettingsMenu : public juce::Component
{
public :
    ChannelSettingsMenu(MuliTrack_Model &multiTrackModel,
                        std::function<void()> && onBackButtonClick)
    :    multitrack_model(multiTrackModel),
         channel_list()
    {
        {
            header.onBackClicked = [click = std::move(onBackButtonClick)] {
                click();
            };
            game_ui_header_update(&header, "Game Tracks Settings", "");
            addAndMakeVisible(header);
        }
        
        channel_list.selected_channel_changed_callback = [&](int){
        };
        
        channel_list.create_channel_callback = [&](juce::String new_channel_name){
            int new_channel_id = random_positive_int();
            Game_Channel new_channel = {
                .id = new_channel_id,
            };
            new_channel_name.copyToUTF8(new_channel.name, sizeof(new_channel.name));
            {
                auto [it, result] = multitrack_model.game_channels.emplace(new_channel_id, new_channel);
                assert(result);
            }
            multitrack_model.order.push_back(new_channel_id);
            multitrack_model_broadcast_change(&multitrack_model);
        };

        channel_list.delete_channel_callback = [&](int row_to_delete){
            auto id = multitrack_model.order.at(row_to_delete);
            multitrack_model.order.erase(multitrack_model.order.begin() + row_to_delete);
            {
                auto deleted_count = multitrack_model.game_channels.erase(id);
                assert(deleted_count == 1);
            }
            multitrack_model_broadcast_change(&multitrack_model);
        };

        channel_list.rename_channel_callback = [&](int row_idx, juce::String new_channel_name){
            int channel_id = multitrack_model.order.at(row_idx);
            auto& channel_to_rename = multitrack_model.game_channels.at(channel_id);
            new_channel_name.copyToUTF8(channel_to_rename.name, sizeof(channel_to_rename.name));
            multitrack_model_broadcast_change(&multitrack_model);
        };

        channel_list.customization_point = [&](int row_idx, List_Row_Label* label){
            int channel_id = multitrack_model.order.at(row_idx);
            bool only_one_daw_track = multitrack_model.assigned_daw_track_count.at(channel_id) == 1;
            juce::Colour colour = only_one_daw_track 
                ? juce::Colours::white
                : juce::Colours::red;
            label->setColour(juce::Label::textColourId, colour);
        };

        channel_list.insert_row_text = "Create new channel";
        
        auto multitrack_observer = [&](MuliTrack_Model *new_model) { 
            std::vector<juce::String> channel_names{};
            channel_names.resize(new_model->game_channels.size());
            auto projection = [new_model] (int id) {
                return new_model->game_channels.at(id).name;
            };
            std::transform(new_model->order.begin(), new_model->order.end(), channel_names.begin(), std::move(projection));
            channel_list.update(channel_names);
        };
        multitrack_observer(&multitrack_model);
        multitrack_model_add_observer(
            &multitrack_model,
            MultiTrack_Observers_Channel_Settings,
            std::move(multitrack_observer)
        );

        addAndMakeVisible(channel_list);
    }

    ~ChannelSettingsMenu()
    {
        multitrack_model_remove_observer(&multitrack_model, MultiTrack_Observers_Channel_Settings);
    }
    
    void paint(juce::Graphics &g) override
    {
        juce::ignoreUnused(g);
    }

    void resized() override
    {
        auto r = getLocalBounds();
            
        auto header_bounds = r.removeFromTop(header.getHeight());
        header.setBounds(header_bounds);

        channel_list.setBounds(r);
    }

    void update()
    {
        channel_list.list_comp.updateContent();
    }

private :
    MuliTrack_Model &multitrack_model;
    Insertable_List channel_list;
    GameUI_Header header;
};


class SettingsMenu : public juce::Component
{
public :
    SettingsMenu(std::function < void() > && onBackButtonClick,
                 Settings &mixerGameSettings) :
        back_button_callback(std::move(onBackButtonClick)),
        settings(mixerGameSettings)
    {
        back_button.setButtonText("back");
        back_button.onClick = [this] {
            back_button_callback();
        };
        addAndMakeVisible(back_button);

        slider.setSize(300, 50);
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setValue(settings.difficulty, juce::dontSendNotification);
        addAndMakeVisible(slider);
        slider.onValueChange = [this] {
            double new_value = slider.getValue();
            settings.difficulty = (float) new_value;
        };
    }
    
    void paint(juce::Graphics &g) override
    {
        juce::ignoreUnused(g);
    }

    void resized() override
    {
        auto top_height = 20;
        auto r = getLocalBounds();
            
        auto top_bounds = r.withHeight(top_height);
            
        auto back_button_bounds = top_bounds.withWidth(90);
        back_button.setBounds(back_button_bounds);

        slider.setCentrePosition(r.getCentre());
    }

private :
    juce::TextButton back_button;
    juce::Slider slider;
    std::function < void() > back_button_callback;
    Settings &settings;
};


class StatsMenu : public juce::Component
{
public :
    StatsMenu(std::function < void() > && onBackButtonClick,
              Stats mixerGameStats) :
        back_button_callback(std::move(onBackButtonClick)),
        stats(mixerGameStats)
    {
        back_button.setButtonText("back");
        back_button.onClick = [this] {
            back_button_callback();
        };
        addAndMakeVisible(back_button);

        test_stat_label.setText(juce::String(stats.total_score), juce::dontSendNotification);
        test_stat_label.setSize(300, 50);
        test_stat_label.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(test_stat_label);
    }
    
    void paint(juce::Graphics &g) override
    {
        juce::ignoreUnused(g);
    }

    void resized() override
    {
        auto top_height = 20;
        auto r = getLocalBounds();
            
        auto top_bounds = r.withHeight(top_height);
            
        auto backButtonBounds = top_bounds.withWidth(90);
        back_button.setBounds(backButtonBounds);
        
        test_stat_label.setCentrePosition(r.getCentre());
    }

private :
    juce::TextButton back_button;
    juce::Slider slider;
    juce::Label test_stat_label;
    std::function < void() > back_button_callback;
    Stats stats;
};