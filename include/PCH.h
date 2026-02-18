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
#include <format>
#include <cstdint>

#include "RE/U/UI.h"
#include "RE/C/Console.h"

namespace logger = SKSE::log;
using namespace std::literals;