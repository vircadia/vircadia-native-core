#include <QString>
#include <string>

void launchClient(const QString& clientPath, const QString& homePath, const QString& defaultScriptOverride,
                  const QString& displayName, const QString& contentCachePath, const QString& loginResponseToken = QString());


void launchAutoUpdater(const QString& autoUpdaterPath);
void swapLaunchers(const QString& oldLauncherPath = QString(), const QString& newLauncherPath = QString());

#ifdef Q_OS_MAC
bool replaceDirectory(const QString& orginalDirectory, const QString& newDirectory);
#endif
