//
//  LogDialog.h
//  interface/src/ui
//
//  Created by Stojce Slavkovski on 12/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_LogDialog_h
#define hifi_LogDialog_h

#include "BaseLogDialog.h"
#include <SettingHandle.h>

class QCheckBox;
class QPushButton;
class QComboBox;
class QLabel;
class QResizeEvent;
class AbstractLoggerInterface;

class LogDialog : public BaseLogDialog {
    Q_OBJECT

public:
    LogDialog(QWidget* parent, AbstractLoggerInterface* logger);

public slots:
    void appendLogLine(QString logLine) override;

private slots:
    void handleRevealButton();
    void handleShowSourceDebuggingCheckbox(int);
    void handleExtraDebuggingCheckbox(int);
    void handleKeepWindowOnTop(int);
    void handleDebugPrintBox(int);
    void handleInfoPrintBox(int);
    void handleCriticalPrintBox(int);
    void handleWarningPrintBox(int);
    void handleSuppressPrintBox(int);
    void handleFatalPrintBox(int);
    void handleUnknownPrintBox(int);
    void handleFilterDropdownChanged(int);
    void handleAllLogsButton();
    void printLogFile();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

    QString getCurrentLog() override;
    void updateMessageCount();
    

private:
    QCheckBox* _showSourceDebuggingBox;
    QCheckBox* _extraDebuggingBox;
    QCheckBox* _keepOnTopBox;
    QPushButton* _revealLogButton;
    QPushButton* _allLogsButton;
    QCheckBox* _debugPrintBox;
    QCheckBox* _infoPrintBox;
    QCheckBox* _criticalPrintBox;
    QCheckBox* _warningPrintBox;
    QCheckBox* _suppressPrintBox;
    QCheckBox* _fatalPrintBox;
    QCheckBox* _unknownPrintBox;
    QComboBox* _filterDropdown;
    QLabel* _messageCount;
    QString _filterSelection;
    QString _countLabel;
    AbstractLoggerInterface* _logger;
    Setting::Handle<QRect> _windowGeometry;
    int _count = 0;
};

#endif // hifi_LogDialog_h
