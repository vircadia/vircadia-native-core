//
//  FileUtils.cpp
//  interface/src
//
//  Created by Stojce Slavkovski on 12/23/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "FileUtils.h"

#include <mutex>

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QUrl>
#include <QtCore/QTextStream>
#include <QtCore/QRegularExpression>
#include <QtCore/QFileSelector>
#include <QtGui/QDesktopServices>


#include "../SharedLogging.h"

const QStringList& FileUtils::getFileSelectors() {
    static std::once_flag once;
    static QStringList extraSelectors;
    std::call_once(once, [] {

#if defined(Q_OS_ANDROID)
        extraSelectors << "android_" HIFI_ANDROID_APP;
#endif

#if defined(USE_GLES)
        extraSelectors << "gles";
#endif

#ifndef Q_OS_ANDROID
        extraSelectors << "webengine";
#endif
    });
    return extraSelectors;

}

QString FileUtils::selectFile(const QString& path) {
    QFileSelector fileSelector;
    fileSelector.setExtraSelectors(FileUtils::getFileSelectors());
    QString result = fileSelector.select(path);
    if (path != result) {
        qDebug() << "Using" << result << "instead of" << path;
    }
    return result;
}


QString FileUtils::readFile(const QString& filename) {
    QFile file(filename);
    file.open(QFile::Text | QFile::ReadOnly);
    QString result;
    result.append(QTextStream(&file).readAll());
    return result;
}

QStringList FileUtils::readLines(const QString& filename, Qt::SplitBehavior splitBehavior) {
    return readFile(filename).split(QRegularExpression("[\\r\\n]"), Qt::SkipEmptyParts);
}

void FileUtils::locateFile(const QString& filePath) {

    // adapted from
    // http://stackoverflow.com/questions/3490336/how-to-reveal-in-finder-or-show-in-explorer-with-qt
    // and
    // http://lynxline.com/show-in-finder-show-in-explorer/

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        return;
    }

    bool success = false;
#ifdef Q_OS_MAC
    QStringList args;
    args << "-e";
    args << "tell application \"Finder\"";
    args << "-e";
    args << "activate";
    args << "-e";
    args << "select POSIX file \"" + fileInfo.absoluteFilePath().toUtf8() + "\"";
    args << "-e";
    args << "end tell";
    success = QProcess::startDetached("osascript", args);
#endif

#ifdef Q_OS_WIN

    QStringList args;
    // don't send `select` command switch if `filePath` is folder
    if (!fileInfo.isDir()) {
        args << "/select,";
    }
    args += QDir::toNativeSeparators(fileInfo.absoluteFilePath().toUtf8());
    success = QProcess::startDetached("explorer", args);

#endif

    // fallback, open enclosing folder
    if (!success) {
        const QString folder = fileInfo.path();
        QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
    }
}

QString FileUtils::standardPath(QString subfolder) {
    // standard path
    // Mac: ~/Library/Application Support/Interface
#ifdef Q_OS_ANDROID
    QString path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
#else
    QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
#endif
    if (!subfolder.startsWith("/")) {
        subfolder.prepend("/");
    }
    if (!subfolder.endsWith("/")) {
        subfolder.append("/");
    }
    path.append(subfolder);
    QDir logDir(path);
    if (!logDir.exists(path)) {
        logDir.mkpath(path);
    }
    return path;
}

QString FileUtils::replaceDateTimeTokens(const QString& originalPath) {
    // Filter for specific tokens potentially present in the path:
    auto now = QDateTime::currentDateTime();
    QString path = originalPath;
    path.replace("{DATE}", now.date().toString("yyyyMMdd"));
    path.replace("{TIME}", now.time().toString("HHmm"));
    return path;
}


QString FileUtils::computeDocumentPath(const QString& originalPath) {
    // If the filename is relative, turn it into an absolute path relative to the document directory.
    QString path = originalPath;
    QFileInfo originalFileInfo(originalPath);
    if (originalFileInfo.isRelative()) {
        QString docsLocation = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        path = docsLocation + "/" + originalPath;
    }
    return path;
}

bool FileUtils::canCreateFile(const QString& fullPath) {
    // If the file exists and we can't remove it, fail early
    QFileInfo fileInfo(fullPath);
    if (fileInfo.exists() && !QFile::remove(fullPath)) {
        qDebug(shared) << "unable to overwrite file '" << fullPath << "'";
        return false;
    }

    QString absolutePath = fileInfo.absolutePath();
    QDir dir(absolutePath);
    if (!dir.exists()) {
        if (!dir.mkpath(absolutePath)) {
            qDebug(shared) << "unable to create dir '" << absolutePath << "'";
            return false;
        }
    }
    return true;
}

QString FileUtils::getParentPath(const QString& fullPath) {
    return QFileInfo(fullPath).absoluteDir().canonicalPath();
}

bool FileUtils::exists(const QString& fileName) {
    return QFileInfo(fileName).exists();
}

bool FileUtils::isRelative(const QString& fileName) {
    return QFileInfo(fileName).isRelative();
}
