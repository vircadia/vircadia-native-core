//
//  Created by Anthony J. Thibault on 2016-12-12
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TabletScriptingInterface_h
#define hifi_TabletScriptingInterface_h

#include <QtCore/QObject>

#include <DependencyManager.h>

/**jsdoc
 * @namespace Tablet
 */
class TabletScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
public:
    /**jsdoc
     * Creates a new button on the tablet ui and returns it.
     * @function Tablet.getTablet
     * @param name {String} tablet name
     * @return {QmlWrapper} tablet instance
     */
    Q_INVOKABLE QObject* getTablet(const QString& tabletId);
};

#endif // hifi_TabletScriptingInterface_h
