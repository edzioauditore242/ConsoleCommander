#pragma once

#include <spdlog/sinks/basic_file_sink.h>
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <format>  // Added to ensure std::format is available

#include "RE/U/UI.h"  // For RE::UI (menu checks)
#include "RE/C/Console.h"  // For RE::Console::MENU_NAME

namespace logger = SKSE::log;
using namespace std::literals;