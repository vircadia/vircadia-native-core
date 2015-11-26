//
//  MainWindow.cpp
//  StackManagerQt/src/ui
//
//  Created by Mohammed Nafees on 10/17/14.
//  Copyright (c) 2014 High Fidelity. All rights reserved.
//

#include "MainWindow.h"

#include <QClipboard>
#include <QPainter>
#include <QApplication>
#include <QDesktopWidget>
#include <QFrame>
#include <QDesktopServices>
#include <QMessageBox>
#include <QMutex>
#include <QLayoutItem>
#include <QCursor>
#include <QtWebKitWidgets/qwebview.h>

#include "AppDelegate.h"
#include "AssignmentWidget.h"
#include "GlobalData.h"
#include "StackManagerVersion.h"

const int GLOBAL_X_PADDING = 55;
const int TOP_Y_PADDING = 25;
const int REQUIREMENTS_TEXT_TOP_MARGIN = 19;
//const int HORIZONTAL_RULE_TOP_MARGIN = 25;

const int BUTTON_PADDING_FIX = -5;

//const int ASSIGNMENT_LAYOUT_RESIZE_FACTOR = 56;
//const int ASSIGNMENT_LAYOUT_WIDGET_STRETCH = 0;

const QColor lightGrayColor = QColor(205, 205, 205);
const QColor darkGrayColor = QColor(84, 84, 84);
const QColor redColor = QColor(189, 54, 78);
const QColor greenColor = QColor(3, 150, 126);

const QString SHARE_BUTTON_COPY_LINK_TEXT = "Copy link";

