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
#include <ui/TabletScriptingInterface.h>

#include "EntityScriptingInterface.h"
#include "ui/overlays/Image3DOverlay.h"
#include "ui/overlays/Cube3DOverlay.h"
#include "ui/overlays/Overlays.h"
#include "scripting/HMDScriptingInterface.h"

#include "EntityTree.h"
#include "ContextOverlayLogging.h"

/**jsdoc
* @namespace ContextOverlay
*/
class ContextOverlayInterface : public QObject, public Dependency  {
    Q_OBJECT

    Q_PROPERTY(QUuid entityWithContextOverlay READ getCurrentEntityWithContextOverlay WRITE setCurrentEntityWithContextOverlay)
    Q_PROPERTY(bool enabled READ getEnabled WRITE setEnabled);
    QSharedPointer<EntityScriptingInterface> _entityScriptingInterface;
    EntityPropertyFlags _entityPropertyFlags;
    QSharedPointer<HMDScriptingInterface> _hmdScriptingInterface;
    QSharedPointer<TabletScriptingInterface> _tabletScriptingInterface;
    OverlayID _contextOverlayID { UNKNOWN_OVERLAY_ID };
    OverlayID _bbOverlayID { UNKNOWN_OVERLAY_ID };
    std::shared_ptr<Image3DOverlay> _contextOverlay { nullptr };
    std::shared_ptr<Cube3DOverlay> _bbOverlay { nullptr };
public:
    ContextOverlayInterface();

    Q_INVOKABLE QUuid getCurrentEntityWithContextOverlay() { return _currentEntityWithContextOverlay; }
    void setCurrentEntityWithContextOverlay(const QUuid& entityID) { _currentEntityWithContextOverlay = entityID; }
    void setEnabled(bool enabled) { _enabled = enabled; }
    bool getEnabled() { return _enabled; }

public slots:
    bool createOrDestroyContextOverlay(const EntityItemID& entityItemID, const PointerEvent& event);
    bool destroyContextOverlay(const EntityItemID& entityItemID, const PointerEvent& event);
    bool destroyContextOverlay(const EntityItemID& entityItemID);
    void clickContextOverlay(const OverlayID& overlayID, const PointerEvent& event);

private:
    bool _verboseLogging { true };
    bool _enabled { true };
    QUuid _currentEntityWithContextOverlay{};
    QString _entityMarketplaceID;

    void openMarketplace();
};

#endif // hifi_ContextOverlayInterface_h
