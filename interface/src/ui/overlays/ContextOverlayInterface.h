//
//  ContextOverlayInterface.h
//  interface/src/ui/overlays
//
//  Created by Zach Fox on 2017-07-14.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_ContextOverlayInterface_h
#define hifi_ContextOverlayInterface_h

#include <QtCore/QObject>
#include <QUuid>

#include <DependencyManager.h>
#include <PointerEvent.h>
#include "EntityScriptingInterface.h"
#include "ui/overlays/Image3DOverlay.h"
#include "ui/overlays/Overlays.h"

#include "EntityTree.h"
#include "ContextOverlayLogging.h"

/**jsdoc
* @namespace ContextOverlay
*/
class ContextOverlayInterface : public QObject, public Dependency  {
    Q_OBJECT

    Q_PROPERTY(QUuid entityWithContextOverlay READ getCurrentEntityWithContextOverlay WRITE setCurrentEntityWithContextOverlay)
    QSharedPointer<EntityScriptingInterface> _entityScriptingInterface;
    EntityPropertyFlags _entityPropertyFlags;
    OverlayID _contextOverlayID { UNKNOWN_OVERLAY_ID };
    std::shared_ptr<Image3DOverlay> _contextOverlay { nullptr };
public:
    ContextOverlayInterface();

    Q_INVOKABLE QUuid getCurrentEntityWithContextOverlay() { return _currentEntityWithContextOverlay; }
    void setCurrentEntityWithContextOverlay(const QUuid& entityID) { _currentEntityWithContextOverlay = entityID; }

public slots:
    void createContextOverlay(const EntityItemID& entityItemID, const PointerEvent& event);
    void destroyContextOverlay(const EntityItemID& entityItemID, const PointerEvent& event);
    void destroyContextOverlay(const EntityItemID& entityItemID);
    void clickContextOverlay(const OverlayID& overlayID, const PointerEvent& event);

private:
    bool _verboseLogging { true };
    QUuid _currentEntityWithContextOverlay{};

};

#endif // hifi_ContextOverlayInterface_h
