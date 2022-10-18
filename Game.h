
static const double slider_values[] = {-100, -12, -9, -6, -3};

#define ArraySize(array) (sizeof((array)) / sizeof(*(array)))

static double equal_double(double a, double b, double theta)
{
    return std::abs(a - b) < theta;
}

static double slider_value_to_db(int value)
{
    jassert(value < ArraySize(slider_values));
    return slider_values[value];
}


static int db_to_slider_value(double db)
{
    for(int i = 0; i < ArraySize(slider_values); i++)
    {
        if(equal_double(db, slider_values[i], 0.001)) return i;
    }
    jassertfalse;
    return -1;
}

static int gain_to_slider_value(double gain)
{ 
    return db_to_slider_value(juce::Decibels::gainToDecibels(gain));
}

static double slider_value_to_gain(int value)
{
    return juce::Decibels::decibelsToGain(slider_value_to_db(value));
}

enum Action {
    
};

enum GameStep {
    Begin,
    Listening,
    Editing,
    ShowingTruth,
    ShowingAnswer
};

struct ChannelState
{
    int id;
    juce::String name;
    double edited_gain;
    double target_gain;
    float minFreq;
    float maxFreq;
    //- unused 
    double target_low_shelf_gain;
    double target_low_shelf_freq;
    double target_high_shelf_gain;
    double target_high_shelf_freq;
    
    double edited_low_shelf_gain;
    double edited_low_shelf_freq;
    double edited_high_shelf_gain;
    double edited_high_shelf_freq;
};

struct GameImplementation;

struct GameState
{
    std::unordered_map<int, ChannelState> channels;
    GameStep step;
    int score;
    GameImplementation *impl;
};

class ProcessorHost;

struct UIImplementation : public juce::Component
{
    UIImplementation(std::function<void()>&& onNextClicked,
                     std::function<void(bool)> && onToggleClicked,
                     std::function<void()> && onBackClicked);
    virtual ~UIImplementation() {}
    void updateGameUI_Generic(const GameState &state);
    void paint(juce::Graphics& g) override;
    void resized() override;

    virtual void resized_child(juce::Rectangle<int> bounds) = 0;
    virtual void createChannelUI(const ChannelState& channel, GameStep step) = 0;
    virtual void deleteChannelUI(int id) = 0;
    virtual void renameChannel(int id, const juce::String &newName) = 0;
    virtual void updateGameUI(const GameState &state) = 0;
            
    juce::Label topLabel;
    juce::Label scoreLabel;
    juce::TextButton backButton;
            
    juce::TextButton nextButton;
    juce::ToggleButton targetMixButton;
    juce::ToggleButton userMixButton;
            
    std::function<void()> onNextClicked;
    std::function<void(bool)> onToggleClicked;
    std::function<void()> onBackClicked;
};

struct GameImplementation {
    ProcessorHost &audioProcessor;
    GameState &state;
    std::unique_ptr<UIImplementation> game_ui = nullptr;
    
    GameImplementation(ProcessorHost &audioProcessor, GameState &state) 
    : audioProcessor(audioProcessor), state(state)
    {}
    virtual ~GameImplementation() {}

    virtual ChannelState generateChannel(int id) = 0;
    virtual int awardPoints() = 0;
    virtual void generateNewRound() = 0;

    virtual UIImplementation *createUI() = 0;

    void deleteUI() {
        jassert(game_ui != nullptr);
        game_ui.reset();
    }
    
    void createChannel(int id);
    void deleteChannel(int id);
    void renameChannelFromUI(int id, juce::String newName);
    void renameChannelFromTrack(int id, const juce::String &new_name);
    void changeFrequencyRange(int id, float new_min, float new_max);
    void toggleInputOrTarget(bool isOn);
    void nextClicked();
    void backClicked();
};


class DecibelSlider : public juce::Slider
{
public:
    DecibelSlider() = default;
    
    juce::String getTextFromValue(double value) override
    {
        return juce::Decibels::toString(slider_value_to_db((int)value));
    }
    
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DecibelSlider)
};

