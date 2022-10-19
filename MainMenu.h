
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
    SettingsMenu(std::function<void()> &&onBackButtonClick) : 
        onBackButtonClick(std::move(onBackButtonClick))
    {
        button.setSize(100, 40);
        button.setButtonText("back");
        button.onClick = [this] {
            this->onBackButtonClick();
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
    std::function < void() > onBackButtonClick;
};