MainWindow::MainWindow() :
    QWidget(),
    _domainServerRunning(false),
    _startServerButton(NULL),
    _stopServerButton(NULL),
    _serverAddressLabel(NULL),
    _viewLogsButton(NULL),
    _settingsButton(NULL),
    _copyLinkButton(NULL),
    _contentSetButton(NULL),
    _logsWidget(NULL),
    _localHttpPortSharedMem(NULL)
{
    // Set build version
    QCoreApplication::setApplicationVersion(BUILD_VERSION);

    setWindowTitle("High Fidelity Stack Manager (build " + QCoreApplication::applicationVersion() + ")");
    const int WINDOW_FIXED_WIDTH = 640;
    const int WINDOW_INITIAL_HEIGHT = 170;

    if (GlobalData::getInstance().getPlatform() == "win") {
        const int windowsYCoord = 30;
        setGeometry(qApp->desktop()->availableGeometry().width() / 2 - WINDOW_FIXED_WIDTH / 2, windowsYCoord,
                    WINDOW_FIXED_WIDTH, WINDOW_INITIAL_HEIGHT);
    } else if (GlobalData::getInstance().getPlatform() == "linux") {
        const int linuxYCoord = 30;
        setGeometry(qApp->desktop()->availableGeometry().width() / 2 - WINDOW_FIXED_WIDTH / 2, linuxYCoord,
                    WINDOW_FIXED_WIDTH, WINDOW_INITIAL_HEIGHT + 40);
    } else {
        const int unixYCoord = 0;
        setGeometry(qApp->desktop()->availableGeometry().width() / 2 - WINDOW_FIXED_WIDTH / 2, unixYCoord,
                    WINDOW_FIXED_WIDTH, WINDOW_INITIAL_HEIGHT);
    }
    setFixedWidth(WINDOW_FIXED_WIDTH);
    setMaximumHeight(qApp->desktop()->availableGeometry().height());
    setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint |
                   Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
    setMouseTracking(true);
    setStyleSheet("font-family: 'Helvetica', 'Arial', 'sans-serif';");

    const int SERVER_BUTTON_HEIGHT = 47;

    _startServerButton = new SvgButton(this);

    QPixmap scaledStart(":/server-start.svg");
    scaledStart.scaledToHeight(SERVER_BUTTON_HEIGHT);

    _startServerButton->setGeometry((width() / 2.0f) - (scaledStart.width() / 2.0f),
                                    TOP_Y_PADDING,
                                    scaledStart.width(),
                                    scaledStart.height());
    _startServerButton->setSvgImage(":/server-start.svg");

    _stopServerButton = new SvgButton(this);
    _stopServerButton->setSvgImage(":/server-stop.svg");
    _stopServerButton->setGeometry(GLOBAL_X_PADDING, TOP_Y_PADDING,
                                   scaledStart.width(), scaledStart.height());

    const int SERVER_ADDRESS_LABEL_LEFT_MARGIN = 20;
    const int SERVER_ADDRESS_LABEL_TOP_MARGIN = 17;
    _serverAddressLabel = new QLabel(this);
    _serverAddressLabel->move(_stopServerButton->geometry().right() + SERVER_ADDRESS_LABEL_LEFT_MARGIN,
                              TOP_Y_PADDING + SERVER_ADDRESS_LABEL_TOP_MARGIN);
    _serverAddressLabel->setOpenExternalLinks(true);

    const int SECONDARY_BUTTON_ROW_TOP_MARGIN = 10;

    int secondaryButtonY = _stopServerButton->geometry().bottom() + SECONDARY_BUTTON_ROW_TOP_MARGIN;

    _viewLogsButton = new QPushButton("View logs", this);
    _viewLogsButton->adjustSize();
    _viewLogsButton->setGeometry(GLOBAL_X_PADDING + BUTTON_PADDING_FIX, secondaryButtonY,
                                 _viewLogsButton->width(), _viewLogsButton->height());

    _settingsButton = new QPushButton("Settings", this);
    _settingsButton->adjustSize();
    _settingsButton->setGeometry(_viewLogsButton->geometry().right(), secondaryButtonY,
                                 _settingsButton->width(), _settingsButton->height());

    _copyLinkButton = new QPushButton(SHARE_BUTTON_COPY_LINK_TEXT, this);
    _copyLinkButton->adjustSize();
    _copyLinkButton->setGeometry(_settingsButton->geometry().right(), secondaryButtonY,
                              _copyLinkButton->width(), _copyLinkButton->height());

    // add the drop down for content sets
    _contentSetButton = new QPushButton("Get content set", this);
    _contentSetButton->adjustSize();
    _contentSetButton->setGeometry(_copyLinkButton->geometry().right(), secondaryButtonY,
                                   _contentSetButton->width(), _contentSetButton->height());

    const QSize logsWidgetSize = QSize(500, 500);
    _logsWidget = new QTabWidget;
    _logsWidget->setUsesScrollButtons(true);
    _logsWidget->setElideMode(Qt::ElideMiddle);
    _logsWidget->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint |
                                Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    _logsWidget->resize(logsWidgetSize);

    connect(_startServerButton, &QPushButton::clicked, this, &MainWindow::toggleDomainServerButton);
    connect(_stopServerButton, &QPushButton::clicked, this, &MainWindow::toggleDomainServerButton);
    connect(_copyLinkButton, &QPushButton::clicked, this, &MainWindow::handleCopyLinkButton);
    connect(_contentSetButton, &QPushButton::clicked, this, &MainWindow::showContentSetPage);
    connect(_viewLogsButton, &QPushButton::clicked, _logsWidget, &QTabWidget::show);
    connect(_settingsButton, &QPushButton::clicked, this, &MainWindow::openSettings);

    AppDelegate* app = AppDelegate::getInstance();
    // update the current server address label and change it if the AppDelegate says the address has changed
    updateServerAddressLabel();
    connect(app, &AppDelegate::domainAddressChanged, this, &MainWindow::updateServerAddressLabel);
    connect(app, &AppDelegate::domainAddressChanged, this, &MainWindow::updateServerBaseUrl);

    // handle response for content set download
    connect(app, &AppDelegate::contentSetDownloadResponse, this, &MainWindow::handleContentSetDownloadResponse);

    // handle response for index path change
    connect(app, &AppDelegate::indexPathChangeResponse, this, &MainWindow::handleIndexPathChangeResponse);

    // handle stack state change
    connect(app, &AppDelegate::stackStateChanged, this, &MainWindow::toggleContent);

    toggleContent(false);

}

void MainWindow::updateServerAddressLabel() {
    AppDelegate* app = AppDelegate::getInstance();

    _serverAddressLabel->setText("<html><head/><body style=\"font:14pt 'Helvetica', 'Arial', 'sans-serif';"
                                 "font-weight: bold;\"><p><span style=\"color:#545454;\">Accessible at: </span>"
                                 "<a href=\"" + app->getServerAddress() + "\">"
                                 "<span style=\"color:#29957e;\">" + app->getServerAddress() +
                                 "</span></a></p></body></html>");
    _serverAddressLabel->adjustSize();
}

void MainWindow::updateServerBaseUrl() {
    quint16 localPort;

    if (getLocalServerPortFromSharedMemory("domain-server.local-http-port", _localHttpPortSharedMem, localPort)) {
        GlobalData::getInstance().setDomainServerBaseUrl(QString("http://localhost:") + QString::number(localPort));
    }
}


void MainWindow::handleCopyLinkButton() {
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(AppDelegate::getInstance()->getServerAddress());
}

void MainWindow::showContentSetPage() {
    const QString CONTENT_SET_HTML_URL = "http://hifi-public.s3.amazonaws.com/content-sets/content-sets.html";

    // show a QWebView for the content set page
    QWebView* contentSetWebView = new QWebView();
    contentSetWebView->setUrl(CONTENT_SET_HTML_URL);

    // have the widget delete on close
    contentSetWebView->setAttribute(Qt::WA_DeleteOnClose);

    // setup the page viewport to be the right size
    const QSize CONTENT_SET_VIEWPORT_SIZE = QSize(800, 480);
    contentSetWebView->resize(CONTENT_SET_VIEWPORT_SIZE);

    // have our app delegate handle a click on one of the content sets
    contentSetWebView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    connect(contentSetWebView->page(), &QWebPage::linkClicked, AppDelegate::getInstance(), &AppDelegate::downloadContentSet);
    connect(contentSetWebView->page(), &QWebPage::linkClicked, contentSetWebView, &QWebView::close);

    contentSetWebView->show();
}

