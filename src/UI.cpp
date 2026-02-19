#include "UI.h"

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <thread>
#include <unordered_map>

// ============================================
// Configuration Implementation
// ============================================
namespace Configuration {
    std::string GetConfigPath() { return "Data\\SKSE\\Plugins\\ConsoleCommander.ini"; }

    void LoadConfiguration() {
        Commands.clear();
        std::string configPath = GetConfigPath();
        std::ifstream file(configPath);
        if (!file.is_open()) {
            logger::info("No configuration file found at {}", configPath);
            return;
        }
        std::string line;
        std::string currentSection = "";
        while (std::getline(file, line)) {
            line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1, std::string::npos);
            if (line.empty() || line[0] == ';' || line[0] == '#') continue;
            if (line[0] == '[' && line.back() == ']') {
                currentSection = line.substr(1, line.size() - 2);
                continue;
            }
            if (currentSection == "Delays") {
                size_t eqPos = line.find('=');
                if (eqPos != std::string::npos) {
                    std::string key = line.substr(0, eqPos);
                    std::string value = line.substr(eqPos + 1);
                    try {
                        uint32_t val = std::stoul(value);
                        if (key == "EscDelay")
                            EscDelay = val;
                        else if (key == "OpenConsoleDelay")
                            OpenConsoleDelay = val;
                        else if (key == "TypingStartDelay")
                            TypingStartDelay = val;
                        else if (key == "CharDelay")
                            CharDelay = val;
                        else if (key == "EnterDelay")
                            EnterDelay = val;
                        else if (key == "CloseConsoleDelay")
                            CloseConsoleDelay = val;
                        else if (key == "KeyboardLayout")
                            KeyboardLayout = val;
                    } catch (...) {
                        logger::warn("Invalid delay value in ini: {}", line);
                    }
                }
            } else if (currentSection == "ConsoleCommands") {
                size_t pos = line.find('|');
                if (pos != std::string::npos) {
                    std::string name = line.substr(0, pos);
                    std::string cmd = line.substr(pos + 1);
                    Commands.push_back(ConsoleCommand(name, cmd));
                }
            }
        }
        file.close();
        logger::info("Loaded {} commands from configuration", Commands.size());
        logger::info("Loaded delays: Esc={}, OpenConsole={}, TypingStart={}, Char={}, Enter={}, CloseConsole={}, KeyboardLayout={}", EscDelay, OpenConsoleDelay, TypingStartDelay, CharDelay, EnterDelay, CloseConsoleDelay, KeyboardLayout);
    }

    void SaveConfiguration() {
        std::string configPath = GetConfigPath();
        std::ofstream file(configPath, std::ios::trunc);
        if (!file.is_open()) {
            logger::error("Failed to save configuration to {}", configPath);
            return;
        }
        file << "; Console Commander Config\n\n";
        file << "[Delays]\n";
        file << "EscDelay=" << EscDelay << "\n";
        file << "OpenConsoleDelay=" << OpenConsoleDelay << "\n";
        file << "TypingStartDelay=" << TypingStartDelay << "\n";
        file << "CharDelay=" << CharDelay << "\n";
        file << "EnterDelay=" << EnterDelay << "\n";
        file << "CloseConsoleDelay=" << CloseConsoleDelay << "\n";
        file << "KeyboardLayout=" << KeyboardLayout << " ; 0 = QWERTY (default), 1 = AZERTY, 2 = QWERTZ\n\n";
        file << "[ConsoleCommands]\n";
        file << "; Format: Name|ConsoleCommand\n";
        for (const auto& cmd : Commands) {
            file << cmd.name << "|" << cmd.command << "\n";
        }
        file.close();
        logger::info("Saved {} commands to configuration", Commands.size());
    }

    void AddCommand(const ConsoleCommand& cmd) {
        Commands.push_back(cmd);
        SaveConfiguration();
    }

    void RemoveCommand(size_t index) {
        if (index < Commands.size()) {
            Commands.erase(Commands.begin() + index);
            SaveConfiguration();
        }
    }
}

