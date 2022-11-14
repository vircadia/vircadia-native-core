//
//  SceneScriptingInterface.h
//  libraries/script-engine
//
//  Created by Sam Gateau on 2/24/15.
//  Copyright 2014 High Fidelity, Inc.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/// @addtogroup ScriptEngine
/// @{

#ifndef hifi_SceneScriptingInterface_h
#define hifi_SceneScriptingInterface_h

#include <qscriptengine.h>
#include <DependencyManager.h>

/*@jsdoc
 * The <code>Scene</code> API provides some control over what is rendered.
 *
 * @namespace Scene
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 *
 * @property {boolean} shouldRenderAvatars - <code>true</code> if avatars are rendered, <code>false</code> if they aren't.
 * @property {boolean} shouldRenderEntities - <code>true</code> if entities (domain, avatar, and local) are rendered, 
 *     <code>false</code> if they aren't.
 * @property {boolean} shouldRenderModelEntityPlaceholders - <code>true</code> if green cubes are rendered when a model hasn't
 *     loaded, <code>false</code> if they aren't.
 */
/// Provides the <code><a href="https://apidocs.vircadia.dev/Scene.html">Scene</a></code> scripting interface
class SceneScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    Q_PROPERTY(bool shouldRenderAvatars READ shouldRenderAvatars WRITE setShouldRenderAvatars)
    Q_PROPERTY(bool shouldRenderEntities READ shouldRenderEntities WRITE setShouldRenderEntities)
    Q_PROPERTY(bool shouldRenderModelEntityPlaceholders READ shouldRenderModelEntityPlaceholders WRITE setShouldRenderModelEntityPlaceholders)
    bool shouldRenderAvatars() const { return _shouldRenderAvatars; }
    bool shouldRenderEntities() const { return _shouldRenderEntities; }
    bool shouldRenderModelEntityPlaceholders() const { return _shouldRenderModelEntityPlaceholders; }
    void setShouldRenderAvatars(bool shouldRenderAvatars);
    void setShouldRenderEntities(bool shouldRenderEntities);
    void setShouldRenderModelEntityPlaceholders(bool shouldRenderModelEntityPlaceholders);

signals:

    /*@jsdoc
     * Triggered when whether or not avatars are rendered changes.
     * @function Scene.shouldRenderAvatarsChanged
     * @param {boolean} shouldRenderAvatars - <code>true</code> if avatars are rendered, <code>false</code> if they aren't.
     * @returns {Signal}
     * @example <caption>Report when the rendering of avatars changes.</caption>
     * Scene.shouldRenderAvatarsChanged.connect(function (shouldRenderAvatars) {
     *     print("Should render avatars changed to: " + shouldRenderAvatars);
     * });
     */
    void shouldRenderAvatarsChanged(bool shouldRenderAvatars);

    /*@jsdoc
     * Triggered when whether or not entities are rendered changes.
     * @function Scene.shouldRenderEntitiesChanged
     * @param {boolean} shouldRenderEntities - <code>true</code> if entities (domain, avatar, and local) are rendered, 
     *     <code>false</code> if they aren't.
     * @returns {Signal}
     */
    void shouldRenderEntitiesChanged(bool shouldRenderEntities);

    /*@jsdoc
     * Triggered when whether or not green boxes should be rendered for missing models changes.
     * @function Scene.shouldRenderModelEntityPlaceholdersChanged
     * @param {boolean} shouldRenderModelEntityPlaceholders - <code>true</code> if green boxes should be rendered for models
     *     that haven't loaded, <code>false</code> if they shouldn't be.
     * @returns {Signal}
     */
    void shouldRenderModelEntityPlaceholdersChanged(bool shouldRenderModelEntityPlaceholders);

protected:
    bool _shouldRenderAvatars { true };
    bool _shouldRenderEntities { true };
    bool _shouldRenderModelEntityPlaceholders { true };
};

#endif // hifi_SceneScriptingInterface_h 

/// @}