void MainWindow::handleContentSetDownloadResponse(bool wasSuccessful) {
    if (wasSuccessful) {
        QMessageBox::information(this, "New content set",
                                 "Your new content set has been downloaded and your assignment-clients have been restarted.");
    } else {
        QMessageBox::information(this, "Error", "There was a problem downloading that content set. Please try again!");
    }
}

void MainWindow::handleIndexPathChangeResponse(bool wasSuccessful) {
    if (!wasSuccessful) {
        QString errorMessage = "The content set was downloaded successfully but there was a problem changing your \
            domain-server index path.\n\nIf you want users to jump to the new content set when they come to your domain \
            please try and re-download the content set.";
        QMessageBox::information(this, "Error", errorMessage);
    }
}

void MainWindow::setRequirementsLastChecked(const QString& lastCheckedDateTime) {
    _requirementsLastCheckedDateTime = lastCheckedDateTime;
}

void MainWindow::setUpdateNotification(const QString& updateNotification) {
    _updateNotification = updateNotification;
}

void MainWindow::toggleContent(bool isRunning) {
    _stopServerButton->setVisible(isRunning);
    _startServerButton->setVisible(!isRunning);
    _domainServerRunning = isRunning;
    _serverAddressLabel->setVisible(isRunning);
    _viewLogsButton->setVisible(isRunning);
    _settingsButton->setVisible(isRunning);
    _copyLinkButton->setVisible(isRunning);
    _contentSetButton->setVisible(isRunning);
    update();
}

void MainWindow::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QFont font("Helvetica");
    font.insertSubstitutions("Helvetica", QStringList() << "Arial" << "sans-serif");

    int currentY = (_domainServerRunning ? _viewLogsButton->geometry().bottom() : _startServerButton->geometry().bottom())
        + REQUIREMENTS_TEXT_TOP_MARGIN;

    if (!_updateNotification.isEmpty()) {
        font.setBold(true);
        font.setUnderline(false);
        if (GlobalData::getInstance().getPlatform() == "linux") {
            font.setPointSize(14);
        }
        painter.setFont(font);
        painter.setPen(redColor);

        QString updateNotificationString = ">>> " + _updateNotification + " <<<";
        float fontWidth = QFontMetrics(font).width(updateNotificationString) + GLOBAL_X_PADDING;

        painter.drawText(QRectF(_domainServerRunning ? ((width() - fontWidth) / 2.0f) : GLOBAL_X_PADDING,
                                currentY,
                                fontWidth,
                                QFontMetrics(font).height()),
                                updateNotificationString);
    }
    else if (!_requirementsLastCheckedDateTime.isEmpty()) {
        font.setBold(false);
        font.setUnderline(false);
        if (GlobalData::getInstance().getPlatform() == "linux") {
            font.setPointSize(14);
        }
        painter.setFont(font);
        painter.setPen(darkGrayColor);

        QString requirementsString = "Requirements are up to date as of " + _requirementsLastCheckedDateTime;
        float fontWidth = QFontMetrics(font).width(requirementsString);

        painter.drawText(QRectF(_domainServerRunning ? GLOBAL_X_PADDING : ((width() - fontWidth)/ 2.0f),
                                currentY,
                                fontWidth,
                                QFontMetrics(font).height()),
                                "Requirements are up to date as of " + _requirementsLastCheckedDateTime);
    }
}

void MainWindow::toggleDomainServerButton() {
    AppDelegate::getInstance()->toggleStack(!_domainServerRunning);
}

void MainWindow::openSettings() {
    QDesktopServices::openUrl(QUrl(GlobalData::getInstance().getDomainServerBaseUrl() + "/settings/"));
}


// XXX this code is duplicate of LimitedNodeList::getLocalServerPortFromSharedMemory
bool MainWindow::getLocalServerPortFromSharedMemory(const QString key, QSharedMemory*& sharedMem, quint16& localPort) {
    if (!sharedMem) {
        sharedMem = new QSharedMemory(key, this);

        if (!sharedMem->attach(QSharedMemory::ReadOnly)) {
            qWarning() << "Could not attach to shared memory at key" << key;
        }
    }

    if (sharedMem->isAttached()) {
        sharedMem->lock();
        memcpy(&localPort, sharedMem->data(), sizeof(localPort));
        sharedMem->unlock();
        return true;
    }

    return false;
}
