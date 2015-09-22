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
#include <QSignalMapper>
#include <QSortFilterProxyModel>

#include "ScriptsModel.h"
#include "ScriptsModelFilter.h"
#include "ScriptsTableWidget.h"

namespace Ui {
    class RunningScriptsWidget;
}

class RunningScriptsWidget : public QWidget {
    Q_OBJECT
public:
    explicit RunningScriptsWidget(QWidget* parent = NULL);
    ~RunningScriptsWidget();

    void setRunningScripts(const QStringList& list);

    const ScriptsModel* getScriptsModel() { return &_scriptsModel; }

signals:
    void scriptStopped(const QString& scriptName);

protected:
    virtual bool eventFilter(QObject* sender, QEvent* event);

    virtual void keyPressEvent(QKeyEvent* event);
    virtual void showEvent(QShowEvent* event);

public slots:
    QVariantList getRunning();
    QVariantList getPublic();
    QVariantList getLocal();
    bool stopScript(const QString& name, bool restart = false);
    bool stopScriptByName(const QString& name);
    
private slots:
    void allScriptsStopped();
    void updateFileFilter(const QString& filter);
    void loadScriptFromList(const QModelIndex& index);
    void loadSelectedScript();
    void selectFirstInList();

private:
    Ui::RunningScriptsWidget* ui;
    QSignalMapper _reloadSignalMapper;
    QSignalMapper _stopSignalMapper;
    ScriptsModelFilter _scriptsModelFilter;
    ScriptsModel _scriptsModel;

    QVariantList getPublicChildNodes(TreeNodeFolder* parent);
};

#endif // hifi_RunningScriptsWidget_h
