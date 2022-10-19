
struct Settings{
    float difficulty;
};

struct Stats {
    int total_score;
};

class ProcessorHost;
class EditorHost;

class Application
{
public:
    Application(ProcessorHost &host);

    void initialiseEditorUI(EditorHost *new_editor);
    void onEditorDelete();
    void toMainMenu();
    void toSettings();

    /*
    void broadcastDSP()
    {
        for(const auto& [_, channel] : state.channels)
        {
            ChannelDSPState dsp;
            switch(state.step)
            {
                case Begin : {
                    dsp = { .gain = channel.edited_gain };
                } break;
                case Listening : {
                    dsp = { .gain = channel.target_gain };
                } break;
                case Editing : {
                    dsp = { .gain = channel.edited_gain };
                } break;
                case ShowingTruth : { 
                    dsp = { .gain = channel.target_gain };
                } break;
                case ShowingAnswer : {
                    dsp = { .gain = channel.edited_gain };
                } break;
                default : {
                    dsp = { .gain = 0.0 };
                    jassertfalse;
                } break;
            }
        }
        
        host.sendDSPMessage(channel.id, dsp);
    }
    
        std::unique_ptr<GameImplementation> game;
    */

#if 0
    void createChannel(int id);
    void deleteChannel(int id);
    void renameChannelFromUI(int id, juce::String newName);
    void renameChannelFromTrack(int id, const juce::String &new_name);
    void changeFrequencyRange(int id, float new_min, float new_max);
#endif 

private:
    enum class PanelType{
        MainMenu,
        Game,
        Settings
    };
#if 0
    GameState state;
#endif
    Settings settings;
    Stats stats;
    PanelType type;
    ProcessorHost &host;
    EditorHost *editor;
};