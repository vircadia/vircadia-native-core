//
//  AssetClient.h
//  libraries/networking/src
//
//  Created by Ryan Huffman on 2015/07/21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_networking_ResourceScriptingInterface_h
#define hifi_networking_ResourceScriptingInterface_h

#include <QtCore/QObject>

#include <DependencyManager.h>

/*@jsdoc
 * The <code>Resources</code> API enables the default location for different resource types to be overridden.
 *
 * @namespace Resources
 * 
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 * @hifi-server-entity
 * @hifi-assignment-client
 */

class ResourceScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
public:

    /*@jsdoc
     * Overrides a path prefix with an alternative path.
     * @function Resources.overrideUrlPrefix
     * @param {string} prefix - The path prefix to override, e.g., <code>"atp:/"</code>.
     * @param {string} replacement - The replacement path for the prefix.
     */
    Q_INVOKABLE void overrideUrlPrefix(const QString& prefix, const QString& replacement);

    /*@jsdoc
     * Restores the default path for a specified prefix.
     * @function Resources.restoreUrlPrefix
     * @param {string} prefix - The prefix of the resource to restore the path for.
     */
    Q_INVOKABLE void restoreUrlPrefix(const QString& prefix) {
        overrideUrlPrefix(prefix, "");
    }
};


#endif
