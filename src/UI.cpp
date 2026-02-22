#include "UI.h"

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// ============================================
// Configuration Implementation (unchanged)
// ============================================
namespace Configuration {
    std::string GetConfigPath() { return "Data\\SKSE\\Plugins\\ConsoleCommander.ini"; }

    void LoadConfiguration() {
        Commands.clear();
        std::string configPath = GetConfigPath();
        std::ifstream file(configPath);
        int mainLoaded = 0;
        if (!file.is_open()) {
            logger::info("No configuration file found at {}", configPath);
        } else {
            logger::info("Opened main configuration file: {}", configPath);
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
                                continue;
                        } catch (...) {
                            logger::warn("Invalid delay value in ini: {}", line);
                        }
                    }
                } else if (currentSection == "ConsoleCommands") {
                    size_t pos1 = line.find('|');
                    if (pos1 != std::string::npos) {
                        std::string name = line.substr(0, pos1);
                        size_t pos2 = line.find('|', pos1 + 1);
                        std::string cmdStr;
                        bool close = true;
                        std::string tooltipStr;
                        if (pos2 != std::string::npos) {
                            cmdStr = line.substr(pos1 + 1, pos2 - pos1 - 1);
                            size_t pos3 = line.find('|', pos2 + 1);
                            if (pos3 != std::string::npos) {
                                std::string closeStr = line.substr(pos2 + 1, pos3 - pos2 - 1);
                                tooltipStr = line.substr(pos3 + 1);
                                if (!closeStr.empty()) {
                                    try {
                                        close = (std::stoi(closeStr) != 0);
                                    } catch (...) {
                                    }
                                }
                            } else {
                                std::string closeStr = line.substr(pos2 + 1);
                                if (!closeStr.empty()) {
                                    try {
                                        close = (std::stoi(closeStr) != 0);
                                    } catch (...) {
                                    }
                                }
                            }
                        } else {
                            cmdStr = line.substr(pos1 + 1);
                        }
                        bool hidden = (name.find("{Hidden}") == 0);
                        Commands.push_back(ConsoleCommand(name, cmdStr, close, false, hidden, "", tooltipStr));
                        mainLoaded++;
                    }
                }
            }
            file.close();
            logger::info("Loaded {} commands from main ini", mainLoaded);
        }

        // Scan for custom .ini files (unchanged)
        std::string customFolder = "Data\\SKSE\\Plugins\\ConsoleCommander";
        logger::info("Checking custom folder: {}", customFolder);
        int totalCustom = 0;
        if (std::filesystem::exists(customFolder)) {
            logger::info("Custom folder exists: {}", customFolder);
            for (const auto& entry : std::filesystem::directory_iterator(customFolder)) {
                std::string path = entry.path().string();
                std::string filename = entry.path().filename().string();
                logger::info("Found file in custom folder: {}", filename);
                if (entry.is_regular_file() && entry.path().extension() == ".ini" && filename.find("ConsoleCommander_") == 0) {
                    logger::info("Loading custom .ini: {}", path);
                    std::ifstream customFile(path);
                    int customInThisFile = 0;
                    if (customFile.is_open()) {
                        std::string line;
                        std::string currentSection = "";
                        while (std::getline(customFile, line)) {
                            line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
                            line.erase(0, line.find_first_not_of(" \t"));
                            line.erase(line.find_last_not_of(" \t") + 1, std::string::npos);
                            if (line.empty() || line[0] == ';' || line[0] == '#') continue;
                            if (line[0] == '[' && line.back() == ']') {
                                currentSection = line.substr(1, line.size() - 2);
                                continue;
                            }
                            if (currentSection == "ConsoleCommands") {
                                size_t pos1 = line.find('|');
                                if (pos1 != std::string::npos) {
                                    std::string name = line.substr(0, pos1);
                                    size_t pos2 = line.find('|', pos1 + 1);
                                    std::string cmd;
                                    bool close = true;
                                    std::string tooltipStr;
                                    if (pos2 != std::string::npos) {
                                        cmd = line.substr(pos1 + 1, pos2 - pos1 - 1);
                                        size_t pos3 = line.find('|', pos2 + 1);
                                        if (pos3 != std::string::npos) {
                                            std::string closeStr = line.substr(pos2 + 1, pos3 - pos2 - 1);
                                            tooltipStr = line.substr(pos3 + 1);
                                            if (!closeStr.empty()) {
                                                try {
                                                    close = (std::stoi(closeStr) != 0);
                                                } catch (...) {
                                                }
                                            }
                                        } else {
                                            std::string closeStr = line.substr(pos2 + 1);
                                            if (!closeStr.empty()) {
                                                try {
                                                    close = (std::stoi(closeStr) != 0);
                                                } catch (...) {
                                                }
                                            }
                                        }
                                    } else {
                                        cmd = line.substr(pos1 + 1);
                                    }
                                    bool hidden = (name.find("{Hidden}") == 0);
                                    Commands.push_back(ConsoleCommand(name, cmd, close, true, hidden, path, tooltipStr));
                                    customInThisFile++;
                                    totalCustom++;
                                }
                            }
                        }
                        customFile.close();
                    }
                    logger::info("Loaded {} commands from {}", customInThisFile, filename);
                }
            }
        } else {
            logger::info("Custom folder does not exist: {}", customFolder);
        }

        logger::info("Total commands loaded: {} (main + custom)", Commands.size());
        logger::info("Loaded delays: Esc={}, OpenConsole={}, TypingStart={}, Char={}, Enter={}, CloseConsole={}", EscDelay, OpenConsoleDelay, TypingStartDelay, CharDelay, EnterDelay, CloseConsoleDelay);
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
        file << "CloseConsoleDelay=" << CloseConsoleDelay << "\n\n";
        file << "[ConsoleCommands]\n";
        file << "; Format: Name|Command1,Command2,...|CloseConsole (1=yes, 0=no)|Tooltip (optional)\n";
        for (const auto& cmd : Commands) {
            if (!cmd.isCustom) {
                file << cmd.name << "|" << cmd.command << "|" << (cmd.closeConsole ? "1" : "0");
                if (!cmd.tooltip.empty()) {
                    file << "|" << cmd.tooltip;
                }
                file << "\n";
            }
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

    void ToggleHideCommand(size_t index) {
        if (index < Commands.size()) {
            auto& cmd = Commands[index];
            if (cmd.isCustom) {
                logger::info("Starting toggle hide for command: {} in {}", cmd.name, cmd.sourcePath);

                std::string oldName = cmd.name;
                std::string newName = cmd.isHidden ? oldName.substr(8) : "{Hidden}" + oldName;
                cmd.name = newName;
                cmd.isHidden = !cmd.isHidden;

                std::ifstream inFile(cmd.sourcePath);
                std::vector<std::string> lines;
                std::string line;
                while (std::getline(inFile, line)) {
                    lines.push_back(line);
                }
                inFile.close();

                bool found = false;

                for (auto& l : lines) {
                    std::string parsedLine = l;
                    parsedLine.erase(std::remove(parsedLine.begin(), parsedLine.end(), '\r'), parsedLine.end());
                    parsedLine.erase(0, parsedLine.find_first_not_of(" \t"));
                    parsedLine.erase(parsedLine.find_last_not_of(" \t") + 1, std::string::npos);

                    if (parsedLine.empty() || parsedLine[0] == ';' || parsedLine[0] == '#' || parsedLine[0] == '[') continue;

                    size_t pos1 = parsedLine.find('|');
                    if (pos1 != std::string::npos) {
                        std::string parsedName = parsedLine.substr(0, pos1);
                        size_t pos2 = parsedLine.find('|', pos1 + 1);
                        std::string parsedCmd;
                        bool parsedClose = true;
                        std::string parsedTooltip;
                        if (pos2 != std::string::npos) {
                            parsedCmd = parsedLine.substr(pos1 + 1, pos2 - pos1 - 1);
                            size_t pos3 = parsedLine.find('|', pos2 + 1);
                            if (pos3 != std::string::npos) {
                                std::string closeStr = parsedLine.substr(pos2 + 1, pos3 - pos2 - 1);
                                parsedTooltip = parsedLine.substr(pos3 + 1);
                                if (!closeStr.empty()) {
                                    try {
                                        parsedClose = (std::stoi(closeStr) != 0);
                                    } catch (...) {
                                    }
                                }
                            } else {
                                std::string closeStr = parsedLine.substr(pos2 + 1);
                                if (!closeStr.empty()) {
                                    try {
                                        parsedClose = (std::stoi(closeStr) != 0);
                                    } catch (...) {
                                    }
                                }
                            }
                        } else {
                            parsedCmd = parsedLine.substr(pos1 + 1);
                        }

                        if (parsedName == oldName && parsedCmd == cmd.command && parsedClose == cmd.closeConsole && parsedTooltip == cmd.tooltip) {
                            found = true;
                            logger::info("Found matching line: {}", l);

                            std::string newLine = newName + "|" + cmd.command + "|" + (cmd.closeConsole ? "1" : "0");
                            if (!cmd.tooltip.empty()) {
                                newLine += "|" + cmd.tooltip;
                            }

                            l = newLine;
                            logger::info("Rewrote line to: {}", l);
                            break;
                        }
                    }
                }

                if (!found) {
                    logger::warn("No matching line found for toggle hide: {} in {}", oldName, cmd.sourcePath);
                }

                std::ofstream outFile(cmd.sourcePath);
                for (const auto& l : lines) {
                    outFile << l << "\n";
                }
                outFile.close();

                logger::info("Toggle hide finished for command: {} in {}", cmd.name, cmd.sourcePath);
                LoadConfiguration();  // Refresh UI
            }
        }
    }
}

