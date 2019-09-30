#include <QString>
#include <string>

#ifdef Q_OS_WIN
#include "Windows.h"
#endif

void launchClient(const QString& clientPath, const QString& homePath, const QString& defaultScriptOverride,
                  const QString& displayName, const QString& contentCachePath, QString loginResponseToken = QString());


void launchAutoUpdater(const QString& autoUpdaterPath);
void swapLaunchers(const QString& oldLauncherPath = QString(), const QString& newLauncherPath = QString());

#ifdef Q_OS_MAC
bool replaceDirectory(const QString& orginalDirectory, const QString& newDirectory);
void closeInterfaceIfRunning();
void waitForInterfaceToClose();
bool isLauncherAlreadyRunning();
#endif

#ifdef Q_OS_WIN
HRESULT createSymbolicLink(LPCSTR lpszPathObj, LPCSTR lpszPathLink, LPCSTR lpszDesc, LPCSTR lpszArgs = (LPCSTR)"");
#endif
