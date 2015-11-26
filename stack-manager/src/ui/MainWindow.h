//
//  MainWindow.h
//  StackManagerQt/src/ui
//
//  Created by Mohammed Nafees on 10/17/14.
//  Copyright (c) 2014 High Fidelity. All rights reserved.
//

#ifndef hifi_MainWindow_h
#define hifi_MainWindow_h

#include <QComboBox>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QSharedMemory>


#include "SvgButton.h"

class MainWindow : public QWidget {
    Q_OBJECT
public:
    MainWindow();

    void setRequirementsLastChecked(const QString& lastCheckedDateTime);
    void setUpdateNotification(const QString& updateNotification);
    QTabWidget* getLogsWidget() { return _logsWidget; }
    bool getLocalServerPortFromSharedMemory(const QString key, QSharedMemory*& sharedMem, quint16& localPort);

protected:
    virtual void paintEvent(QPaintEvent*);

private slots:
    void toggleDomainServerButton();
    void openSettings();
    void updateServerAddressLabel();
    void updateServerBaseUrl();
    void handleCopyLinkButton();
    void showContentSetPage();

    void handleContentSetDownloadResponse(bool wasSuccessful);
    void handleIndexPathChangeResponse(bool wasSuccessful);
private:
    void toggleContent(bool isRunning);

    bool _domainServerRunning;

    QString _requirementsLastCheckedDateTime;
    QString _updateNotification;
    SvgButton* _startServerButton;
    SvgButton* _stopServerButton;
    QLabel* _serverAddressLabel;
    QPushButton* _viewLogsButton;
    QPushButton* _settingsButton;
    QPushButton* _copyLinkButton;
    QPushButton* _contentSetButton;
    QTabWidget* _logsWidget;

    QSharedMemory* _localHttpPortSharedMem; // memory shared with domain server
};

#endif
