//
//  AssignmentClient.h
//  assignment-client/src
//
//  Created by Stephen Birarda on 11/25/2013.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AssignmentClient_h
#define hifi_AssignmentClient_h

#include <QtCore/QCoreApplication>

#include "ThreadedAssignment.h"

class Parser {
public:
    Parser(const QString& option, bool required) : _option(option), _required(required), _exists(false), _isValid(false) {}
    virtual bool parse(const QStringList& argumentList) { return false; }
    bool exists() { return _exists; }
    bool isValid() { return _isValid; }

protected:
    int findToken(const QStringList& argumentList) {
        int argumentIndex = argumentList.indexOf(_option);
        validateOption(argumentIndex);
        return argumentIndex;
    }

    void validateOption(int index) {
        _exists = true;
        if (index == -1) {
            _exists = false;
            if (_required) {
                qDebug() << "Command line option " << _option << " is missing";
            }
        }
    }

    void valueNotSpecified() {
        _isValid = false;
        qDebug() << "Command line option " << _option << " value is missing";
    }

    void validateValue(bool ok) {
        _isValid = ok;
        if (!ok) {
            qDebug() << "Command line option " << _option << " value is invalid";
        }
    }

private:
    QString _option;
    bool _required;
    bool _exists;
    bool _isValid;
};

class AssignmentClient : public QCoreApplication {
    Q_OBJECT
public:
    AssignmentClient(int &argc, char **argv);
    ~AssignmentClient();

    static const SharedAssignmentPointer& getCurrentAssignment() { return _currentAssignment; }

private slots:
    void sendAssignmentRequest();
    void readPendingDatagrams();
    void assignmentCompleted();
    void handleAuthenticationRequest();

private:
    Assignment _requestAssignment;
    static SharedAssignmentPointer _currentAssignment;
    QString _assignmentServerHostname;

    QMap<QString, Parser*> _parsers;
};

#endif // hifi_AssignmentClient_h
