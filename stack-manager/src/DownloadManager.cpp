//
//  DownloadManager.cpp
//  StackManagerQt/src
//
//  Created by Mohammed Nafees on 07/09/14.
//  Copyright (c) 2014 High Fidelity. All rights reserved.
//

#include "DownloadManager.h"
#include "GlobalData.h"

#include <QFileInfo>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QDebug>
#include <QVBoxLayout>
#include <QLabel>
#include <QSizePolicy>
#include <QDir>
#include <QFileInfo>
#include <QProgressBar>
#include <QMessageBox>
#include <QApplication>

DownloadManager::DownloadManager(QNetworkAccessManager* manager, QWidget* parent) :
    QWidget(parent),
    _manager(manager)
{
    setBaseSize(500, 250);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins(10, 10, 10, 10);
    QLabel* label = new QLabel;
    label->setText("Download Manager");
    label->setStyleSheet("font-size: 19px;");
    label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);

    _table = new QTableWidget;
    _table->setEditTriggers(QTableWidget::NoEditTriggers);
    _table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _table->setColumnCount(3);
    _table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    _table->setHorizontalHeaderLabels(QStringList() << "Name" << "Progress" << "Status");
    layout->addWidget(_table);

    setLayout(layout);
}

DownloadManager::~DownloadManager() {
    _downloaderHash.clear();
}

void DownloadManager::downloadFile(const QUrl& url) {
    for (int i = 0; i < _downloaderHash.size(); ++i) {
        if (_downloaderHash.keys().at(i)->getUrl() == url) {
            qDebug() << "Downloader for URL " << url << " already initialised.";
            return;
        }
    }

    Downloader* downloader = new Downloader(url);
    connect(downloader, SIGNAL(downloadCompleted(QUrl)), SLOT(onDownloadCompleted(QUrl)));
    connect(downloader, SIGNAL(downloadStarted(Downloader*,QUrl)),
            SLOT(onDownloadStarted(Downloader*,QUrl)));
    connect(downloader, SIGNAL(downloadFailed(QUrl)), SLOT(onDownloadFailed(QUrl)));
    connect(downloader, SIGNAL(downloadProgress(QUrl,int)), SLOT(onDownloadProgress(QUrl,int)));
    connect(downloader, SIGNAL(installingFiles(QUrl)), SLOT(onInstallingFiles(QUrl)));
    connect(downloader, SIGNAL(filesSuccessfullyInstalled(QUrl)), SLOT(onFilesSuccessfullyInstalled(QUrl)));
    connect(downloader, SIGNAL(filesInstallationFailed(QUrl)), SLOT(onFilesInstallationFailed(QUrl)));
    downloader->start(_manager);
}

void DownloadManager::onDownloadStarted(Downloader* downloader, const QUrl& url) {
    int rowIndex = _table->rowCount();
    _table->setRowCount(rowIndex + 1);
    QTableWidgetItem* nameItem = new QTableWidgetItem(QFileInfo(url.toString()).fileName());
    _table->setItem(rowIndex, 0, nameItem);
    QProgressBar* progressBar = new QProgressBar;
    _table->setCellWidget(rowIndex, 1, progressBar);
    QTableWidgetItem* statusItem = new QTableWidgetItem;
    if (QFile(QDir::toNativeSeparators(GlobalData::getInstance().getClientsLaunchPath() + "/" + QFileInfo(url.toString()).fileName())).exists()) {
        statusItem->setText("Updating");
    } else {
        statusItem->setText("Downloading");
    }
    _table->setItem(rowIndex, 2, statusItem);
    _downloaderHash.insert(downloader, rowIndex);
}

void DownloadManager::onDownloadCompleted(const QUrl& url) {
    _table->item(downloaderRowIndexForUrl(url), 2)->setText("Download Complete");
}

void DownloadManager::onDownloadProgress(const QUrl& url, int percentage) {
    qobject_cast<QProgressBar*>(_table->cellWidget(downloaderRowIndexForUrl(url), 1))->setValue(percentage);
}

void DownloadManager::onDownloadFailed(const QUrl& url) {
    _table->item(downloaderRowIndexForUrl(url), 2)->setText("Download Failed");
    _downloaderHash.remove(downloaderForUrl(url));
}

void DownloadManager::onInstallingFiles(const QUrl& url) {
    _table->item(downloaderRowIndexForUrl(url), 2)->setText("Installing");
}

void DownloadManager::onFilesSuccessfullyInstalled(const QUrl& url) {
    _table->item(downloaderRowIndexForUrl(url), 2)->setText("Successfully Installed");
    _downloaderHash.remove(downloaderForUrl(url));
    emit fileSuccessfullyInstalled(url);
    if (_downloaderHash.size() == 0) {
        close();
    }
}

void DownloadManager::onFilesInstallationFailed(const QUrl& url) {
    _table->item(downloaderRowIndexForUrl(url), 2)->setText("Installation Failed");
    _downloaderHash.remove(downloaderForUrl(url));
}

void DownloadManager::closeEvent(QCloseEvent*) {
    if (_downloaderHash.size() > 0) {
        QMessageBox msgBox;
        msgBox.setText("There are active downloads that need to be installed for the proper functioning of Stack Manager. Do you want to stop the downloads and exit?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        int ret = msgBox.exec();
        switch (ret) {
        case QMessageBox::Yes:
            qApp->quit();
            break;
        case QMessageBox::No:
            msgBox.close();
            break;
        }
    }
}

int DownloadManager::downloaderRowIndexForUrl(const QUrl& url) {
    QHash<Downloader*, int>::const_iterator i = _downloaderHash.constBegin();
    while (i != _downloaderHash.constEnd()) {
        if (i.key()->getUrl() == url) {
            return i.value();
        } else {
            ++i;
        }
    }

    return -1;
}

Downloader* DownloadManager::downloaderForUrl(const QUrl& url) {
    QHash<Downloader*, int>::const_iterator i = _downloaderHash.constBegin();
    while (i != _downloaderHash.constEnd()) {
        if (i.key()->getUrl() == url) {
            return i.key();
        } else {
            ++i;
        }
    }

    return NULL;
}
