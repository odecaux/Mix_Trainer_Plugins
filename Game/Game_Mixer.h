struct MixerGameUI;


struct Game_Mixer_Effect_Transition {
    GameStep in_transition;
    GameStep out_transition;
};

struct Game_Mixer_Effect_DSP {
    std::unordered_map<int, Channel_DSP_State> dsp_states;
};

struct Game_Mixer_Effect_UI {
    juce::String header_text;
    int score; 
    FaderStep fader_step; 
    std::optional < std::unordered_map<int, int> > slider_pos_to_display;
    Mix mix;
    juce::String button_text;
    Event_Type button_event;
};


enum MixerGame_Variant
{
    MixerGame_Normal,
    MixerGame_Tries,
    MixerGame_Timer
};

struct MixerGame_Config 
{
    juce::String title;
    std::unordered_map<int, ChannelInfos> channel_infos;
    std::vector < double > db_slider_values;
    MixerGame_Variant variant;
    int listens;
    int timeout_ms;
};

struct MixerGame_State {
    GameStep step;
    Mix mix;
    int score;
    //mixer
    std::unordered_map < int, int > edited_slider_pos;
    std::unordered_map < int, int > target_slider_pos;

    MixerGame_Config config;
    int remaining_listens;
    bool can_still_listen;
    juce::int64 timestamp_start;
    juce::int64 current_timestamp;
};


struct Game_Mixer_Effects {
    int error;
    MixerGame_State new_state;
    std::optional < Game_Mixer_Effect_Transition> transition;
    std::optional < Game_Mixer_Effect_DSP> dsp;
    std::optional < Game_Mixer_Effect_UI > ui;
    bool quit;
};
using observer_t = std::function<void(const Game_Mixer_Effects &)>;

struct MixerGame_IO
{
    std::mutex mutex;
    Timer timer;
    std::vector<observer_t> observers;
    std::function < void() > on_quit;
    MixerGame_State game_state;
};

struct MixerGameUI;

void mixer_game_post_event(MixerGame_IO *io, Event event);
Game_Mixer_Effects mixer_game_update(MixerGame_State state, Event event);
void mixer_game_ui_transitions(MixerGameUI &ui, Game_Mixer_Effect_Transition transition);
void game_ui_update(const Game_Mixer_Effect_UI &new_ui, MixerGameUI &ui);
void mixer_game_add_observer(MixerGame_IO *io, observer_t new_observer);

MixerGame_State mixer_game_state_init(
    std::unordered_map<int, ChannelInfos> &channel_infos,
    MixerGame_Variant variant,
    int listens,
    int timeout_ms,
    std::vector<double> db_slider_values);

std::unique_ptr<MixerGame_IO> mixer_game_io_init(MixerGame_State state);


struct MixerGameUI : public juce::Component
{
    MixerGameUI(const std::unordered_map<int, ChannelInfos>& channelInfo,
                const std::vector<double> &SliderValuesdB,
                MixerGame_IO *gameIO) 
        :
        fader_row(faders),
        db_slider_values(SliderValuesdB),
        io(gameIO)
    {
        auto f =
            [io = io, &fader_row = fader_row, &db_slider_values = db_slider_values] (const auto &a)->std::pair < int, std::unique_ptr<FaderComponent> > {
            const int id = a.first;
            
            auto onFaderMoved = [id, io] (int new_pos) {
                Event event = {
                    .type = Event_Slider,
                    .id = id,
                    .value_i = new_pos
                };
                mixer_game_post_event(io, event);
            };

            auto new_fader = std::make_unique < FaderComponent > (
                db_slider_values,
                a.second.name,
                std::move(onFaderMoved)
            );
            fader_row.addAndMakeVisible(new_fader.get());
            return { id, std::move(new_fader)};
        };
        
        std::transform(channelInfo.begin(), channelInfo.end(), 
                       std::inserter(faders, faders.end()), 
                       f);
        fader_row.adjustWidth();
        fader_viewport.setScrollBarsShown(false, true);
        fader_viewport.setViewedComponent(&fader_row, false);

        bottom.onNextClicked = [io = io] (Event_Type e){
            Event event = {
                .type = e
            };
            
            mixer_game_post_event(io, event);
        };
        header.onBackClicked = [io = io] {
            Event event = {
                .type = Event_Click_Back
            };
            mixer_game_post_event(io, event);
        };
        bottom.onToggleClicked = [io = io] (bool a){
            Event event = {
                .type = Event_Toggle_Input_Target,
                .value_b = a
            };
            mixer_game_post_event(io, event);
        };
        addAndMakeVisible(header);
        addAndMakeVisible(fader_viewport);
        addAndMakeVisible(bottom);
    }

    void resized() override 
    {
        auto bounds = getLocalBounds();
        auto bottom_height = 50;
        auto header_height = 20;

        auto header_bounds = bounds.withHeight(header_height);
        auto game_bounds = bounds.withTrimmedBottom(bottom_height).withTrimmedTop(header_height);
        auto bottom_bounds = bounds.withTrimmedTop(bounds.getHeight() - bottom_height);
        
        header.setBounds(header_bounds);
        fader_viewport.setBounds(game_bounds);
        bottom.setBounds(bottom_bounds);

        fader_row.setSize(fader_row.getWidth(),
                          fader_viewport.getHeight() - fader_viewport.getScrollBarThickness());
    }

    //NOTE solution 1) keeping them in sync
    //solution 2) passing in the map and rebuilding the childs everytime

    void addChannel(int id, const juce::String name)
    {
        {
            auto assertChannel = faders.find(id);
            assert(assertChannel == faders.end());
        }

        auto onFaderMoved = [id, io = io] (int new_pos){
            Event event = {
                .type = Event_Slider,
                .id = id,
                .value_i = new_pos
            };
            mixer_game_post_event(io, event);
        };

        auto [it, result] = faders.emplace(id, std::make_unique < FaderComponent > (
            db_slider_values,
            name,
            std::move(onFaderMoved)
        ));
        assert(result);
        auto &new_fader = it->second;

        fader_row.addAndMakeVisible(new_fader.get());
        fader_row.adjustWidth();
        resized();
    }
    
    void removeChannel(int id)
    {
        const auto channel_to_remove = faders.find(id);
        assert(channel_to_remove != faders.end());
        fader_row.removeChildComponent(channel_to_remove->second.get());
        faders.erase(channel_to_remove);
        fader_row.adjustWidth();
        resized();
    }

    void changeChannelName(int id, const juce::String name)
    {
        faders[id]->setName(name);
    }

    std::unordered_map < int, std::unique_ptr<FaderComponent>> faders;
    FaderRowComponent fader_row;
    juce::Viewport fader_viewport;
    const std::vector < double > &db_slider_values;
    GameUI_Header header;
    GameUI_Bottom bottom;
    MixerGame_IO *io;
};