class FaderComponent : public juce::Component
{
public:
    FaderComponent(const ChannelState &state,
                   GameStep step,
                   std::function<void(double)> onFaderChange,
                   std::function<void(const juce::String&)> onEditedName)
    : onFaderChange(onFaderChange),
        onEditedName(onEditedName)
    {
        label.setText(state.name, juce::NotificationType::dontSendNotification);
        label.setEditable(true);
        label.onTextChange = [this] {
            this->onEditedName(label.getText());
        };
        label.onEditorShow = [&] {
            auto *editor = label.getCurrentTextEditor();
            jassert(editor != nullptr);
            editor->setJustification(juce::Justification::centred);
        };
        addAndMakeVisible(label);
        
        fader.setSliderStyle(juce::Slider::SliderStyle::LinearBarVertical);
        fader.setTextBoxStyle(juce::Slider::TextBoxAbove, true, getWidth(), 20);
        fader.setRange(0.0f, (double)(ArraySize(slider_values) - 1), 1.0);
        fader.setScrollWheelEnabled(false);
        
        fader.onValueChange = [this] {
            jassert(this->step == Editing);
            auto newGain = slider_value_to_gain((int)fader.getValue());
            this->onFaderChange(newGain);
        };
        addAndMakeVisible(fader);
        
        updateStep(step, state);
    }
    
    
    void resized() override
    {
        auto r = getLocalBounds();
        auto labelBounds = r.withBottom(50);
        label.setBounds(labelBounds);
        
        auto faderBounds = r.withTrimmedTop(50);
        fader.setBounds(faderBounds);
    }
    
    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds();
        g.setColour(juce::Colours::white);
        g.drawRoundedRectangle(r.toFloat(), 5.0f, 2.0f);
    }
    
    void setName(const juce::String& newName)
    {
        label.setText(newName, juce::dontSendNotification);
    }
    
    void updateStep(GameStep newStep, const ChannelState &state)
    {
        switch(newStep)
        {
            case Begin :
            {
                fader.setValue(gain_to_slider_value(state.edited_gain), juce::dontSendNotification);
                fader.setEnabled(true);
                fader.setVisible(true);
            } break;
            case Listening :
            {
                fader.setValue(gain_to_slider_value(state.edited_gain), juce::dontSendNotification);
                fader.setEnabled(false);
                fader.setVisible(false);
            }break;
            case Editing :
            {
                fader.setValue(gain_to_slider_value(state.edited_gain), juce::dontSendNotification);
                fader.setEnabled(true);
                fader.setVisible(true);
            }break;
            case ShowingTruth :
            {
                fader.setValue(gain_to_slider_value(state.target_gain), juce::dontSendNotification);
                fader.setEnabled(false);
                fader.setVisible(true);
            }break;
            case ShowingAnswer : 
            {
                fader.setValue(gain_to_slider_value(state.edited_gain), juce::dontSendNotification);
                fader.setEnabled(false);
                fader.setVisible(true);
            }break;
        };

        step = newStep;
        repaint();
    }
    
private:
    juce::Label label;
    DecibelSlider fader;
    std::function<void(double)> onFaderChange;
    std::function<void(const juce::String&)> onEditedName;
    GameStep step;
    //double targetValue;
    //double smoothing;
};

class FaderRowComponent : public juce::Component
{
public:
    FaderRowComponent(std::unordered_map<int, std::unique_ptr<FaderComponent>>& faderComponents)
    : faderComponents(faderComponents)
    {
    }
    
    void adjustWidth() 
    {
        setSize(fader_width * (int)faderComponents.size(), getHeight());
    }
    
    //==============================================================================
    
    void paint(juce::Graphics& g) override
    {
        juce::ignoreUnused(g);
    }
    
    
    void resized() override
    {
        int x_offset = 0;
        auto bounds = getLocalBounds();
        auto topLeft = bounds.getTopLeft();
        //draw in order
        for (auto& [_, fader] : faderComponents)
        {
            fader->setTopLeftPosition(topLeft + juce::Point<int>(x_offset, 0));
            fader->setSize(fader_width, bounds.getHeight());
            x_offset += fader_width;
        }
        adjustWidth();
    }
    
private:
    const static int fader_width = 60;
    std::unordered_map<int, std::unique_ptr<FaderComponent>> &faderComponents;
};


struct MixerUI : public UIImplementation
{
    MixerUI(GameState &state,
               std::function<void()> &&onNextClicked,
               std::function<void(bool)> &&onToggleClicked,
               std::function<void()> &&onBackClicked,
               std::function<void(int, double)> &&onFaderMoved,
               std::function<void(int, const juce::String&)> && onEditedName);
    virtual ~MixerUI() {}
    
    void resized_child(juce::Rectangle<int> bounds) override;
    void createChannelUI(const ChannelState& channel, GameStep step) override;
    void deleteChannelUI(int id) override;
    void renameChannel(int id, const juce::String &newName) override;
    void updateGameUI(const GameState &new_state) override;
    
    std::unordered_map<int, std::unique_ptr<FaderComponent>> faderComponents = {};
    FaderRowComponent fadersRow;
    juce::Viewport fadersViewport;
    std::function<void(int, double)> onFaderMoved;
    std::function<void(int, const juce::String&)> onEditedName;
};


struct MixerGame : public GameImplementation {
    MixerGame(ProcessorHost &audioProcessor, GameState &state);
    virtual ~MixerGame() {} 

    ChannelState generateChannel(int id) override;
    int awardPoints() override;
    void generateNewRound() override;
    UIImplementation *createUI() override;
    
    void editGain(int id, double new_gain);
};
