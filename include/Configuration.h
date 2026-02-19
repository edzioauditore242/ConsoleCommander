#pragma once
#include <string>
#include <vector>
namespace Configuration {
    struct ConsoleCommand {
        std::string name;
        std::string command;
        ConsoleCommand() : name(""), command("") {}
        ConsoleCommand(const std::string& n, const std::string& cmd) : name(n), command(cmd) {}
    };
    inline std::vector<ConsoleCommand> Commands;
    // Adjustable delays (in ms) - loaded from ini
    inline uint32_t EscDelay = 100;
    inline uint32_t OpenConsoleDelay = 200;
    inline uint32_t TypingStartDelay = 150;
    inline uint32_t CharDelay = 30;
    inline uint32_t EnterDelay = 200;
    inline uint32_t CloseConsoleDelay = 50;
    inline uint32_t KeyboardLayout = 0;  // 0 = QWERTY, 1 = AZERTY
    void LoadConfiguration();
    void SaveConfiguration();
    void AddCommand(const ConsoleCommand& cmd);
    void RemoveCommand(size_t index);
}