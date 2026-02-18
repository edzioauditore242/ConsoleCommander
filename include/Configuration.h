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

    void LoadConfiguration();
    void SaveConfiguration();
    void AddCommand(const ConsoleCommand& cmd);
    void RemoveCommand(size_t index);
}