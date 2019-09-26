#include "CommandlineOptions.h"

#include <algorithm>
#include <iostream>

bool isCommandlineOption(const std::string& option) {
    if (option.rfind("--", 0) == 0 && option.at(2) != '-') {
        return true;
    }
    return false;
}
bool CommandlineOptions::contains(const std::string& option) {
    auto iter = std::find(_commandlineOptions.begin(), _commandlineOptions.end(), option);
    return (iter != _commandlineOptions.end());
}

void CommandlineOptions::parse(const int argc, char** argv) {
    for (int index = 1; index < argc; index++) {
        std::string option = argv[index];
        if (isCommandlineOption(option)) {
            std::cout << "adding commandline option: " << option << "\n";
            _commandlineOptions.push_back(option);
        }
    }
}

CommandlineOptions* CommandlineOptions::getInstance() {
    static CommandlineOptions commandlineOptions;
    return &commandlineOptions;
}
