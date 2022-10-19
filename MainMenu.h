
class MainMenu : public juce::Component
{
public :
    MainMenu(std::function<void()> &&onMainButtonClick) : 
        onMainButtonClick(std::move(onMainButtonClick))
    {
        button.setSize(100, 40);
        button.setButtonText("Settings");
        button.onClick = [this] {
            this->onMainButtonClick();
        };
        addAndMakeVisible(button);
    }
    
    void paint(juce::Graphics &g) override
    {
        juce::ignoreUnused(g);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        button.setCentrePosition(bounds.getCentre());
    }

private :
    juce::TextButton button;
    std::function < void() > onMainButtonClick;
};

class SettingsMenu : public juce::Component
{
public :
    SettingsMenu(std::function < void() > && onBackButtonClick,
                 std::function < void(double) > && onSliderValueChanged) :
        onBackButtonClick(std::move(onBackButtonClick)),
        onSliderValueChanged(std::move(onSliderValueChanged))
    {
        back_button.setButtonText("back");
        back_button.onClick = [this] {
            this->onBackButtonClick();
        };
        addAndMakeVisible(back_button);

        slider.setSize(300, 50);
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        addAndMakeVisible(slider);
        slider.onValueChange = [this] {
            double new_value = slider.getValue();
            this->onSliderValueChanged(new_value);
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
    std::function < void(double) > onSliderValueChanged;
};