// ============================================
// Key Executor Implementation (unchanged)
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

    static std::unordered_map<char, uint32_t> qwertyScan = {{'a', 30},  {'b', 48}, {'c', 46}, {'d', 32}, {'e', 18}, {'f', 33}, {'g', 34}, {'h', 35},  {'i', 23}, {'j', 36}, {'k', 37}, {'l', 38}, {'m', 50},
                                                            {'n', 49},  {'o', 24}, {'p', 25}, {'q', 16}, {'r', 19}, {'s', 31}, {'t', 20}, {'u', 22},  {'v', 47}, {'w', 17}, {'x', 45}, {'y', 21}, {'z', 44},
                                                            {'0', 11},  {'1', 2},  {'2', 3},  {'3', 4},  {'4', 5},  {'5', 6},  {'6', 7},  {'7', 8},   {'8', 9},  {'9', 10}, {' ', 57}, {'.', 52}, {',', 51},
                                                            {'/', 53},  {'-', 12}, {'_', 12}, {'=', 13}, {'+', 13}, {';', 39}, {':', 39}, {'\'', 40}, {'"', 40}, {'[', 26}, {']', 27}, {'{', 26}, {'}', 27},
                                                            {'\\', 43}, {'`', 41}, {'!', 2},  {'@', 3},  {'#', 4},  {'$', 5},  {'%', 6},  {'^', 7},   {'&', 8},  {'*', 9},  {'(', 10}, {')', 11}, {'?', 53}};

    void SendChar(char c) {
        bool isUpper = std::isupper(static_cast<unsigned char>(c));
        char lowerC = std::tolower(static_cast<unsigned char>(c));

        auto it = qwertyScan.find(lowerC);
        if (it == qwertyScan.end()) {
            logger::warn("Unsupported character: {}", c);
            return;
        }
        uint32_t scan = it->second;

        bool needShift = isUpper || c == '_' || c == '"' || c == '!' || c == '@' || c == '#' || c == '$' || c == '%' || c == '^' || c == '&' || c == '*' || c == '(' || c == ')' || c == '+' || c == '{' || c == '}' || c == ':' || c == '<' ||
                         c == '>' || c == '?';

        if (needShift) {
            SendKey(42, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        SendKey(scan, true);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        SendKey(scan, false);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        if (needShift) {
            SendKey(42, false);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void ExecuteCommand(const std::string& command, bool closeConsole = true) {
        logger::info("Executing command: {} (closeConsole: {}, delays: Esc={}, OpenConsole={}, TypingStart={}, Char={}, Enter={}, CloseConsole={})", command, closeConsole ? "yes" : "no", Configuration::EscDelay,
                     Configuration::OpenConsoleDelay, Configuration::TypingStartDelay, Configuration::CharDelay, Configuration::EnterDelay, Configuration::CloseConsoleDelay);

        // Split by comma for multi-command support
        std::vector<std::string> cmds;
        std::istringstream iss(command);
        std::string cmdPart;
        while (std::getline(iss, cmdPart, ',')) {
            cmdPart.erase(0, cmdPart.find_first_not_of(" \t"));
            cmdPart.erase(cmdPart.find_last_not_of(" \t") + 1);
            if (!cmdPart.empty()) {
                cmds.push_back(cmdPart);
            }
        }

        if (cmds.empty()) {
            logger::warn("No valid commands to execute");
            return;
        }

        std::thread([cmds, closeConsole]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            std::this_thread::sleep_for(std::chrono::milliseconds(Configuration::EscDelay));
            SendKey(1, true);  // Esc
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            SendKey(1, false);

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            auto ui = RE::UI::GetSingleton();
            bool needsOpen = ui && !ui->IsMenuOpen(RE::Console::MENU_NAME);
            if (needsOpen) {
                std::this_thread::sleep_for(std::chrono::milliseconds(Configuration::OpenConsoleDelay));
                SendKey(41, true);  // `
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                SendKey(41, false);
            } else {
                logger::info("Console already open - skipping open step");
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            for (size_t i = 0; i < cmds.size(); ++i) {
                const std::string& cmd = cmds[i];
                logger::info("Executing command {} of {}: {}", i + 1, cmds.size(), cmd);

                std::this_thread::sleep_for(std::chrono::milliseconds(Configuration::TypingStartDelay));

                for (char c : cmd) {
                    SendChar(c);
                    std::this_thread::sleep_for(std::chrono::milliseconds(Configuration::CharDelay));
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(Configuration::EnterDelay));

                SendKey(28, true);  // Enter
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                SendKey(28, false);
            }

            if (closeConsole) {
                std::this_thread::sleep_for(std::chrono::milliseconds(Configuration::CloseConsoleDelay));
                SendKey(41, true);  // `
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                SendKey(41, false);
            } else {
                logger::info("Skipping console close as per command setting");
            }
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
    static char newCommandText[4096] = "";  // large buffer for multiple commands
    static bool closeConsoleChecked = true;

    void __stdcall Render() {
        ImGuiMCP::Text("Commands:");
        ImGuiMCP::SameLine();
        if (ImGuiMCP::Button("Add Command")) {
            newCommandName[0] = '\0';
            newCommandText[0] = '\0';
            closeConsoleChecked = true;
            AddCommandWindow->IsOpen = true;
        }
        ImGuiMCP::SameLine();
        if (ImGuiMCP::Button("Reload Config")) {
            Configuration::LoadConfiguration();
            logger::info("Configuration reloaded");
        }
        ImGuiMCP::SameLine();
        std::string showLabel = Configuration::ShowHiddenGlobal ? "Hide Hidden" : "Show Hidden";
        if (ImGuiMCP::Button(showLabel.c_str())) {
            Configuration::ShowHiddenGlobal = !Configuration::ShowHiddenGlobal;
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
                    if (cmd.isCustom) continue;

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
                        logger::info("Executing command: {}", cmd.name);
                        KeyExecutor::ExecuteCommand(cmd.command, cmd.closeConsole);
                    }

                    ImGuiMCP::SameLine();

                    if (!cmd.tooltip.empty() && ImGuiMCP::IsItemHovered()) {
                        ImGuiMCP::BeginTooltip();
                        ImGuiMCP::TextUnformatted(cmd.tooltip.c_str());
                        ImGuiMCP::EndTooltip();
                    }

                    if (ImGuiMCP::Button(("Delete##" + std::to_string(i)).c_str())) {
                        logger::info("Deleted command: {}", cmd.name);
                        Configuration::RemoveCommand(i);
                        break;
                    }
                }

                // Custom commands section
                bool hasVisibleCustom = false;
                for (size_t i = 0; i < Configuration::Commands.size(); i++) {
                    const auto& cmd = Configuration::Commands[i];
                    if (!cmd.isCustom) continue;
                    if (cmd.isHidden && !Configuration::ShowHiddenGlobal) continue;

                    std::string lowerName = cmd.name;
                    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

                    std::string lowerCommand = cmd.command;
                    std::transform(lowerCommand.begin(), lowerCommand.end(), lowerCommand.begin(), ::tolower);

                    if (!lowerSearch.empty() && lowerName.find(lowerSearch) == std::string::npos && lowerCommand.find(lowerSearch) == std::string::npos) {
                        continue;
                    }

                    hasVisibleCustom = true;
                    break;
                }

                if (hasVisibleCustom) {
                    ImGuiMCP::TableNextRow();
                    ImGuiMCP::TableSetColumnIndex(0);
                    ImGuiMCP::Separator();
                    ImGuiMCP::TextColored(ImGuiMCP::ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "Custom Commands");
                    ImGuiMCP::TableSetColumnIndex(1);
                    ImGuiMCP::Separator();
                    ImGuiMCP::TableSetColumnIndex(2);
                    ImGuiMCP::Separator();

                    for (size_t i = 0; i < Configuration::Commands.size(); i++) {
                        const auto& cmd = Configuration::Commands[i];
                        if (!cmd.isCustom) continue;

                        if (cmd.isHidden && !Configuration::ShowHiddenGlobal) continue;

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
                            logger::info("Executing command: {}", cmd.name);
                            KeyExecutor::ExecuteCommand(cmd.command, cmd.closeConsole);
                        }

                        ImGuiMCP::SameLine();

                        if (!cmd.tooltip.empty() && ImGuiMCP::IsItemHovered()) {
                            ImGuiMCP::BeginTooltip();
                            ImGuiMCP::TextUnformatted(cmd.tooltip.c_str());
                            ImGuiMCP::EndTooltip();
                        }

                        ImGuiMCP::SameLine();

                        std::string hideLabel = cmd.isHidden ? "Unhide##" + std::to_string(i) : "Hide##" + std::to_string(i);
                        if (ImGuiMCP::Button(hideLabel.c_str())) {
                            Configuration::ToggleHideCommand(i);
                            break;
                        }
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

        ImGuiMCP::Text("Console Commands (separate multiple with comma):");
        ImGuiMCP::InputText("##CommandText", newCommandText, sizeof(newCommandText));
        ImGuiMCP::Spacing();

        ImGuiMCP::TextColored(ImGuiMCP::ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Example:\nCommand1,Command2,Command3");

        ImGuiMCP::Spacing();
        ImGuiMCP::Separator();
        ImGuiMCP::Spacing();

        ImGuiMCP::Checkbox("Close Console after Executing Command", &closeConsoleChecked);

        ImGuiMCP::Spacing();
        ImGuiMCP::Separator();
        ImGuiMCP::Spacing();

        ImGuiMCP::Text("Tooltip (optional - shown on hover over Execute):");
        static char tooltipBuffer[512] = "";
        ImGuiMCP::InputText("##Tooltip", tooltipBuffer, sizeof(tooltipBuffer));

        ImGuiMCP::Spacing();
        ImGuiMCP::Separator();
        ImGuiMCP::Spacing();

        bool canAdd = strlen(newCommandName) > 0 && strlen(newCommandText) > 0;
        if (!canAdd) {
            ImGuiMCP::BeginDisabled();
        }

        if (ImGuiMCP::Button("Add Command") && canAdd) {
            Configuration::ConsoleCommand newCmd(newCommandName, newCommandText, closeConsoleChecked, false, false, "", tooltipBuffer);
            logger::info("Added new command: {} (full command: {}, closeConsole: {}, tooltip: {})", newCmd.name, newCmd.command, newCmd.closeConsole ? "yes" : "no", newCmd.tooltip.empty() ? "none" : newCmd.tooltip);
            Configuration::AddCommand(newCmd);
            AddCommandWindow->IsOpen = false;
            newCommandText[0] = '\0';
            tooltipBuffer[0] = '\0';
        }

        if (!canAdd) {
            ImGuiMCP::EndDisabled();
        }

        ImGuiMCP::SameLine();

        if (ImGuiMCP::Button("Cancel")) {
            AddCommandWindow->IsOpen = false;
            newCommandText[0] = '\0';
            tooltipBuffer[0] = '\0';
        }

        ImGuiMCP::End();
    }
}