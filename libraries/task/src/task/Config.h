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

#include <chrono>

#include <QtCore/qobject.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonvalue.h>
#include <shared/JSONHelpers.h>

#include "SettingHandle.h"

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
        if (C::isEnabled()) {
            _presets.insert(DEFAULT, _default);
        }
        _presets.insert(NONE, QVariantMap{{ "enabled", false }});

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

protected:
    friend class TaskConfig;

    bool _isEnabled{ true };

public:
    using Persistent = PersistentConfig<JobConfig>;

    JobConfig() = default;
    JobConfig(bool enabled): _isEnabled{ enabled }  {}
    ~JobConfig();

    bool isEnabled() const { return _isEnabled; }
    void setEnabled(bool enable);

    virtual void setPresetList(const QJsonObject& object);

    /**jsdoc
     * @function Render.toJSON
     * @returns {string}
     */
    // This must be named toJSON to integrate with the global scripting JSON object
    Q_INVOKABLE QString toJSON() { return QJsonDocument(toJsonValue(*this).toObject()).toJson(QJsonDocument::Compact); }

    /**jsdoc
     * @function Render.load
     * @param {object} map
     */
    Q_INVOKABLE void load(const QVariantMap& map) { qObjectFromJsonValue(QJsonObject::fromVariantMap(map), *this); emit loaded(); }

    Q_INVOKABLE QObject* getConfig(const QString& name) { return nullptr; }

    // Running Time measurement
    // The new stats signal is emitted once per run time of a job when stats  (cpu runtime) are updated
    void setCPURunTime(const std::chrono::nanoseconds& runtime) { _msCPURunTime = std::chrono::duration<double, std::milli>(runtime).count(); emit newStats(); }
    double getCPURunTime() const { return _msCPURunTime; }

    // Describe the node graph data connections of the associated Job/Task
    /**jsdoc
     * @function Render.isTask
     * @returns {boolean}
     */
    Q_INVOKABLE virtual bool isTask() const { return false; }

    /**jsdoc
     * @function Render.getSubConfigs
     * @returns {object[]}
     */
    Q_INVOKABLE virtual QObjectList getSubConfigs() const { return QObjectList(); }

    /**jsdoc
     * @function Render.getNumSubs
     * @returns {number}
     */
    Q_INVOKABLE virtual int getNumSubs() const { return 0; }

    /**jsdoc
     * @function Render.getSubConfig
     * @param {number} index
     * @returns {object}
     */
    Q_INVOKABLE virtual QObject* getSubConfig(int i) const { return nullptr; }

    void connectChildConfig(std::shared_ptr<JobConfig> childConfig, const std::string& name);
    void transferChildrenConfigs(std::shared_ptr<JobConfig> source);

    JobConcept* _jobConcept;

public slots:

    /**jsdoc
     * @function Render.load
     * @param {object} map
     */
    void load(const QJsonObject& val) { qObjectFromJsonValue(val, *this); emit loaded(); }

    /**jsdoc
     * @function Render.refresh
     */
    void refresh();

signals:

    /**jsdoc
     * @function Render.loaded
     * @returns {Signal}
     */
    void loaded();

    /**jsdoc
     * @function Render.newStats
     * @returns {Signal}
     */
    void newStats();

    /**jsdoc
     * @function Render.dirtyEnabled
     * @returns {Signal}
     */
    void dirtyEnabled();
};

using QConfigPointer = std::shared_ptr<JobConfig>;

class TConfigProxy {
public:
    using Config = JobConfig;
};


/**jsdoc
 * @namespace Render
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 *
 * @property {number} cpuRunTime - <em>Read-only.</em>
 * @property {boolean} enabled
 */
class TaskConfig : public JobConfig {
    Q_OBJECT

public:
    using Persistent = PersistentConfig<TaskConfig>;

    TaskConfig() = default;
    TaskConfig(bool enabled) : JobConfig(enabled) {}


    /**jsdoc
     * @function Render.getConfig
     * @param {string} name
     * @returns {object}
     */
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
    TaskConfig* getRootConfig(const std::string& jobPath, std::string& jobName) const;
    JobConfig* getJobConfig(const std::string& jobPath) const;

    template <class T> typename T::Config* getConfig(std::string jobPath = "") const {
        return dynamic_cast<typename T::Config*>(getJobConfig(jobPath));
    }

    Q_INVOKABLE bool isTask() const override { return true; }
    Q_INVOKABLE QObjectList getSubConfigs() const override {
        auto list = findChildren<JobConfig*>(QRegExp(".*"), Qt::FindDirectChildrenOnly);
        QObjectList returned;
        for (int i = 0; i < list.size(); i++) {
            returned.push_back(list[i]);
        }
        return returned;
    }
    Q_INVOKABLE int getNumSubs() const override { return getSubConfigs().size(); }
    Q_INVOKABLE QObject* getSubConfig(int i) const override {
        auto subs = getSubConfigs();
        return ((i < 0 || i >= subs.size()) ? nullptr : subs[i]);
    }
};

class SwitchConfig : public JobConfig {
    Q_OBJECT
    Q_PROPERTY(bool branch READ getBranch WRITE setBranch NOTIFY dirtyEnabled)

public:
    uint8_t getBranch() const { return _branch; }
    void setBranch(uint8_t index);

protected:
    uint8_t _branch { 0 };
};

}

#endif // hifi_task_Config_h
