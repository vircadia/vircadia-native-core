#ifndef hifi_ScreenshareScriptingInterface_h
#define hifi_ScreenshareScriptingInterface_h

#include <QObject>
#include <QProcess>
#include <QtCore/QCoreApplication>

#include <DependencyManager.h>
#include <PathUtils.h>

class ScreenshareScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
public:
	ScreenshareScriptingInterface();
    ~ScreenshareScriptingInterface();

	Q_INVOKABLE void startScreenshare(QString displayName, QString userName, QString token, QString sessionID, QString apiKey);
    Q_INVOKABLE void stopScreenshare();

signals:
    void screenshareStopped();

private:
#if DEV_BUILD
#ifdef Q_OS_WIN
    const QString SCREENSHARE_EXE_PATH{ PathUtils::projectRootPath() + "/screenshare/hifi-screenshare-win32-x64/hifi-screenshare.exe" };
#elif defined(Q_OS_MAC)
    const QString SCREENSHARE_EXE_PATH{ PathUtils::projectRootPath() + "/screenshare/screenshare-darwin-x64/hifi-screenshare.app" };
#else
    // This path won't exist on other platforms, so the Screenshare Scripting Interface will exit early when invoked.
    const QString SCREENSHARE_EXE_PATH{ PathUtils::projectRootPath() + "/screenshare/screenshare-other-os/hifi-screenshare" };
#endif
#else
#ifdef Q_OS_WIN
    const QString SCREENSHARE_EXE_PATH{ QCoreApplication::applicationDirPath() + "/hifi-screenshare/hifi-screenshare.exe" };
#elif defined(Q_OS_MAC)
    const QString SCREENSHARE_EXE_PATH{ QCoreApplication::applicationDirPath() + "/hifi-screenshare/hifi-screenshare.app" };
#else
    // This path won't exist on other platforms, so the Screenshare Scripting Interface will exit early when invoked.
    const QString SCREENSHARE_EXE_PATH{ QCoreApplication::applicationDirPath() + "/hifi-screenshare/hifi-screenshare" };
#endif
#endif

    std::unique_ptr<QProcess> _screenshareProcess{ nullptr };
};

#endif // hifi_ScreenshareScriptingInterface_h