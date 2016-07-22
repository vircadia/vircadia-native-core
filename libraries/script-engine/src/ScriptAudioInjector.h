//
//  ScriptAudioInjector.h
//  libraries/script-engine/src
//
//  Created by Stephen Birarda on 2015-02-11.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ScriptAudioInjector_h
#define hifi_ScriptAudioInjector_h

#include <QtCore/QObject>

#include <AudioInjector.h>

class ScriptAudioInjector : public QObject {
    Q_OBJECT
    
    Q_PROPERTY(bool playing READ isPlaying)
    Q_PROPERTY(float loudness READ getLoudness)
    Q_PROPERTY(AudioInjectorOptions options WRITE setOptions READ getOptions)
public:
    ScriptAudioInjector(AudioInjector* injector);
    ~ScriptAudioInjector();
public slots:
    void restart() { _injector->restart(); }
    void stop() { _injector->stop(); }
   
    const AudioInjectorOptions& getOptions() const { return _injector->getOptions(); }
    void setOptions(const AudioInjectorOptions& options) { _injector->setOptions(options); }
    
    float getLoudness() const { return _injector->getLoudness(); }
    bool isPlaying() const { return _injector->isPlaying(); }
    
signals:
    void finished();
    
protected slots:
    void stopInjectorImmediately();
private:
    QPointer<AudioInjector> _injector;
    
    friend QScriptValue injectorToScriptValue(QScriptEngine* engine, ScriptAudioInjector* const& in);
};

Q_DECLARE_METATYPE(ScriptAudioInjector*)

QScriptValue injectorToScriptValue(QScriptEngine* engine, ScriptAudioInjector* const& in);
void injectorFromScriptValue(const QScriptValue& object, ScriptAudioInjector*& out);

#endif // hifi_ScriptAudioInjector_h
