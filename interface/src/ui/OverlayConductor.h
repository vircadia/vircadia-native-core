//
//  OverlayConductor.h
//  interface/src/ui
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OverlayConductor_h
#define hifi_OverlayConductor_h

class OverlayConductor {
public:
    OverlayConductor();
    ~OverlayConductor();

    void update(float dt);
    void setEnabled(bool enable);
    bool getEnabled() const;

private:
    void updateMode();

    enum Mode {
        FLAT,
        SITTING,
        STANDING
    };

    Mode _mode { FLAT };
    bool _enabled { false };
};

#endif
