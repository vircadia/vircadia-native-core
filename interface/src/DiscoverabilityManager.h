//
//  DiscoverabilityManager.h
//  interface/src
//
//  Created by Stephen Birarda on 2015-03-09.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DiscoverabilityManager_h
#define hifi_DiscoverabilityManager_h

#include <DependencyManager.h>
#include <SettingHandle.h>

namespace Discoverability {
    enum Mode {
        None,
        Friends,
        All
    };
}

Q_DECLARE_METATYPE(Discoverability::Mode);

class DiscoverabilityManager : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
    
public slots:
    void updateLocation();
    void removeLocation();
    
    Discoverability::Mode getDiscoverabilityMode() { return static_cast<Discoverability::Mode>(_mode.get()); }
    void setDiscoverabilityMode(Discoverability::Mode discoverabilityMode);

    void setVisibility();
    void visibilityChanged(Discoverability::Mode discoverabilityMode);

signals:
    void discoverabilityModeChanged(Discoverability::Mode discoverabilityMode);

private slots:
    void handleHeartbeatResponse(QNetworkReply& requestReply);
    
private:
    DiscoverabilityManager();
    
    Setting::Handle<int> _mode;
    QJsonObject _lastLocationObject;
};

#endif // hifi_DiscoverabilityManager_h
