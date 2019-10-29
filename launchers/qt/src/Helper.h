#include <QString>
#include <string>

#ifdef Q_OS_WIN
#include "Windows.h"
#endif

QString getMetaverseAPIDomain();

void launchClient(const QString& clientPath, const QString& homePath, const QString& defaultScriptOverride,
                  const QString& displayName, const QString& contentCachePath, QString loginResponseToken = QString());


void launchAutoUpdater(const QString& autoUpdaterPath);
bool swapLaunchers(const QString& oldLauncherPath = QString(), const QString& newLauncherPath = QString());
void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message);
void cleanLogFile();

#ifdef Q_OS_MAC
bool replaceDirectory(const QString& orginalDirectory, const QString& newDirectory);
void closeInterfaceIfRunning();
void waitForInterfaceToClose();
bool isLauncherAlreadyRunning();
QString getBundlePath();
#endif

#ifdef Q_OS_WIN
HRESULT createSymbolicLink(LPCSTR lpszPathObj, LPCSTR lpszPathLink, LPCSTR lpszDesc, LPCSTR lpszArgs = (LPCSTR)"");
bool insertRegistryKey(const std::string& regPath, const std::string& name, const std::string& value);
bool insertRegistryKey(const std::string& regPath, const std::string& name, DWORD value);
bool deleteRegistryKey(const std::string& regPath);

BOOL isProcessRunning(const char* processName, int& processID);
BOOL shutdownProcess(DWORD dwProcessId, UINT uExitCode);
#endif

QString getHTTPUserAgent();

const QString& getInterfaceSharedMemoryName();
