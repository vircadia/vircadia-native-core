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

/*@jsdoc
 * The <code>SpeechRecognizer</code> API provides facilities to recognize voice commands.
 * <p>Speech recognition is enabled or disabled via the Developer &gt; Scripting &gt; Enable Speech Control API menu item or 
 * the {@link SpeechRecognizer.setEnabled} method.</p>
 *
 * @namespace SpeechRecognizer
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 */
class SpeechRecognizer : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
    
public:
    void handleCommandRecognized(const char* command);
    bool getEnabled() const { return _enabled; }

public slots:

    /*@jsdoc
     * Enables or disables speech recognition.
     * @function SpeechRecognizer.setEnabled
     * @param {boolean} enabled - <code>true</code> to enable speech recognition, <code>false</code> to disable.
     */
    void setEnabled(bool enabled);

    /*@jsdoc
     * Adds a voice command to the speech recognizer.
     * @function SpeechRecognizer.addCommand
     * @param {string} command - The voice command to recognize.
     */
    void addCommand(const QString& command);

    /*@jsdoc
     * Removes a voice command from the speech recognizer.
     * @function SpeechRecognizer.removeCommand
     * @param {string} command - The voice command to stop recognizing.
     */
    void removeCommand(const QString& command);

signals:

    /*@jsdoc
     * Triggered when a voice command has been recognized.
     * @function SpeechRecognizer.commandRecognized
     * @param {string} command - The voice command recognized.
     * @returns {Signal}
     * @example <caption>Turn your avatar upon voice command.</caption>
     * var TURN_LEFT = "turn left";
     * var TURN_RIGHT = "turn right";
     * var TURN_RATE = 0.5;
     * var TURN_DURATION = 1000; // ms
     * var turnRate = 0;
     * 
     * function getTurnRate() {
     *     return turnRate;
     * }
     * 
     * var MAPPING_NAME = "com.vircadia.controllers.example.speechRecognizer";
     * var mapping = Controller.newMapping(MAPPING_NAME);
     * 
     * mapping.from(getTurnRate).to(Controller.Actions.Yaw);
     * Controller.enableMapping(MAPPING_NAME);
     * 
     * function onCommandRecognized(command) {
     *     print("Speech command: " + command);
     *     switch (command) {
     *         case TURN_LEFT:
     *             turnRate = -TURN_RATE;
     *             break;
     *         case TURN_RIGHT:
     *             turnRate = TURN_RATE;
     *             break;
     *     }
     *     Script.setTimeout(function () {
     *         turnRate = 0;
     *     }, TURN_DURATION);
     * }
     * 
     * SpeechRecognizer.addCommand(TURN_LEFT);
     * SpeechRecognizer.addCommand(TURN_RIGHT);
     * SpeechRecognizer.commandRecognized.connect(onCommandRecognized);
     * 
     * Script.scriptEnding.connect(function () {
     *     Controller.disableMapping(MAPPING_NAME);
     *     SpeechRecognizer.removeCommand(TURN_LEFT);
     *     SpeechRecognizer.removeCommand(TURN_RIGHT);
     * });
     */
    void commandRecognized(const QString& command);

    /*@jsdoc
     * Triggered when speech recognition is enabled or disabled.
     * @function SpeechRecognizer.enabledUpdated
     * @param {boolean} enabled - <code>true</code> if speech recognition is enabled, <code>false</code> if it is disabled.
     * @returns {Signal}
     * @example <caption>Report when speech recognition is enabled or disabled.</caption>
     * SpeechRecognizer.enabledUpdated.connect(function (enabled) {
     *     print("Speech recognition: " + (enabled ? "enabled" : "disabled"));
     * });
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
