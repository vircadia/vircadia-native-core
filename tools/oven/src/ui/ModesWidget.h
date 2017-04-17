//
//  ModesWidget.h
//  tools/oven/src/ui
//
//  Created by Stephen Birarda on 4/7/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModesWidget_h
#define hifi_ModesWidget_h

#include <QtWidgets/QWidget>

class ModesWidget : public QWidget {
    Q_OBJECT
public:
    ModesWidget(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

private slots:
    void showModelBakingWidget();
    void showDomainBakingWidget();
    void showSkyboxBakingWidget();

private:
    void setupUI();
};

#endif // hifi_ModesWidget_h
