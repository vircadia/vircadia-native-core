//
//  ModesWidget.cpp
//  tools/oven/src/ui
//
//  Created by Stephen Birarda on 4/7/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStackedWidget>

#include "DomainBakeWidget.h"
#include "ModelBakeWidget.h"
#include "SkyboxBakeWidget.h"

#include "ModesWidget.h"

ModesWidget::ModesWidget(QWidget* parent, Qt::WindowFlags flags) :
    QWidget(parent, flags)
{
    setupUI();
}

void ModesWidget::setupUI() {
    // setup a horizontal box layout to hold our mode buttons
    QHBoxLayout* horizontalLayout = new QHBoxLayout;

    // add a button for domain baking
    QPushButton* domainButton = new QPushButton("Bake Domain");
    connect(domainButton, &QPushButton::clicked, this, &ModesWidget::showDomainBakingWidget);
    horizontalLayout->addWidget(domainButton);

    // add a button for model baking
    QPushButton* modelsButton = new QPushButton("Bake Models");
    connect(modelsButton, &QPushButton::clicked, this, &ModesWidget::showModelBakingWidget);
    horizontalLayout->addWidget(modelsButton);

    // add a button for skybox baking
    QPushButton* skyboxButton = new QPushButton("Bake Skyboxes");
    connect(skyboxButton, &QPushButton::clicked, this, &ModesWidget::showSkyboxBakingWidget);
    horizontalLayout->addWidget(skyboxButton);

    setLayout(horizontalLayout);
}

void ModesWidget::showModelBakingWidget() {
    auto stackedWidget = qobject_cast<QStackedWidget*>(parentWidget());

    // add a new widget for model baking to the stack, and switch to it
    stackedWidget->setCurrentIndex(stackedWidget->addWidget(new ModelBakeWidget));
}

void ModesWidget::showDomainBakingWidget() {
    auto stackedWidget = qobject_cast<QStackedWidget*>(parentWidget());

    // add a new widget for domain baking to the stack, and switch to it
    stackedWidget->setCurrentIndex(stackedWidget->addWidget(new DomainBakeWidget));
}

void ModesWidget::showSkyboxBakingWidget() {
    auto stackedWidget = qobject_cast<QStackedWidget*>(parentWidget());

    // add a new widget for skybox baking to the stack, and switch to it
    stackedWidget->setCurrentIndex(stackedWidget->addWidget(new SkyboxBakeWidget));
}
