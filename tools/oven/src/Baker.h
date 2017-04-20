//
//  Baker.h
//  tools/oven/src
//
//  Created by Stephen Birarda on 4/14/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Baker_h
#define hifi_Baker_h

#include <QtCore/QObject>

class Baker : public QObject {
    Q_OBJECT

public:
    bool hasErrors() const { return !_errorList.isEmpty(); }
    QStringList getErrors() const { return _errorList; }

    bool hasWarnings() const { return !_warningList.isEmpty(); }
    QStringList getWarnings() const { return _warningList; }

public slots:
    virtual void bake() = 0;

signals:
    void finished();

protected:
    void handleError(const QString& error);
    void handleWarning(const QString& warning);

    void handleErrors(const QStringList& errors);

    QStringList _errorList;
    QStringList _warningList;
};

#endif // hifi_Baker_h