// ============================================
// Key Executor Implementation
// ============================================
namespace KeyExecutor {
    void SendKey(uint32_t dxCode, bool down) {
        INPUT input = {0};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = 0;
        input.ki.wScan = static_cast<WORD>(dxCode);
        input.ki.dwFlags = KEYEVENTF_SCANCODE | (down ? 0 : KEYEVENTF_KEYUP);
        if (dxCode >= 0xE0 && dxCode <= 0xE7) {
            input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        }
        SendInput(1, &input, sizeof(INPUT));
    }

    // QWERTY scan codes
    static std::unordered_map<char, uint32_t> qwertyScan = {
        {'a', 30}, {'b', 48}, {'c', 46}, {'d', 32}, {'e', 18}, {'f', 33}, {'g', 34}, {'h', 35}, {'i', 23}, {'j', 36}, {'k', 37}, {'l', 38},  {'m', 50}, {'n', 49}, {'o', 24},  {'p', 25},
        {'q', 16}, {'r', 19}, {'s', 31}, {'t', 20}, {'u', 22}, {'v', 47}, {'w', 17}, {'x', 45}, {'y', 21}, {'z', 44}, {'0', 11}, {'1', 2},   {'2', 3},  {'3', 4},  {'4', 5},   {'5', 6},
        {'6', 7},  {'7', 8},  {'8', 9},  {'9', 10}, {' ', 57}, {'.', 52}, {',', 51}, {'/', 53}, {'-', 12}, {'=', 13}, {';', 39}, {'\'', 40}, {'[', 26}, {']', 27}, {'\\', 43}, {'`', 41}
    };

    // AZERTY scan codes
    static std::unordered_map<char, uint32_t> azertyScan = {
        {'a', 16}, {'b', 48}, {'c', 46}, {'d', 32}, {'e', 18}, {'f', 33}, {'g', 34}, {'h', 35}, {'i', 23}, {'j', 36}, {'k', 37}, {'l', 38},  {'m', 50}, {'n', 49}, {'o', 24},  {'p', 25},
        {'q', 30}, {'r', 19}, {'s', 31}, {'t', 20}, {'u', 22}, {'v', 47}, {'w', 44}, {'x', 45}, {'y', 21}, {'z', 17}, {'0', 11}, {'1', 2},   {'2', 3},  {'3', 4},  {'4', 5},   {'5', 6},
        {'6', 7},  {'7', 8},  {'8', 9},  {'9', 10}, {' ', 57}, {'.', 52}, {',', 51}, {'/', 53}, {'-', 12}, {'=', 13}, {';', 39}, {'\'', 40}, {'[', 26}, {']', 27}, {'\\', 43}, {'`', 41}
    };

    // QWERTZ scan codes
    static std::unordered_map<char, uint32_t> qwertzScan = {
        {'a', 30}, {'b', 48}, {'c', 46}, {'d', 32}, {'e', 18}, {'f', 33}, {'g', 34}, {'h', 35}, {'i', 23}, {'j', 36}, {'k', 37}, {'l', 38},  {'m', 50}, {'n', 49}, {'o', 24},  {'p', 25},
        {'q', 16}, {'r', 19}, {'s', 31}, {'t', 20}, {'u', 22}, {'v', 47}, {'w', 17}, {'x', 45}, {'y', 21}, {'z', 44}, {'0', 11}, {'1', 2},   {'2', 3},  {'3', 4},  {'4', 5},   {'5', 6},
        {'6', 7},  {'7', 8},  {'8', 9},  {'9', 10}, {' ', 57}, {'.', 52}, {',', 51}, {'/', 53}, {'-', 12}, {'=', 13}, {';', 39}, {'\'', 40}, {'[', 26}, {']', 27}, {'\\', 43}, {'`', 41}
    };

