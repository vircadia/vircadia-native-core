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
    Q_PROPERTY(RenderMethod renderMethod READ getRenderMethod WRITE setRenderMethod)
    Q_PROPERTY(bool shadowsEnabled READ getShadowsEnabled WRITE setShadowsEnabled)
    Q_PROPERTY(bool ambientOcclusionEnabled READ getAmbientOcclusionEnabled WRITE setAmbientOcclusionEnabled)
    Q_PROPERTY(bool antialiasingEnabled READ getAntialiasingEnabled WRITE setAntialiasingEnabled)

public:
    RenderScriptingInterface();

    static RenderScriptingInterface* getInstance();

    // RenderMethod enum type
    enum RenderMethod {
        DEFERRED = render::Args::RenderMethod::DEFERRED,
        FORWARD = render::Args::RenderMethod::FORWARD,
    };
    Q_ENUM(RenderMethod);

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
     * Gets the current render resolution scale
     * @function Render.getRenderResolutionScale
     * @returns {number} 
     */
//    RenderMethod getRenderMethod();

    /**jsdoc
     * Sets the current render method
     * @function Render.setRenderMethod
     * @param {number} renderMethod - <code>"DEFERRED"</code> or <code>"FORWARD"</code>
     */
  //  void setRenderMethod(RenderMethod renderMethod);

private:
    Setting::Handle<int> _renderMethodSetting { "renderMethod", RENDER_FORWARD ? render::Args::RenderMethod::FORWARD : render::Args::RenderMethod::DEFERRED };
    Setting::Handle<bool> _shadowsEnabledSetting { "shadowsEnabled", true };
    Setting::Handle<bool> _ambientOcclusionEnabledSetting { "ambientOcclusionEnabled", false };
    Setting::Handle<bool> _antialiasingEnabledSetting { "antialiasingEnabled", true };
};

#endif  // hifi_RenderScriptingInterface_h
