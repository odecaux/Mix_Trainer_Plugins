
class MainMenu : public juce::Component
{
public :
    MainMenu(std::function<void()> &&toGameButtonClick,
             std::function<void()> &&toStatsButtonClick,
             std::function<void()> &&toSettingsButtonClick)
    {
        game_button.setSize(100, 40);
        game_button.setButtonText("Game");
        game_button.onClick = [click = std::move(toGameButtonClick)] {
            click();
        };
        addAndMakeVisible(game_button);
        
        stats_button.setSize(100, 40);
        stats_button.setButtonText("Statistics");
        stats_button.onClick = [click = std::move(toStatsButtonClick)] {
            click();
        };
        addAndMakeVisible(stats_button);
        
        settings_button.setSize(100, 40);
        settings_button.setButtonText("Settings");
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
        game_button.setCentrePosition(bounds.getCentre() + juce::Point<int>(0, -50));
        stats_button.setCentrePosition(bounds.getCentre());
        settings_button.setCentrePosition(bounds.getCentre() + juce::Point<int>(0, 50));
    }

private :
    juce::TextButton game_button;
    juce::TextButton stats_button;
    juce::TextButton settings_button;
};

class SettingsMenu : public juce::Component
{
public :
    SettingsMenu(std::function < void() > && onBackButtonClick,
                 Settings &settings) :
        onBackButtonClick(std::move(onBackButtonClick)),
        settings(settings)
    {
        back_button.setButtonText("back");
        back_button.onClick = [this] {
            this->onBackButtonClick();
        };
        addAndMakeVisible(back_button);

        slider.setSize(300, 50);
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setValue(settings.difficulty, juce::dontSendNotification);
        addAndMakeVisible(slider);
        slider.onValueChange = [this] {
            double new_value = slider.getValue();
            this->settings.difficulty = (float) new_value;
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
            
        auto backButtonBounds = top_bounds.withWidth(90);
        back_button.setBounds(backButtonBounds);

        slider.setCentrePosition(r.getCentre());
    }

private :
    juce::TextButton back_button;
    juce::Slider slider;
    std::function < void() > onBackButtonClick;
    Settings &settings;
};


class StatsMenu : public juce::Component
{
public :
    StatsMenu(std::function < void() > && onBackButtonClick,
              Stats stats) :
        onBackButtonClick(std::move(onBackButtonClick)),
        stats(stats)
    {
        back_button.setButtonText("back");
        back_button.onClick = [this] {
            this->onBackButtonClick();
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
    std::function < void() > onBackButtonClick;
    Stats stats;
};