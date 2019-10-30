#include "CommandlineOptions.h"

#include <algorithm>
#include <iostream>
#include <QDebug>
#include <QString>

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
            _commandlineOptions.push_back(option);
        }
    }
}

void CommandlineOptions::append(const std::string& command) {
    _commandlineOptions.push_back(command);
}

CommandlineOptions* CommandlineOptions::getInstance() {
    static CommandlineOptions commandlineOptions;
    return &commandlineOptions;
}
