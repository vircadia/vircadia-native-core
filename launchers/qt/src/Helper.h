#include <QString>
#include <string>

#ifdef Q_OS_WIN
#include "Windows.h"
#endif

//#define USE_STAGING

#ifdef USE_STAGING
const QString METAVERSE_API_DOMAIN{ "https://staging.highfidelity.com" };
#else
const QString METAVERSE_API_DOMAIN{ "https://metaverse.highfidelity.com" };
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
bool insertRegistryKey(const std::string& regPath, const std::string& name, const std::string& value);
bool insertRegistryKey(const std::string& regPath, const std::string& name, DWORD value);
bool deleteRegistryKey(const std::string& regPath);
#endif

QString getHTTPUserAgent();

const QString& getInterfaceSharedMemoryName();

//#ifdef Q_OS_WIN
//bool isProcessRunning(const wchar_t *processName, int& processID) {
//#endif
