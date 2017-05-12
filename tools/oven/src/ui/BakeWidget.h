//
//  BakeWidget.h
//  tools/oven/src/ui
//
//  Created by Stephen Birarda on 4/17/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BakeWidget_h
#define hifi_BakeWidget_h

#include <QtWidgets/QWidget>

#include "../Baker.h"

class BakeWidget : public QWidget {
    Q_OBJECT
public:
    BakeWidget(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
    ~BakeWidget();

    void cancelButtonClicked();

protected:
    using BakerRowPair = std::pair<std::unique_ptr<Baker>, int>;
    using BakerRowPairList = std::list<BakerRowPair>;
    BakerRowPairList _bakers;
};

#endif // hifi_BakeWidget_h
