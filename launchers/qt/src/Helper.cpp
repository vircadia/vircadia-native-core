#include "Helper.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QProcess>


void swapLaunchers(const QString& oldLauncherPath, const QString& newLauncherPath) {
    if (!(QFileInfo::exists(oldLauncherPath) && QFileInfo::exists(newLauncherPath))) {
        qDebug() << "old launcher: " << oldLauncherPath << "new launcher: " << newLauncherPath << " file does not exist";
    }

    bool success = false;
#ifdef Q_OS_MAC
    success = replaceDirectory(oldLauncherPath, newLauncherPath);
#endif

    if (success) {
        qDebug() << "succeessfully replaced";
    } else {
        qDebug() << "failed";
        exit(0);
    }
}
