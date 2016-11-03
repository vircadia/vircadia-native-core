//
//  Billboard3DOverlay.h
//  hifi/interface/src/ui/overlays
//
//  Created by Zander Otavka on 8/4/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Billboard3DOverlay_h
#define hifi_Billboard3DOverlay_h

#include "Planar3DOverlay.h"
#include "PanelAttachable.h"
#include "Billboardable.h"

class Billboard3DOverlay : public Planar3DOverlay, public PanelAttachable, public Billboardable {
    Q_OBJECT

public:
    Billboard3DOverlay() {}
    Billboard3DOverlay(const Billboard3DOverlay* billboard3DOverlay);

    void setProperties(const QVariantMap& properties) override;
    QVariant getProperty(const QString& property) override;

protected:
    virtual void applyTransformTo(Transform& transform, bool force = false) override;
};

#endif // hifi_Billboard3DOverlay_h
