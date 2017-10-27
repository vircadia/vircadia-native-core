//
//  Config.h
//  render/src/render
//
//  Created by Zach Pomerantz on 1/6/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_task_Config_h
#define hifi_task_Config_h

#include <QtCore/qobject.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonvalue.h>
#include <shared/JSONHelpers.h>

#include "SettingHandle.h"

#include "Logging.h"

namespace task {

class JobConcept;

template <class C> class PersistentConfig : public C {
public:
    const QString DEFAULT = "Default";
    const QString NONE = "None";

    PersistentConfig() = delete;
    PersistentConfig(const QStringList& path) :
        _preset(path, DEFAULT) { }
    PersistentConfig(const QStringList& path, bool enabled) : C(enabled),
        _preset(path, enabled ? DEFAULT : NONE) { }

    QStringList getPresetList() {
        if (_presets.empty()) {
            setPresetList(QJsonObject());
        }
        return _presets.keys();
    }

    virtual void setPresetList(const QJsonObject& list) override {
        assert(_presets.empty());

        _default = toJsonValue(*this).toObject().toVariantMap();

        _presets.unite(list.toVariantMap());
        if (C::alwaysEnabled || C::enabled) {
            _presets.insert(DEFAULT, _default);
        }
        if (!C::alwaysEnabled) {
            _presets.insert(NONE, QVariantMap{{ "enabled", false }});
        }

        auto preset = _preset.get();
        if (preset != _preset.getDefault() && _presets.contains(preset)) {
            // Load the persisted configuration
            C::load(_presets[preset].toMap());
        }
    }

    QString getPreset() { return _preset.get(); }

    void setPreset(const QString& preset) {
        _preset.set(preset);
        if (_presets.contains(preset)) {
            // Always start back at default to remain deterministic
            QVariantMap config = _default;
            QVariantMap presetConfig = _presets[preset].toMap();
            for (auto it = presetConfig.cbegin(); it != presetConfig.cend(); it++) {
                config.insert(it.key(), it.value());
            }
            C::load(config);
        }
    }

protected:
    QVariantMap _default;
    QVariantMap _presets;
    Setting::Handle<QString> _preset;
};

// A default Config is always on; to create an enableable Config, use the ctor JobConfig(bool enabled)
class JobConfig : public QObject {
    Q_OBJECT
    Q_PROPERTY(double cpuRunTime READ getCPURunTime NOTIFY newStats()) //ms
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY dirtyEnabled())

    double _msCPURunTime{ 0.0 };
public:
    using Persistent = PersistentConfig<JobConfig>;

    JobConfig() = default;
    JobConfig(bool enabled) : alwaysEnabled{ false }, enabled{ enabled } {}

    bool isEnabled() { return alwaysEnabled || enabled; }
    void setEnabled(bool enable) { enabled = alwaysEnabled || enable; emit dirtyEnabled(); }

    bool alwaysEnabled{ true };
    bool enabled{ true };

    virtual void setPresetList(const QJsonObject& object);

    // This must be named toJSON to integrate with the global scripting JSON object
    Q_INVOKABLE QString toJSON() { return QJsonDocument(toJsonValue(*this).toObject()).toJson(QJsonDocument::Compact); }
    Q_INVOKABLE void load(const QVariantMap& map) { qObjectFromJsonValue(QJsonObject::fromVariantMap(map), *this); emit loaded(); }

    // Running Time measurement
    // The new stats signal is emitted once per run time of a job when stats  (cpu runtime) are updated
    void setCPURunTime(double mstime) { _msCPURunTime = mstime; emit newStats(); }
    double getCPURunTime() const { return _msCPURunTime; }

public slots:
    void load(const QJsonObject& val) { qObjectFromJsonValue(val, *this); emit loaded(); }

signals:
    void loaded();
    void newStats();
    void dirtyEnabled();
};

class TConfigProxy {
public:
    using Config = JobConfig;
};

class TaskConfig : public JobConfig {
    Q_OBJECT
public:
    using QConfigPointer = std::shared_ptr<QObject>;

    using Persistent = PersistentConfig<TaskConfig>;

    TaskConfig() = default ;
    TaskConfig(bool enabled) : JobConfig(enabled) {}


    
    // Get a sub job config through task.getConfig(path)
    // where path can be:
    // - <job_name> search for the first job named job_name traversing the the sub graph of task and jobs (from this task as root)
    // - <parent_name>.[<sub_parent_names>.]<job_name>
    //    Allowing to first look for the parent_name job (from this task as root) and then search from there for the 
    //    optional sub_parent_names and finally from there looking for the job_name (assuming every job in the path were found)
    //
    // getter for qml integration, prefer the templated getter
    Q_INVOKABLE QObject* getConfig(const QString& name) { return getConfig<TConfigProxy>(name.toStdString()); }
    // getter for cpp (strictly typed), prefer this getter
    template <class T> typename T::Config* getConfig(std::string job = "") const {
        const TaskConfig* root = this;
        QString path = (job.empty() ? QString() : QString(job.c_str())); // an empty string is not a null string
        auto tokens = path.split('.', QString::SkipEmptyParts);

        if (tokens.empty()) {
            tokens.push_back(QString());
        } else {
            while (tokens.size() > 1) {
                auto name = tokens.front();
                tokens.pop_front();
                root = QObject::findChild<TaskConfig*>(name);
                if (!root) {
                    return nullptr;
                }
            }
        }

        return root->findChild<typename T::Config*>(tokens.front());
    }

    void connectChildConfig(QConfigPointer childConfig, const std::string& name);
    void transferChildrenConfigs(QConfigPointer source);
     
    JobConcept* _task;

public slots:
    void refresh();
};

using QConfigPointer = std::shared_ptr<QObject>;
    
}

#endif // hifi_task_Config_h
