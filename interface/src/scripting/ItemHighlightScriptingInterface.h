
//  ItemHighlightScriptingInterface.h
//  interface/src/scripting
//
//  Created by Zach Fox on 2017-08-22.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ItemHighlightScriptingInterface_h
#define hifi_ItemHighlightScriptingInterface_h

#include <QtCore/QObject>
#include <DependencyManager.h>

#include <AbstractViewStateInterface.h>

#include "EntityItemID.h"
#include "ui/overlays/Overlay.h"

class ItemHighlightScriptingInterface : public QObject, public Dependency {
    Q_OBJECT

public:
    ItemHighlightScriptingInterface(AbstractViewStateInterface* viewState);

    Q_INVOKABLE bool addToHighlightedItemsList(const EntityItemID& entityID);
    Q_INVOKABLE bool removeFromHighlightedItemsList(const EntityItemID& entityID);
    
    Q_INVOKABLE bool addToHighlightedItemsList(const OverlayID& overlayID);
    Q_INVOKABLE bool removeFromHighlightedItemsList(const OverlayID& overlayID);

//signals:

private:
    AbstractViewStateInterface* _viewState;
    render::ItemIDs _highlightedItemsList;

    bool addToHighlightedItemsList(render::ItemID idToAdd);
    bool removeFromHighlightedItemsList(render::ItemID idToRemove);

    void updateRendererHighlightList();
};

#endif // hifi_ItemHighlightScriptingInterface_h
