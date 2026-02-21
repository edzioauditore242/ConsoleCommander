#pragma once
#include <string>
#include <vector>

namespace Configuration {
    struct ConsoleCommand {
        std::string name;
        std::string command;
        bool closeConsole = true;  // default yes (1)
        bool isCustom = false;     // true if from custom .ini
        bool isHidden = false;     // true if name starts with {Hidden}
        std::string sourcePath;    // full path to custom .ini (empty for main)

        ConsoleCommand() = default;
        ConsoleCommand(const std::string& n, const std::string& cmd, bool close = true, bool custom = false, bool hidden = false, std::string path = "")
            : name(n), command(cmd), closeConsole(close), isCustom(custom), isHidden(hidden), sourcePath(path) {}
    };

    inline std::vector<ConsoleCommand> Commands;
    inline bool ShowHiddenGlobal = false;

    // Adjustable delays (in ms) - loaded from ini
    inline uint32_t EscDelay = 100;
    inline uint32_t OpenConsoleDelay = 200;
    inline uint32_t TypingStartDelay = 150;
    inline uint32_t CharDelay = 30;
    inline uint32_t EnterDelay = 200;
    inline uint32_t CloseConsoleDelay = 50;

    void LoadConfiguration();
    void SaveConfiguration();
    void AddCommand(const ConsoleCommand& cmd);
    void RemoveCommand(size_t index);
    void ToggleHideCommand(size_t index);
}