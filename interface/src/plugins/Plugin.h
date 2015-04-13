#pragma once

#include <QString>
#include <QObject>

class Plugin : public QObject {
private:
    bool _active{ false };
public:
    virtual const QString & getName() = 0;
    virtual bool isSupported() { return true; }

    virtual void init() {}
    virtual void deinit() {}

    virtual void activate() {}
    virtual void deactivate() {}

    virtual void idle() {}
};
