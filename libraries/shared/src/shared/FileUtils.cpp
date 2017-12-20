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

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QUrl>
#include <QtCore/QTextStream>
#include <QtCore/QRegularExpression>
#include <QtGui/QDesktopServices>

#include "../SharedLogging.h"


QString FileUtils::readFile(const QString& filename) {
    QFile file(filename);
    file.open(QFile::Text | QFile::ReadOnly);
    QString result;
    result.append(QTextStream(&file).readAll());
    return result;
}

QStringList FileUtils::readLines(const QString& filename, QString::SplitBehavior splitBehavior) {
    return readFile(filename).split(QRegularExpression("[\\r\\n]"), QString::SkipEmptyParts);
}

void FileUtils::locateFile(QString filePath) {

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
    QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
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
    QDir dir(fileInfo.absolutePath());
    if (!dir.exists()) {
        if (!dir.mkpath(fullPath)) {
            qDebug(shared) << "unable to create dir '" << dir.absolutePath() << "'";
            return false;
        }
    }
    return true;
}
