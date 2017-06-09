//
//  ConsoleScriptingInterface.cpp
//  libraries/script-engine/src
//
//  Created by volansystech on 6/1/17.
//  Copyright 2014 High Fidelity, Inc.
//
//  ConsoleScriptingInterface is responsible to print logs 
//  using "console" object on debug Window and Logs/log file with proper tags.
//  Used in Javascripts file.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ConsoleScriptingInterface.h"
#include "ScriptEngine.h"

void ConsoleScriptingInterface::info(QString message) {
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
        scriptEngine->scriptInfoMessage(message);
    }
}

void ConsoleScriptingInterface::log(QString message) {
    /*if (!listgrp.isEmpty()){
    listgrp.append(qMakePair(currentGroupSelection, message));
    }*/
    /*_listOfGroupData.append(qMakePair(_selectedGroup, message));
    if (!_listOfGroupData.isEmpty()) {
        showdata(_selectedGroup, message);
    }*/
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
        scriptEngine->scriptPrintedMessage(message);
    }
}

void ConsoleScriptingInterface::debug(QString message) {
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
        scriptEngine->scriptPrintedMessage(message);
    }
}

void ConsoleScriptingInterface::warn(QString message) {
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
        scriptEngine->scriptWarningMessage(message);
    }
}

void ConsoleScriptingInterface::error(QString message) {
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
        scriptEngine->scriptErrorMessage(message);
    }
}

void ConsoleScriptingInterface::exception(QString message) {
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
        scriptEngine->scriptErrorMessage(message);
    }
}

void  ConsoleScriptingInterface::time(QString labelName) {
    QDateTime _currentTime = QDateTime::currentDateTime().toUTC();
    _listOfTimeValues.insert(labelName, _currentTime.toUTC());
}

void ConsoleScriptingInterface::timeEnd(QString labelName) {
    QDateTime _dateTimeOfLabel;
    QString message;
    qint64 millisecondsDiff = 0;

    bool _labelInList = _listOfTimeValues.contains(labelName);

    //Check if not exist items in list
    if (!_labelInList) {
        if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
            message = "No such label found " + labelName;
            scriptEngine->scriptErrorMessage(message);
        }
        return;
    }

    QDateTime _currentDateTimeValue = QDateTime::currentDateTime().toUTC();
    _dateTimeOfLabel = _listOfTimeValues.value(labelName);

    if (!_dateTimeOfLabel.isNull()) {
        _listOfTimeValues.remove(labelName);
    }

    if (_dateTimeOfLabel.toString(TIME_FORMAT) == _currentDateTimeValue.toString(TIME_FORMAT)) {
        millisecondsDiff = _dateTimeOfLabel.msecsTo(_currentDateTimeValue);
        message = QString::number(millisecondsDiff) + " ms";
    }
    else {
        message = secondsToString(_dateTimeOfLabel.secsTo(_currentDateTimeValue));
    }

    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
        scriptEngine->scriptPrintedMessage("Time : " + message);
    }
}

QString ConsoleScriptingInterface::secondsToString(qint64 seconds)
{
    qint64 days = seconds / DAY;
    QTime timeDuration = QTime(0, 0).addSecs(seconds % DAY);

    return QString("%1 days, %2 hours, %3 minutes, %4 seconds")
        .arg(QString::number(days)).arg(QString::number(timeDuration.hour())).arg(QString::number(timeDuration.minute())).arg(QString::number(timeDuration.second()));
}

void  ConsoleScriptingInterface::asserts(bool condition, QString message) {
    if (!condition) {
        QString assertFailed = "Assertion failed";
        if (message.isEmpty()) {
            message = assertFailed;
        }
        else {
            if (typeid(message) != typeid(QString)) {
                message = assertFailed;
            }
            else {
                message = assertFailed + " " + message;
            }
        }
        if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
            scriptEngine->scriptErrorMessage(message);
        }
    }
}

void  ConsoleScriptingInterface::trace(QString labelName) {
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
        scriptEngine->scriptErrorMessage(labelName);
    }
}

void  ConsoleScriptingInterface::clear() {
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
        scriptEngine->clearConsole();
    }
}

void ConsoleScriptingInterface::group(QString groupName) {
    _listOfGroupData.append(qMakePair(GROUP, groupName));
    _selectedGroup = groupName;
    showdata(GROUP, groupName);
}

void ConsoleScriptingInterface::groupCollapsed(QString  groupName) {
    _listOfGroupData.append(qMakePair(GROUPCOLLAPSED, groupName));
    _selectedGroup = groupName;
    showdata(GROUPCOLLAPSED, groupName);
}

void ConsoleScriptingInterface::consoleLog(QString  message) {
    _listOfGroupData.append(qMakePair(_selectedGroup, message));
    if (!_listOfGroupData.isEmpty()) {
        showdata(_selectedGroup, message);
    }
}

void ConsoleScriptingInterface::groupEnd() {
    _listOfGroupData.append(qMakePair(_selectedGroup, GROUPEND));
    showdata(_selectedGroup, GROUPEND);
}

void ConsoleScriptingInterface::showdata(QString currentGroup, QString groupName) {
    QPair<QString, QString> groupData;
    QString firstGroupData;

    for (int index = 0; index<_listOfGroupData.count(); index++) {
        groupData = _listOfGroupData.at(index);
        firstGroupData = groupData.first;
        if (groupData.first == GROUP || groupData.first == GROUPCOLLAPSED) {
            firstGroupData = groupData.second;
            if (_isSameLevel) {
                _addSpace = _addSpace.mid(0, _addSpace.length() - 3);
            }
            _addSpace += "   "; //add inner groupcollapsed

            if (groupData.first == GROUPCOLLAPSED) {
                QString space = _addSpace.mid(0, _addSpace.length() - 1);
                debug(space + groupData.second);
            } else {
                debug(_addSpace + groupData.second);
            }
            _isSameLevel = false;
        } else if (groupData.second == GROUPEND) {
            _addSpace = _addSpace.mid(0, _addSpace.length() - 3);
        } else {
            if (groupData.first == firstGroupData && groupData.first != GROUPCOLLAPSED && groupData.first != GROUP && groupData.second != GROUPEND) {
                if (!_isSameLevel) {
                    _addSpace += "   ";  //2 space inner element
                    _isSameLevel = true; //same level log entry
                }
                debug(_addSpace + groupData.second);
            }
        }
    }
    _listOfGroupData.removeOne(qMakePair(currentGroup, groupName));

}
