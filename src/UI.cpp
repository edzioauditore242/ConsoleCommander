#include "UI.h"
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <thread>

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
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == ';' || line[0] == '#') continue;
            size_t pos = line.find('|');
            if (pos != std::string::npos) {
                std::string name = line.substr(0, pos);
                std::string cmd = line.substr(pos + 1);
                Commands.push_back(ConsoleCommand(name, cmd));
            }
        }
        file.close();
        logger::info("Loaded {} commands from configuration", Commands.size());
    }

    void SaveConfiguration() {
        std::string configPath = GetConfigPath();
        std::ofstream file(configPath);
        if (!file.is_open()) {
            logger::error("Failed to save configuration to {}", configPath);
            return;
        }
        file << "; Console Commander Config\n";
        file << "; Format: Name|ConsoleCommand\n";
        file << "; Example: Add Gold|player.additem f 100\n\n";
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
// Key Executor Implementation (adapted from Execute Hotkeys)
// ============================================
namespace KeyExecutor {
    void SendKey(uint32_t dxCode, bool down) {
        INPUT input = {0};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = 0;
        input.ki.wScan = static_cast<WORD>(dxCode);
        input.ki.dwFlags = KEYEVENTF_SCANCODE | (down ? 0 : KEYEVENTF_KEYUP);
        if (dxCode >= 0xE0 && dxCode <= 0xE7) {  // Extended keys
            input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        }
        SendInput(1, &input, sizeof(INPUT));
    }

    // Map from char (lowercase) to DX scan code (from your provided code)
    static std::unordered_map<char, uint32_t> charToScan = {
        {'a', 30}, {'b', 48}, {'c', 46}, {'d', 32}, {'e', 18}, {'f', 33}, {'g', 34}, {'h', 35}, {'i', 23}, {'j', 36},
        {'k', 37}, {'l', 38}, {'m', 50}, {'n', 49}, {'o', 24}, {'p', 25}, {'q', 16}, {'r', 19}, {'s', 31}, {'t', 20},
        {'u', 22}, {'v', 47}, {'w', 17}, {'x', 45}, {'y', 21}, {'z', 44},
        {'0', 11}, {'1', 2}, {'2', 3}, {'3', 4}, {'4', 5}, {'5', 6}, {'6', 7}, {'7', 8}, {'8', 9}, {'9', 10},
        {' ', 57}, {'.', 52}, {',', 51}, {'/', 53}, {'-', 12}, {'=', 13}, {';', 39}, {'\'', 40}, {'[', 26}, {']', 27}, {'\\', 43}
    };

    void SendChar(char c) {
        bool isUpper = std::isupper(static_cast<unsigned char>(c));
        char lowerC = std::tolower(static_cast<unsigned char>(c));

        auto it = charToScan.find(lowerC);
        if (it == charToScan.end()) {
            logger::warn("Unsupported character: {}", c);
            return;
        }

        uint32_t scan = it->second;

        if (isUpper) {
            SendKey(42, true);  // LShift down
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        SendKey(scan, true);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        SendKey(scan, false);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        if (isUpper) {
            SendKey(42, false);  // LShift up
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void ExecuteCommand(const std::string& command) {
        logger::info("Executing command: {}", command);

        // Run in separate thread to not block UI
        std::thread([command]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));  // Delay to allow menu close

            // 1. Send Esc to close menu
            SendKey(1, true);   // Esc down (dxCode 1)
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            SendKey(1, false);  // Esc up

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // 2. Send ` to open console
            SendKey(41, true);  // ` down (dxCode 41 for ~/` )
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            SendKey(41, false); // ` up

            std::this_thread::sleep_for(std::chrono::milliseconds(200));  // Wait for console to open

            // 3. Type the command char by char
            for (char c : command) {
                SendChar(c);
                std::this_thread::sleep_for(std::chrono::milliseconds(30));  // Small delay between chars for reliability
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // 4. Send Enter to execute
            SendKey(28, true);  // Enter down (dxCode 28)
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            SendKey(28, false); // Enter up

            std::this_thread::sleep_for(std::chrono::milliseconds(200));  // Wait for execution

            // 5. Send ` to close console
            SendKey(41, true);  // ` down
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            SendKey(41, false); // ` up

        }).detach();
    }
}

// ============================================
// UI Implementation (similar to Execute Hotkeys)
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
    // State for adding new command
    static char newCommandName[256] = "";
    static char newCommandText[1024] = "";  // Larger buffer for command

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

                    if (!lowerSearch.empty() && lowerName.find(lowerSearch) == std::string::npos && lowerCommand.find(lowerSearch) == std::string::npos) continue;

                    ImGuiMCP::TableNextRow();

                    // Name column
                    ImGuiMCP::TableSetColumnIndex(0);
                    ImGuiMCP::Text(cmd.name.c_str());

                    // Command column
                    ImGuiMCP::TableSetColumnIndex(1);
                    ImGuiMCP::Text(cmd.command.c_str());

                    // Actions column
                    ImGuiMCP::TableSetColumnIndex(2);
                    std::string executeButtonId = "Execute##" + std::to_string(i);
                    std::string deleteButtonId = "Delete##" + std::to_string(i);

                    if (ImGuiMCP::Button(executeButtonId.c_str())) {
                        logger::info("Executing command: {}", cmd.name);
                        KeyExecutor::ExecuteCommand(cmd.command);
                    }

                    ImGuiMCP::SameLine();

                    if (ImGuiMCP::Button(deleteButtonId.c_str())) {
                        Configuration::RemoveCommand(i);
                        logger::info("Deleted command: {}", cmd.name);
                        break;  // Exit loop since vector was modified
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
            Configuration::AddCommand(newCmd);
            logger::info("Added new command: {}", newCmd.name);
            AddCommandWindow->IsOpen = false;
        }

        if (!canAdd) {
            ImGuiMCP::PopStyleVar();
        }

        ImGuiMCP::SameLine();

        if (ImGuiMCP::Button("Cancel")) {
            AddCommandWindow->IsOpen = false;
        }

        if (ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_Escape)) {
            AddCommandWindow->IsOpen = false;
        }

        ImGuiMCP::End();
    }
}