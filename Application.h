

class ProcessorHost;
class EditorHost;

class Application
{
public:
    Application(ProcessorHost &host);

    void initialiseEditorUI(EditorHost *new_editor);
    void onEditorDelete();
    void toMainMenu();
    void toGame();
    void toStats();
    void toSettings();
    
    //TODO rename
    static std::unordered_map < int, ChannelDSPState > bypassedAllChannelsDSP(const std::unordered_map<int, ChannelInfos> &channels) {
        std::unordered_map < int, ChannelDSPState > dsp_states;
        
        std::transform(channels.begin(), channels.end(), 
                       std::inserter(dsp_states, dsp_states.end()), 
                       [](const auto &a) -> std::pair<int, ChannelDSPState>{
                       return { a.first, ChannelDSP_on() };
        });
        return dsp_states;
    }

    void broadcastDSP(const std::unordered_map < int, ChannelDSPState > &dsp_states);

    void createChannel(int id);
    void deleteChannel(int id);
    void renameChannelFromUI(int id, juce::String newName);
    void renameChannelFromTrack(int id, const juce::String &new_name);
    void changeFrequencyRange(int id, float new_min, float new_max);
 

private:
    enum class PanelType{
        MainMenu,
        Game,
        Settings,
        Stats
    };
    
    std::unique_ptr < Game > game;
    std::unordered_map<int, ChannelInfos> channels;
    Settings settings = { 0.0f };
    Stats stats = { 0 };
    PanelType type;
    ProcessorHost &host;
    EditorHost *editor;
};