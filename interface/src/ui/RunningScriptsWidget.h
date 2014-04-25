//
//  RunningScriptsWidget.h
//  interface/src/ui
//
//  Created by Mohammed Nafees on 03/28/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RunningScriptsWidget_h
#define hifi_RunningScriptsWidget_h

#include "FramelessDialog.h"
#include "ScriptsTableWidget.h"

namespace Ui {
    class RunningScriptsWidget;
}

class RunningScriptsWidget : public FramelessDialog
{
    Q_OBJECT
public:
    explicit RunningScriptsWidget(QWidget *parent = 0);
    ~RunningScriptsWidget();

    void setRunningScripts(const QStringList& list);

signals:
    void stopScriptName(const QString& name);

protected:
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void paintEvent(QPaintEvent *);

public slots:
    void setBoundary(const QRect& rect);

private slots:
    void stopScript(int row, int column);
    void loadScript(int row, int column);
    void allScriptsStopped();

private:
    Ui::RunningScriptsWidget *ui;
    ScriptsTableWidget *_runningScriptsTable;
    ScriptsTableWidget *_recentlyLoadedScriptsTable;
    QStringList _recentlyLoadedScripts;
    QString _lastStoppedScript;
    QRect _boundary;
    bool _mousePressed;
    QPoint _mousePosition;

    void createRecentlyLoadedScriptsTable();
};

#endif // hifi_RunningScriptsWidget_h
