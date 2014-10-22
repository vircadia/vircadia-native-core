//
//  MetavoxelNetworkSimulator.h
//  interface/src/ui
//
//  Created by Andrzej Kapolka on 10/20/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MetavoxelNetworkSimulator_h
#define hifi_MetavoxelNetworkSimulator_h

#include <QWidget>

class QDoubleSpinBox;
class QSpinBox;

/// Allows tweaking network simulation (packet drop percentage, etc.) settings for metavoxels.
class MetavoxelNetworkSimulator : public QWidget {
    Q_OBJECT

public:
    
    MetavoxelNetworkSimulator();

private slots:

    void updateMetavoxelSystem();

private:
    
    QDoubleSpinBox* _dropRate;
    QDoubleSpinBox* _repeatRate;
    QSpinBox* _minimumDelay;
    QSpinBox* _maximumDelay;
    QSpinBox* _bandwidthLimit;
};

#endif // hifi_MetavoxelNetworkSimulator_h
