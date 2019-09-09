#pragma once
#include <memory>

#include <QGuiApplication>

class LauncherWindow;
class LauncherState;
class Launcher : public QGuiApplication {
public:
    Launcher(int& argc, char** argv);
    ~Launcher();

private:
    std::unique_ptr<LauncherWindow> _launcherWindow;
    std::shared_ptr<LauncherState> _launcherState;
};
