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

#if defined(Q_OS_WIN)
#include <QWinEventNotifier>
#endif

#include <DependencyManager.h>

/**jsdoc
 * @namespace SpeechRecognizer
 *
 * @hifi-interface
 * @hifi-client-entity
 */
class SpeechRecognizer : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
    
public:
    void handleCommandRecognized(const char* command);
    bool getEnabled() const { return _enabled; }

public slots:

    /**jsdoc
     * @function SpeechRecognizer.setEnabled
     * @param {boolean} enabled
     */
    void setEnabled(bool enabled);

    /**jsdoc
     * @function SpeechRecognizer.addCommand
     * @param {string} command
     */
    void addCommand(const QString& command);

    /**jsdoc
     * @function SpeechRecognizer.removeCommand
     * @param {string} command
     */
    void removeCommand(const QString& command);

signals:

    /**jsdoc
     * @function SpeechRecognizer.commandRecognized
     * @param {string} command
     * @returns {Signal}
     */
    void commandRecognized(const QString& command);

    /**jsdoc
     * @function SpeechRecognizer.enabledUpdated
     * @param {boolean} enabled
     * @returns {Signal}
     */
    void enabledUpdated(bool enabled);

protected:
    void reloadCommands();

private:
    SpeechRecognizer();
    virtual ~SpeechRecognizer();
    
    bool _enabled;
    QSet<QString> _commands;
#if defined(Q_OS_MAC)
    void* _speechRecognizerDelegate;
    void* _speechRecognizer;
#elif defined(Q_OS_WIN)
    bool _comInitialized;
    // Use void* instead of ATL CComPtr<> for speech recognizer in order to avoid linker errors with Visual Studio Express.
    void* _speechRecognizer;
    void* _speechRecognizerContext;
    void* _speechRecognizerGrammar;
    void* _commandRecognizedEvent;
    QWinEventNotifier* _commandRecognizedNotifier;
#endif

#if defined(Q_OS_WIN)
private slots:
    void notifyCommandRecognized(void* handle);
#endif
};

#endif // hifi_SpeechRecognizer_h
