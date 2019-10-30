#ifndef hifi_ScreenshareScriptingInterface_h
#define hifi_ScreenshareScriptingInterface_h

#include <QObject>
#include <DependencyManager.h>

class ScreenshareScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
public:
	ScreenshareScriptingInterface();
    // This is a note for Milad:
    // The value of this `SCREENSHARE_EXE_PATH` string will have to be changed based on the operating system we're using.
    // The binary should probably be stored in a path like this one.
    const QString SCREENSHARE_EXE_PATH{ "C:\\hifi\\hiFi\\screenshare\\screenshare-win32-x64\\screenshare.exe" };

	Q_INVOKABLE void startScreenshare(QString displayName, QString userName, QString token, QString sessionID, QString apiKey, QString fileLocation);
};

#endif // hifi_ScreenshareScriptingInterface_h