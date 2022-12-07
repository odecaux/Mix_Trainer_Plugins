
class MainMenu : public juce::Component
{
public :
    MainMenu(std::function<void()> &&toNormalGameButtonClick,
             std::function<void()> &&toTimerGameButtonClick,
             std::function<void()> &&toTriesGameButtonClick,
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
        game_normal_button.setCentrePosition(bounds.getCentre() + juce::Point<int>(0, -100));
        game_timer_button.setCentrePosition(bounds.getCentre() + juce::Point<int>(0, -50));
        game_tries_button.setCentrePosition(bounds.getCentre());
        stats_button.setCentrePosition(bounds.getCentre() + juce::Point<int>(0, 50));
        settings_button.setCentrePosition(bounds.getCentre() + juce::Point<int>(0, 100));
    }

private :
    juce::TextButton game_normal_button;
    juce::TextButton game_timer_button;
    juce::TextButton game_tries_button;
    juce::TextButton stats_button;
    juce::TextButton settings_button;
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