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
    virtual ~Baker() = default;

    bool shouldStop();

    bool hasErrors() const { return !_errorList.isEmpty(); }
    QStringList getErrors() const { return _errorList; }

    bool hasWarnings() const { return !_warningList.isEmpty(); }
    QStringList getWarnings() const { return _warningList; }

    std::vector<QString> getOutputFiles() const { return _outputFiles; }

    virtual void setIsFinished(bool isFinished);
    bool isFinished() const { return _isFinished.load(); }

    virtual void setWasAborted(bool wasAborted);

    bool wasAborted() const { return _wasAborted.load(); }

public slots:
    virtual void bake() = 0;
    virtual void abort() { _shouldAbort.store(true); }

signals:
    void finished();
    void aborted();

protected:
    void handleError(const QString& error);
    void handleWarning(const QString& warning);

    void handleErrors(const QStringList& errors);

    // List of baked output files. For instance, for an FBX this would
    // include the .fbx and all of its texture files.
    std::vector<QString> _outputFiles;

    QStringList _errorList;
    QStringList _warningList;

    std::atomic<bool> _isFinished { false };

    std::atomic<bool> _shouldAbort { false };
    std::atomic<bool> _wasAborted { false };
};

#endif // hifi_Baker_h