    void SendChar(char c) {
        bool isUpper = std::isupper(static_cast<unsigned char>(c));
        char lowerC = std::tolower(static_cast<unsigned char>(c));

        std::unordered_map<char, uint32_t>* scanMap;
        if (Configuration::KeyboardLayout == 1) {
            scanMap = &azertyScan;
        } else if (Configuration::KeyboardLayout == 2) {
            scanMap = &qwertzScan;
        } else {
            scanMap = &qwertyScan;
        }

        auto it = scanMap->find(lowerC);
        if (it == scanMap->end()) {
            logger::warn("Unsupported character: {}", c);
            return;
        }
        uint32_t scan = it->second;

        if (isUpper) {
            SendKey(42, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        SendKey(scan, true);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        SendKey(scan, false);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        if (isUpper) {
            SendKey(42, false);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void ExecuteCommand(const std::string& command) {
        logger::info("Executing command: {} (full delays used: Esc={}, OpenConsole={}, TypingStart={}, Char={}, Enter={}, CloseConsole={}, KeyboardLayout={})", command, Configuration::EscDelay, Configuration::OpenConsoleDelay,
                     Configuration::TypingStartDelay, Configuration::CharDelay, Configuration::EnterDelay, Configuration::CloseConsoleDelay, Configuration::KeyboardLayout);

        std::thread([command]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            std::this_thread::sleep_for(std::chrono::milliseconds(Configuration::EscDelay));
            SendKey(1, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            SendKey(1, false);

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            auto ui = RE::UI::GetSingleton();
            bool needsOpen = ui && !ui->IsMenuOpen(RE::Console::MENU_NAME);
            if (needsOpen) {
                std::this_thread::sleep_for(std::chrono::milliseconds(Configuration::OpenConsoleDelay));
                SendKey(41, true);
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                SendKey(41, false);
            } else {
                logger::info("Console already open - skipping open step");
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            std::this_thread::sleep_for(std::chrono::milliseconds(Configuration::TypingStartDelay));

            for (char c : command) {
                SendChar(c);
                std::this_thread::sleep_for(std::chrono::milliseconds(Configuration::CharDelay));
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(Configuration::EnterDelay));

            SendKey(28, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            SendKey(28, false);

            std::this_thread::sleep_for(std::chrono::milliseconds(Configuration::CloseConsoleDelay));

            SendKey(41, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            SendKey(41, false);
        }).detach();
    }
}

// ============================================
// UI Implementation
// ============================================
void UI::Register() {
    if (!SKSEMenuFramework::IsInstalled()) {
        logger::error("SKSE Menu Framework not installed!");
        return;
    }
    SKSEMenuFramework::SetSection("Console Commander");
    SKSEMenuFramework::AddSectionItem("Command Manager", ConsoleCommander::Render);
    ConsoleCommander::AddCommandWindow = SKSEMenuFramework::AddWindow(ConsoleCommander::RenderAddCommandWindow, true);
    logger::info("UI registered successfully");
}

namespace UI::ConsoleCommander {
    static char newCommandName[256] = "";
    static char newCommandText[1024] = "";

    void __stdcall Render() {
        ImGuiMCP::Text("Commands:");
        ImGuiMCP::SameLine();
        if (ImGuiMCP::Button("Add Command")) {
            newCommandName[0] = '\0';
            newCommandText[0] = '\0';
            AddCommandWindow->IsOpen = true;
        }
        ImGuiMCP::SameLine();
        if (ImGuiMCP::Button("Reload Config")) {
            Configuration::LoadConfiguration();
            logger::info("Configuration reloaded");
        }
        ImGuiMCP::SameLine();
        ImGuiMCP::Text("Search:");
        ImGuiMCP::SameLine();
        static char searchBuffer[256] = "";
        ImGuiMCP::InputText("##Search", searchBuffer, sizeof(searchBuffer));

        ImGuiMCP::Separator();

        if (Configuration::Commands.empty()) {
            ImGuiMCP::TextColored(ImGuiMCP::ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No commands configured. Click 'Add Command' to create one.");
        } else {
            static ImGuiMCP::ImGuiTableFlags flags = ImGuiMCP::ImGuiTableFlags_Borders | ImGuiMCP::ImGuiTableFlags_RowBg | ImGuiMCP::ImGuiTableFlags_ScrollY;
            if (ImGuiMCP::BeginTable("CommandTable", 3, flags)) {
                ImGuiMCP::TableSetupColumn("Name", ImGuiMCP::ImGuiTableColumnFlags_WidthStretch);
                ImGuiMCP::TableSetupColumn("Command", ImGuiMCP::ImGuiTableColumnFlags_WidthStretch);
                ImGuiMCP::TableSetupColumn("Actions", ImGuiMCP::ImGuiTableColumnFlags_WidthFixed, 180.0f);
                ImGuiMCP::TableHeadersRow();

                std::string lowerSearch = searchBuffer;
                std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), ::tolower);

                for (size_t i = 0; i < Configuration::Commands.size(); i++) {
                    const auto& cmd = Configuration::Commands[i];
                    std::string lowerName = cmd.name;
                    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
                    std::string lowerCommand = cmd.command;
                    std::transform(lowerCommand.begin(), lowerCommand.end(), lowerCommand.begin(), ::tolower);

                    if (!lowerSearch.empty() && lowerName.find(lowerSearch) == std::string::npos && lowerCommand.find(lowerSearch) == std::string::npos) {
                        continue;
                    }

                    ImGuiMCP::TableNextRow();
                    ImGuiMCP::TableSetColumnIndex(0);
                    ImGuiMCP::Text(cmd.name.c_str());
                    ImGuiMCP::TableSetColumnIndex(1);
                    ImGuiMCP::Text(cmd.command.c_str());
                    ImGuiMCP::TableSetColumnIndex(2);

                    if (ImGuiMCP::Button(("Execute##" + std::to_string(i)).c_str())) {
                        logger::info("Executing command: {} (full command: {})", cmd.name, cmd.command);
                        KeyExecutor::ExecuteCommand(cmd.command);
                    }

                    ImGuiMCP::SameLine();

                    if (ImGuiMCP::Button(("Delete##" + std::to_string(i)).c_str())) {
                        logger::info("Deleted command: {} (full command: {})", cmd.name, cmd.command);
                        Configuration::RemoveCommand(i);
                        break;
                    }
                }
                ImGuiMCP::EndTable();
            }
        }
    }

    void __stdcall RenderAddCommandWindow() {
        auto viewport = ImGuiMCP::GetMainViewport();
        ImGuiMCP::SetNextWindowPos(ImGuiMCP::ImVec2(viewport->Size.x * 0.5f, viewport->Size.y * 0.5f), ImGuiMCP::ImGuiCond_Appearing, ImGuiMCP::ImVec2(0.5f, 0.5f));
        ImGuiMCP::SetNextWindowSize(ImGuiMCP::ImVec2(700, 500), ImGuiMCP::ImGuiCond_Appearing);

        ImGuiMCP::Begin("Add Command##ConsoleCommander", nullptr, ImGuiMCP::ImGuiWindowFlags_NoCollapse);

        if (ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_Escape)) {
            AddCommandWindow->IsOpen = false;
        }

        ImGuiMCP::Text("Configure a new command:");
        ImGuiMCP::Separator();
        ImGuiMCP::Spacing();

        ImGuiMCP::InputText("Command Name", newCommandName, sizeof(newCommandName));
        ImGuiMCP::Spacing();

        ImGuiMCP::Text("Console Command:");
        ImGuiMCP::InputText("##CommandText", newCommandText, sizeof(newCommandText));

        ImGuiMCP::Spacing();
        ImGuiMCP::Separator();
        ImGuiMCP::Spacing();

        bool canAdd = strlen(newCommandName) > 0 && strlen(newCommandText) > 0;
        if (!canAdd) {
            ImGuiMCP::PushStyleVar(ImGuiMCP::ImGuiStyleVar_Alpha, 0.5f);
        }

        if (ImGuiMCP::Button("Add Command") && canAdd) {
            Configuration::ConsoleCommand newCmd(newCommandName, newCommandText);
            logger::info("Added new command: {} (full command: {})", newCmd.name, newCmd.command);
            Configuration::AddCommand(newCmd);
            AddCommandWindow->IsOpen = false;
        }

        if (!canAdd) {
            ImGuiMCP::PopStyleVar();
        }

        ImGuiMCP::SameLine();

        if (ImGuiMCP::Button("Cancel")) {
            AddCommandWindow->IsOpen = false;
        }

        ImGuiMCP::End();
    }
}