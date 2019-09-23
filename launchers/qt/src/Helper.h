#include <QString>
#include <string>

void launchClient(const QString& clientPath, const QString& homePath, const QString& defaultScriptOverride,
                  const QString& displayName, const QString& contentCachePath, QString loginResponseToken = QString());


void launchAutoUpdater(const QString& autoUpdaterPath);
void swapLaunchers(const QString& oldLauncherPath = QString(), const QString& newLauncherPath = QString());

#ifdef Q_OS_MAC
bool replaceDirectory(const QString& orginalDirectory, const QString& newDirectory);
#endif
>>>>>>> 09b9b1e10cae3dfc901d48c5af8f0b6f486cbbda
