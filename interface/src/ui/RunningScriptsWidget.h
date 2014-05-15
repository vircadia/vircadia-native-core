//
//  RunningScriptsWidget.h
//  interface/src/ui
//
//  Created by Mohammed Nafees on 03/28/2014.
//  Updated by Ryan Huffman on 05/13/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RunningScriptsWidget_h
#define hifi_RunningScriptsWidget_h

#include <QFileSystemModel>
#include <QSortFilterProxyModel>

#include "ScriptsModel.h"
#include "FramelessDialog.h"
#include "ScriptsTableWidget.h"

namespace Ui {
    class RunningScriptsWidget;
}

class RunningScriptsWidget : public FramelessDialog
{
    Q_OBJECT
public:
    explicit RunningScriptsWidget(QWidget* parent = NULL);
    ~RunningScriptsWidget();

    void setRunningScripts(const QStringList& list);

signals:
    void stopScriptName(const QString& name);

protected:
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void paintEvent(QPaintEvent* event);

public slots:
    void scriptStopped(const QString& scriptName);
    void setBoundary(const QRect& rect);

private slots:
    void stopScript(int row, int column);
    void loadScript(int row, int column);
    void allScriptsStopped();
    void updateFileFilter(const QString& filter);
    void scriptFileSelected(const QModelIndex& index);

private:
    Ui::RunningScriptsWidget* ui;
    QSortFilterProxyModel _proxyModel;
    ScriptsModel _scriptsModel;
    ScriptsTableWidget* _runningScriptsTable;
    ScriptsTableWidget* _recentlyLoadedScriptsTable;
    QStringList _recentlyLoadedScripts;
    QString _lastStoppedScript;
    QRect _boundary;

    void createRecentlyLoadedScriptsTable();
};

#endif // hifi_RunningScriptsWidget_h
