//
//  FileLogger.cpp
//  hifi
//
//  Created by Stojce Slavkovski on 12/22/13.
//
//

#include "FileLogger.h"
#include "HifiSockAddr.h"
#include <QDateTime>
#include <QFile>

FileLogger::FileLogger() : _lines(NULL) {
    QHostAddress clientAddress = QHostAddress(getHostOrderLocalAddress());
    QDateTime now = QDateTime::currentDateTime();
    _fileName = QString("hifi-log_%1_%2.txt").arg(clientAddress.toString(), now.toString("yyyy-MM-dd_hh.mm.ss"));
    setExtraDebugging(false);
}

QStringList FileLogger::getLogData(QString searchText) {
    if (searchText.isEmpty()) {
        return _lines;
    }

    // wait for adding new log data while iterating over _lines
    _mutex.lock();
    QStringList filteredList;
    for (int i = 0; i < _lines.size(); ++i) {
        if (_lines[i].contains(searchText, Qt::CaseInsensitive)) {
            filteredList.append(_lines[i]);
        }
    }
    _mutex.unlock();
    return filteredList;
}

void FileLogger::addMessage(QString message) {
    _mutex.lock();
    emit logReceived(message);
    _lines.append(message);

    QFile file(_fileName);

    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out <<  message;
    }
    _mutex.unlock();
}