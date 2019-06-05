//
//  Created by Sam Gondelman on 5/16/19
//  Copyright 2013-2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderScriptingInterface_h
#define hifi_RenderScriptingInterface_h

#include <QtCore/QObject>
#include "Application.h"

#include "RenderForward.h"

/**jsdoc
 * The <code>Render</code> API allows you to configure the graphics engine
 *
 * @namespace Render
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 */
class RenderScriptingInterface : public QObject {
    Q_OBJECT
    Q_PROPERTY(RenderMethod renderMethod READ getRenderMethod WRITE setRenderMethod NOTIFY settingsChanged)
    Q_PROPERTY(bool shadowsEnabled READ getShadowsEnabled WRITE setShadowsEnabled NOTIFY settingsChanged)
    Q_PROPERTY(bool ambientOcclusionEnabled READ getAmbientOcclusionEnabled WRITE setAmbientOcclusionEnabled NOTIFY settingsChanged)
    Q_PROPERTY(bool antialiasingEnabled READ getAntialiasingEnabled WRITE setAntialiasingEnabled NOTIFY settingsChanged)

public:
    RenderScriptingInterface();

    static RenderScriptingInterface* getInstance();

    // RenderMethod enum type
    enum class RenderMethod {
        DEFERRED = render::Args::RenderMethod::DEFERRED,
        FORWARD = render::Args::RenderMethod::FORWARD,
    };
    Q_ENUM(RenderMethod)


    // Load Settings
    // Synchronize the runtime value to the actual setting
    // Need to be called on start up to re-initialize the runtime to the saved setting states
    void loadSettings();

public slots:
    /**jsdoc
     * Get a config for a job by name
     * @function Render.getConfig
     * @param {string} name - Can be:
     *     - <job_name>: Search for the first job named job_name traversing the the sub graph of task and jobs (from this task as root)
     *     - <parent_name>.[<sub_parent_names>.]<job_name>:  Allows you to first look for the parent_name job (from this task as root) and then search from there for the
     *    optional sub_parent_names and finally from there looking for the job_name (assuming every job in the path is found)
     * @returns {object} The sub job config.
     */
    QObject* getConfig(const QString& name) { return qApp->getRenderEngine()->getConfiguration()->getConfig(name); }

    /**jsdoc
     * Gets the current render method
     * @function Render.getRenderMethod
     * @returns {number} <code>"DEFERRED"</code> or <code>"FORWARD"</code>
     */
    RenderMethod getRenderMethod();

    /**jsdoc
     * Sets the current render method
     * @function Render.setRenderMethod
     * @param {number} renderMethod - <code>"DEFERRED"</code> or <code>"FORWARD"</code>
     */
    void setRenderMethod(RenderMethod renderMethod);

    /**jsdoc
    * Gets the possible enum names of the RenderMethod type
    * @function Render.getRenderMethodNames
    * @returns [string] [ <code>"DEFERRED"</code>, <code>"FORWARD"</code> ]
    */
    QStringList getRenderMethodNames() const;


    /**jsdoc
     * Whether or not shadows are enabled
     * @function Render.getShadowsEnabled
     * @returns {bool} <code>true</code> if shadows are enabled, otherwise <code>false</code>
     */
    bool getShadowsEnabled();

    /**jsdoc
     * Enables or disables shadows
     * @function Render.setShadowsEnabled
     * @param {bool} enabled - <code>true</code> to enable shadows, <code>false</code> to disable them
     */
    void setShadowsEnabled(bool enabled);

    /**jsdoc
     * Whether or not ambient occlusion is enabled
     * @function Render.getAmbientOcclusionEnabled
     * @returns {bool} <code>true</code> if ambient occlusion is enabled, otherwise <code>false</code>
     */
    bool getAmbientOcclusionEnabled();

    /**jsdoc
     * Enables or disables ambient occlusion
     * @function Render.setAmbientOcclusionEnabled
     * @param {bool} enabled - <code>true</code> to enable ambient occlusion, <code>false</code> to disable it
     */
    void setAmbientOcclusionEnabled(bool enabled);

    /**jsdoc
     * Whether or not anti-aliasing is enabled
     * @function Render.getAntialiasingEnabled
     * @returns {bool} <code>true</code> if anti-aliasing is enabled, otherwise <code>false</code>
     */
    bool getAntialiasingEnabled();

    /**jsdoc
     * Enables or disables anti-aliasing
     * @function Render.setAntialiasingEnabled
     * @param {bool} enabled - <code>true</code> to enable anti-aliasing, <code>false</code> to disable it
     */
    void setAntialiasingEnabled(bool enabled);

    /**jsdoc
     * Gets the current viewport resolution scale
     * @function Render.getViewportResolutionScale
     * @returns {number} 
     */
  //  float getViewportResolutionScale();

    /**jsdoc
     * Sets the current viewport resolution scale
     * @function Render.setViewportResolutionScale
     * @param {number} resolutionScale - between epsilon and 1.0
     */
  //  void setViewportResolutionScale(float resolutionScale);

signals:
    void settingsChanged();

private:
    // One lock to serialize and access safely all the settings
    mutable ReadWriteLockable _renderSettingLock;

    // Runtime value of each settings
    int  _renderMethod{ RENDER_FORWARD ? render::Args::RenderMethod::FORWARD : render::Args::RenderMethod::DEFERRED };
    bool _shadowsEnabled{ true };
    bool _ambientOcclusionEnabled{ false };
    bool _antialiasingEnabled { true };

    // Actual settings saved on disk
    Setting::Handle<int> _renderMethodSetting { "renderMethod", RENDER_FORWARD ? render::Args::RenderMethod::FORWARD : render::Args::RenderMethod::DEFERRED };
    Setting::Handle<bool> _shadowsEnabledSetting { "shadowsEnabled", true };
    Setting::Handle<bool> _ambientOcclusionEnabledSetting { "ambientOcclusionEnabled", false };
    Setting::Handle<bool> _antialiasingEnabledSetting { "antialiasingEnabled", true };

    // Force assign both setting AND runtime value to the parameter value
    void forceRenderMethod(RenderMethod renderMethod);
    void forceShadowsEnabled(bool enabled);
    void forceAmbientOcclusionEnabled(bool enabled);
    void forceAntialiasingEnabled(bool enabled);

    static std::once_flag registry_flag;
};

#endif  // hifi_RenderScriptingInterface_h
