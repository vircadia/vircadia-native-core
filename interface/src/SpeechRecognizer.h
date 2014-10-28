//
//  SpeechRecognizer.h
//  interface/src
//
//  Created by Ryan Huffman on 07/31/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SpeechRecognizer_h
#define hifi_SpeechRecognizer_h

#include <QObject>
#include <QSet>
#include <QString>

#if defined(Q_OS_WIN) && defined(HAVE_ATL)
#include <QWinEventNotifier>

//#include <atlbase.h>
//#include <sapi.h>
#endif

class SpeechRecognizer : public QObject {
    Q_OBJECT
public:
    SpeechRecognizer();
    ~SpeechRecognizer();

    void handleCommandRecognized(const char* command);
    bool getEnabled() const { return _enabled; }

public slots:
    void setEnabled(bool enabled);
    void addCommand(const QString& command);
    void removeCommand(const QString& command);

signals:
    void commandRecognized(const QString& command);
    void enabledUpdated(bool enabled);

protected:
    void reloadCommands();

private:
    bool _enabled;
    QSet<QString> _commands;
#if defined(Q_OS_MAC)
    void* _speechRecognizerDelegate;
    void* _speechRecognizer;
#elif defined(Q_OS_WIN) && defined(HAVE_ATL)
    bool _comInitialized;
    // Use void* instead of ATL CComPtr<> for speech recognizer in order to avoid linker errors with Visual Studio Express.
    void* _speechRecognizer;
    void* _speechRecognizerContext;
    void* _speechRecognizerGrammar;
    void* _commandRecognizedEvent;
    QWinEventNotifier* _commandRecognizedNotifier;
#endif

#if defined(Q_OS_WIN) && defined(HAVE_ATL)
private slots:
    void notifyCommandRecognized(void* handle);
#endif
};

#endif // hifi_SpeechRecognizer_h
