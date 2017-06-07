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
   } else {
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
       } else {                      
           if (typeid(message) != typeid(QString)) {
               message = assertFailed;
           } else {
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
