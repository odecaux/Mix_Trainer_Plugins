
class MainMenu : public juce::Component
{
public :
    MainMenu(std::function<void()> &&toNormalGameButtonClick,
             std::function<void()> &&toTimerGameButtonClick,
             std::function<void()> &&toTriesGameButtonClick,
             std::function<void()> &&toChannelSettingsButtonClick,
             std::function<void()> &&toStatsButtonClick,
             std::function<void()> &&toSettingsButtonClick)
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
};

class Channel_List : public juce::Component,
                     public juce::ListBoxModel
{
public: 
    Channel_List(MuliTrack_Model &multitrackModel)
    : model(multitrackModel)
    {
        list_comp.setModel(this);
        list_comp.setMultipleSelectionEnabled(false);
        addAndMakeVisible(list_comp);

        editable_mouse_down_callback = [&] (int row_idx) {
            if(row_idx == -1)
                row_idx = checked_cast<int>(model.game_channels.size());
            list_comp.selectRow(row_idx);
        };
        editable_text_changed_callback = [&] (int row_idx, juce::String row_text) {
            assert(row_idx <= model.game_channels.size());
            assert(row_idx >= 0);
            int channel_id = model.order[row_idx];
            model.game_channels[channel_id].name = row_text;
            channel_list_changed_callback();
        };
        editable_create_row_callback = [&] (juce::String new_row_text) {
            int new_channel_id = random_positive_int();
            Game_Channel new_channel = {
                .id = new_channel_id,
                .name = new_row_text,
            };
            {
                auto [it, result] = model.game_channels.emplace(new_channel_id, new_channel);
                assert(result);
            }
            model.order.push_back(new_channel_id);
            channel_list_changed_callback();
            list_comp.updateContent();
        };
    }

    
    virtual ~Channel_List() override = default;

    void paintOverChildren(juce::Graphics& g) override
    {
        if (getNumRows() == 0)
        {
            auto r = getLocalBounds();
            g.setColour(juce::Colours::white);
            g.drawText("Create game track", r.toFloat(), juce::Justification::centred);
        }
    }

    void resized() override
    {
        list_comp.setBounds(getLocalBounds());
    }

    int getNumRows() override { return static_cast<int>(model.game_channels.size()) + 1; }

    void paintListBoxItem (int row,
                           juce::Graphics& g,
                           int width, int height,
                           bool rowIsSelected) override
    {
        if (rowIsSelected && row < getNumRows())
        {
            g.setColour(juce::Colours::white);
            auto bounds = juce::Rectangle { 0, 0, width, height };
            g.drawRect(bounds);
        }
    }

    void deleteKeyPressed (int) override
    {
        auto selected_row = list_comp.getSelectedRow();
        if(selected_row == -1) 
            return;
        auto id = model.order[selected_row];
        model.order.erase(model.order.begin() + selected_row);
        {
            auto deleted_count = model.game_channels.erase(id);
            assert(deleted_count == 1);
        }
#if 0
        {
            auto deleted_count = model.assigned_daw_track_count .erase(id);
            assert(deleted_count == 1);
        }
#endif
        auto row_to_select = selected_row == 0 ? 0 : selected_row - 1;
        list_comp.selectRow(row_to_select);
        list_comp.updateContent();
        selected_channel_changed_callback(row_to_select);
        channel_list_changed_callback();
    }

    
    juce::Component *refreshComponentForRow (int row_number,
                                             bool,
                                             Component *existing_component) override
    {
        //assert (existingComponentToUpdate == nullptr || dynamic_cast<EditableTextCustomComponent*> (existingComponentToUpdate) != nullptr);
        //unused row            
        assert(model.order.size() == model.game_channels.size());

        if (row_number > model.game_channels.size())
        {
            if (existing_component != nullptr)
                delete existing_component;
            return nullptr;
        }
        else
        {
            List_Row_Label *label;

            assert(model.order.size() == model.game_channels.size());
            if (existing_component != nullptr)
            {
                label = dynamic_cast<List_Row_Label*>(existing_component);
                assert(label != nullptr);
            }
            else
            {
                label = new List_Row_Label("Insert new config",
                                                   editable_mouse_down_callback,
                                                   editable_text_changed_callback,
                                                   editable_create_row_callback);
            }
            juce::String row_text = "";
            if (row_number < model.game_channels.size())
            {
                int channel_id = model.order[row_number];
                row_text = model.game_channels[channel_id].name;
            }
            label->update(row_number, row_text, row_number == model.game_channels.size());
            return label;
        } 
    }
    
    std::function < void(int) > editable_mouse_down_callback;
    std::function < void(int, juce::String) > editable_text_changed_callback;
    std::function < void(juce::String) > editable_create_row_callback;

    std::function < void(int) > selected_channel_changed_callback = {};
    std::function < void() > channel_list_changed_callback = {};
    juce::ListBox list_comp;
private:
    MuliTrack_Model &model;

};

class ChannelSettingsMenu : public juce::Component
{
public :
    ChannelSettingsMenu(MuliTrack_Model &multiTrackModel,
                        std::function<void()> && onBackButtonClick)
    :    multitrack_model(multiTrackModel),
         channel_list(multiTrackModel)
    {
        {
            header.onBackClicked = [click = std::move(onBackButtonClick)] {
                click();
            };
            game_ui_header_update(&header, "Game Tracks Settings", "");
            addAndMakeVisible(header);
        }

        channel_list.selected_channel_changed_callback = [&] (int) {

        };
        channel_list.channel_list_changed_callback = [&] {
            multitrack_model_broadcast_change(&multitrack_model);
            selected_channel_changed_callback(); //TODO supprimer celui-là, on va juste mettre un observer à la place
        };
        addAndMakeVisible(channel_list);
    }
    
    void paint(juce::Graphics &g) override
    {
        juce::ignoreUnused(g);
    }

    void resized() override
    {
        auto top_height = 20;
        auto r = getLocalBounds();
            
        auto header_bounds = r.removeFromTop(top_height);
        header.setBounds(header_bounds);

        channel_list.setBounds(r);
    }

    void update()
    {
        channel_list.list_comp.updateContent();
    }

    std::function<void()> selected_channel_changed_callback;
private :
    MuliTrack_Model &multitrack_model;
    Channel_List channel_list;
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