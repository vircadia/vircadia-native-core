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
    Q_PROPERTY(QString renderMethod READ getRenderMethod WRITE setRenderMethod)

public:
    RenderScriptingInterface();

    static RenderScriptingInterface* getInstance();

public slots:
    /**jsdoc
     * Get a config for a job by name
     * @function Render.getConfig
     * @param {string} name - Can be:
     *     - <job_name>.  Search for the first job named job_name traversing the the sub graph of task and jobs (from this task as root)
     *     - <parent_name>.[<sub_parent_names>.]<job_name>.  Allows you to first look for the parent_name job (from this task as root) and then search from there for the
     *    optional sub_parent_names and finally from there looking for the job_name (assuming every job in the path is found)
     * @returns {object} The sub job config.
     */
    QObject* getConfig(const QString& name) { return qApp->getRenderEngine()->getConfiguration()->getConfig(name); }

    /**jsdoc
     * Gets the current render method
     * @function Render.getRenderMethod
     * @returns {string} <code>"deferred"</code> or <code>"forward"</code>
     */
    QString getRenderMethod();

    /**jsdoc
     * Sets the current render method
     * @function Render.setRenderMethod
     * @param {string} renderMethod - <code>"deferred"</code> or <code>"forward"</code>
     */
    void setRenderMethod(const QString& renderMethod);

private:
    Setting::Handle<int> _renderMethodSetting { "renderMethod", RENDER_FORWARD ? render::Args::RenderMethod::FORWARD : render::Args::RenderMethod::DEFERRED };
};

#endif  // hifi_RenderScriptingInterface_h
