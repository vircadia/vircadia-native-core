#pragma once

#include <string>
#include <vector>

class CommandlineOptions {
public:
    CommandlineOptions() = default;
    ~CommandlineOptions() = default;

    void parse(const int argc, char** argv);
    bool contains(const std::string& option);
    static CommandlineOptions* getInstance();
private:
    std::vector<std::string> _commandlineOptions;
};
