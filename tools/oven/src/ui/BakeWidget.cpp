//
//  BakeWidget.cpp
//  tools/oven/src/ui
//
//  Created by Stephen Birarda on 4/17/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtWidgets/QStackedWidget>

#include "../Oven.h"
#include "OvenMainWindow.h"

#include "BakeWidget.h"

BakeWidget::BakeWidget(QWidget* parent, Qt::WindowFlags flags) :
	QWidget(parent, flags)
{

}

BakeWidget::~BakeWidget() {
    // if we're going down, our bakers are about to too
    // enumerate them, send a cancelled status to the results table, and remove them
    auto it = _bakers.begin();
    while (it != _bakers.end()) {
        auto resultRow = it->second;
        auto resultsWindow = qApp->getMainWindow()->showResultsWindow();

        resultsWindow->changeStatusForRow(resultRow, "Cancelled");

        it = _bakers.erase(it);
    }
}

void BakeWidget::cancelButtonClicked() {
    // the user wants to go back to the mode selection screen
    // remove ourselves from the stacked widget and call delete later so we'll be cleaned up
    auto stackedWidget = qobject_cast<QStackedWidget*>(parentWidget());
    stackedWidget->removeWidget(this);

    this->deleteLater();
}
