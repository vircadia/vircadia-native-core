//
//  FileLogger.cpp
//  hifi
//
//  Created by Stojce Slavkovski on 12/22/13.
//
//

#include "FileLogger.h"

FileLogger::FileLogger() : _lines(NULL) {
    setExtraDebugging(false);
}

QStringList FileLogger::getLogData(QString searchText) {

    if (searchText.isEmpty()) {
        return _lines;
    }

    // wait for adding new log data while iterating over _lines
//    pthread_mutex_lock(& _mutex);
    QStringList filteredList;
    for (int i = 0; i < _lines.size(); ++i) {
        if (_lines[i].contains(searchText, Qt::CaseInsensitive)) {
            filteredList.append(_lines[i]);
        }
    }
//    pthread_mutex_unlock(& _mutex);
    return filteredList;
}

void FileLogger::addMessage(QString message) {
    emit logReceived(message);
    _lines.append(message);

    // TODO: append message to file
}