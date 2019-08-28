#include <QGuiApplication>

class Launcher : public QGuiApplication {
public:
    Launcher(int& argc, char** argv);
    ~Launcher();
};
