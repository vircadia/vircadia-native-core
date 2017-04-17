//
//  ResultsWindow.h
//  tools/oven/src/ui
//
//  Created by Stephen Birarda on 4/14/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ResultsWindow_h
#define hifi_ResultsWindow_h

#include <QtCore/QDir>
#include <QtWidgets/QWidget>

class QTableWidget;

class ResultsWindow : public QWidget {
    Q_OBJECT

public:
    ResultsWindow(QWidget* parent = nullptr);

    void setupUI();

    int addPendingResultRow(const QString& fileName, const QDir& outputDirectory);
    void changeStatusForRow(int rowIndex, const QString& result);

private slots:
    void handleCellClicked(int rowIndex, int columnIndex);

private:
    QTableWidget* _resultsTable { nullptr };
    QList<QDir> _outputDirectories;
};

#endif // hifi_ResultsWindow_h
