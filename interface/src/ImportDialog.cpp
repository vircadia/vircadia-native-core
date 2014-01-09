//
//  ImportDialog.cpp
//  hifi
//
//  Created by Clement Brisset on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
#include "ImportDialog.h"

#include <QStandardPaths>
#include <QGridLayout>
#include <QSplitter>
#include <QFile>
#include <QFileInfo>
#include <QMetaEnum>

#include <QDir>

#include <QApplication>
#include <QStyle>

const QString WINDOW_NAME = QObject::tr("Import Voxels");
const QString IMPORT_BUTTON_NAME = QObject::tr("Import");
const QString IMPORT_INFO = QObject::tr("<b>Import</b> .svo, .schematic, or .png as voxels");
const QString CANCEL_BUTTON_NAME = QObject::tr("Cancel");
const QString IMPORT_FILE_TYPES = QObject::tr("Sparse Voxel Octree Files, "
                                              "Square PNG, "
                                              "Schematic Files "
                                              "(*.svo *.png *.schematic)");
const QString INFO_LABEL_TEXT = QObject::tr("This will load selected file into Hifi and\n"
                                            "allow you to place it with Command V");

const QString DESKTOP_LOCATION = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);

QIcon HiFiIconProvider::icon(QFileIconProvider::IconType type) const {
    
    switchToResourcesParentIfRequired();
    
    // types
    // Computer, Desktop, Trashcan, Network, Drive, Folder, File

    QString typeString;
    
    switch (type) {
        case QFileIconProvider::Computer:
            typeString = "computer";
            break;
            
        case QFileIconProvider::Desktop:
            typeString = "desktop";
            break;
        case QFileIconProvider::Trashcan:
            typeString = "folder";
            break;
            
        case QFileIconProvider::Network:
            typeString = "folder";
            break;
            
        case QFileIconProvider::Drive:
            typeString = "folder";
            break;
            
        case QFileIconProvider::Folder:
            typeString = "folder";
            break;
            
        default:
            typeString = "file";
            break;
    }
    
    QIcon ico = QIcon("resources/icons/" + typeString + ".svg");
    ico.pixmap(QSize(50, 50));
    return ico;
}

QIcon HiFiIconProvider::icon(const QFileInfo &info) const {
    switchToResourcesParentIfRequired();
    const QString ext = info.suffix().toLower();
    
    if (info.isDir()) {
        if (info.absoluteFilePath() == QDir::homePath()) {
            return QIcon("resources/icons/home.svg");
        } else if (info.absoluteFilePath() == DESKTOP_LOCATION) {
            return QIcon("resources/icons/desktop.svg");
        } else if (info.absoluteFilePath() == QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)) {
            return QIcon("resources/icons/documents.svg");
        }
        return QIcon("resources/icons/folder.svg");
    }
    
    QFileInfo iconFile = QFileInfo("resources/icons/" + ext + ".svg");
    qDebug() << "Icon type: " << iconFile.filePath();
    if (iconFile.exists()) {
        return QIcon(iconFile.filePath());
    }
    
    return QIcon("resources/icons/file.svg");
}

ImportDialog::ImportDialog(QWidget *parent) : QFileDialog(parent, WINDOW_NAME, DESKTOP_LOCATION, IMPORT_FILE_TYPES),
_importButton(IMPORT_BUTTON_NAME, this),
_cancelButton(CANCEL_BUTTON_NAME, this),
_infoLabel(INFO_LABEL_TEXT) {
    
    setOption(QFileDialog::DontUseNativeDialog, true);
    setFileMode(QFileDialog::ExistingFile);
    setViewMode(QFileDialog::Detail);

    setLayout();
    QLabel* _importLabel = findChild<QLabel*>("lookInLabel");
    _importLabel->setText(IMPORT_INFO);

    QGridLayout* gridLayout = (QGridLayout*) layout();
    gridLayout->addWidget(&_infoLabel, 2, 0);
    gridLayout->addWidget(&_cancelButton, 2, 1);
    gridLayout->addWidget(&_importButton, 2, 2);

    connect(&_importButton, SIGNAL(pressed()), SLOT(import()));
    connect(this, SIGNAL(currentChanged(QString)), SLOT(saveCurrentFile(QString)));
    
    resize(QSize(790, 477));
}

ImportDialog::~ImportDialog() {
}

void ImportDialog::import() {
    _importButton.setDisabled(true);
    emit accepted();
}

void ImportDialog::accept() {
    QFileDialog::accept();
}

void ImportDialog::reject() {
    QFileDialog::reject();
}

int ImportDialog::exec() {
    return QFileDialog::exec();
}

void ImportDialog::reset() {
    _importButton.setEnabled(true);
}

void ImportDialog::saveCurrentFile(QString filename) {
    _currentFile = filename;
}

void ImportDialog::setLayout() {
    
    // set ObjectName used in qss
    _importButton.setObjectName("importButton");
    _cancelButton.setObjectName("cancelButton");
    
    // set size policy used in
    _importButton.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    _cancelButton.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // hide unused embeded widgets in QFileDialog
    QWidget* widget = findChild<QWidget*>("lookInCombo");
    widget->hide();
    
    widget = findChild<QWidget*>("backButton");
    widget->hide();
    
    widget = findChild<QWidget*>("forwardButton");
    widget->hide();
    
    widget = findChild<QWidget*>("toParentButton");
    widget->hide();
    
    widget = findChild<QWidget*>("newFolderButton");
    widget->hide();
    
    widget = findChild<QWidget*>("listModeButton");
    widget->hide();
    
    widget = findChild<QWidget*>("detailModeButton");
    widget->hide();
    
    widget = findChild<QWidget*>("fileNameEdit");
    widget->hide();
    
    widget = findChild<QWidget*>("fileNameLabel");
    widget->hide();
    
    widget = findChild<QWidget*>("fileTypeCombo");
    widget->hide();
    
    widget = findChild<QWidget*>("fileTypeLabel");
    widget->hide();
    
    widget = findChild<QWidget*>("buttonBox");
    widget->hide();
 
    QSplitter *splitter = findChild<QSplitter*>("splitter");
    splitter->setHandleWidth(0);
    
    // remove blue outline on Mac
    widget = findChild<QWidget*>("sidebar");
    widget->setAttribute(Qt::WA_MacShowFocusRect, false);
    
    widget = findChild<QWidget*>("treeView");
    widget->setAttribute(Qt::WA_MacShowFocusRect, false);
    
    // set custom file icons
    setIconProvider(new HiFiIconProvider());
    
    switchToResourcesParentIfRequired();
    QFile styleSheet("resources/styles/import_dialog.qss");
    if (styleSheet.open(QIODevice::ReadOnly)) {
        setStyleSheet(styleSheet.readAll());
    }
    
}
