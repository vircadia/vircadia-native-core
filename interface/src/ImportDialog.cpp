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
#include <QApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

const QString WINDOW_NAME = QObject::tr("Import Voxels");
const QString IMPORT_BUTTON_NAME = QObject::tr("Import");
const QString IMPORT_INFO = QObject::tr("<b>Import</b> %1 as voxels");
const QString CANCEL_BUTTON_NAME = QObject::tr("Cancel");
const QString INFO_LABEL_TEXT = QObject::tr("<div style='line-height:20px;'>"
                                            "This will load the selected file into Hifi and allow you<br/>"
                                            "to place it with %1-V; you must be in select or<br/>"
                                            "add mode (S or V keys will toggle mode) to place.</div>");

const QString DESKTOP_LOCATION = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
const int SHORT_FILE_EXTENSION = 4;
const int SECOND_INDEX_LETTER = 1;

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
        case QFileIconProvider::Network:
        case QFileIconProvider::Drive:
        case QFileIconProvider::Folder:
            typeString = "folder";
            break;
            
        default:
            typeString = "file";
            break;
    }

    return QIcon("resources/icons/" + typeString + ".svg");
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
    
    QFileInfo iconFile("resources/icons/" + iconsMap[ext]);
    if (iconFile.exists() && iconFile.isFile()) {
        return QIcon(iconFile.filePath());
    }
    
    return QIcon("resources/icons/file.svg");
}

QString HiFiIconProvider::type(const QFileInfo &info) const {
    if (info.isFile()) {
        if (info.suffix().size() > SHORT_FILE_EXTENSION) {
            // Capitalize extension
            return info.suffix().left(SECOND_INDEX_LETTER).toUpper() + info.suffix().mid(SECOND_INDEX_LETTER);
        }
        return info.suffix().toUpper();
    }

    return QFileIconProvider::type(info);
}

ImportDialog::ImportDialog(QWidget* parent) :
    QFileDialog(parent, WINDOW_NAME, DESKTOP_LOCATION, NULL),
    _importButton(IMPORT_BUTTON_NAME, this),
    _cancelButton(CANCEL_BUTTON_NAME, this) {
    
    setOption(QFileDialog::DontUseNativeDialog, true);
    setFileMode(QFileDialog::ExistingFile);
    setViewMode(QFileDialog::Detail);

#ifdef Q_OS_MAC
    QString cmdString = ("Command");
#else
    QString cmdString = ("Control");
#endif
    QLabel* infoLabel = new QLabel(QString(INFO_LABEL_TEXT).arg(cmdString));
    infoLabel->setObjectName("infoLabel");
    
    QGridLayout* gridLayout = (QGridLayout*) layout();
    gridLayout->addWidget(infoLabel, 2, 0, 2, 1);
    gridLayout->addWidget(&_cancelButton, 2, 1, 2, 1);
    gridLayout->addWidget(&_importButton, 2, 2, 2, 1);
    
    setImportTypes();
    setLayout();

    connect(&_importButton, SIGNAL(pressed()), SLOT(import()));
    connect(&_cancelButton, SIGNAL(pressed()), SLOT(close()));
    connect(this, SIGNAL(currentChanged(QString)), SLOT(saveCurrentFile(QString)));
}

ImportDialog::~ImportDialog() {
    deleteLater();
}

void ImportDialog::import() {
    emit accepted();
    close();
}

void ImportDialog::accept() {
    QFileDialog::accept();
}

void ImportDialog::reject() {
    QFileDialog::reject();
}

int ImportDialog::exec() {
    // deselect selected file
    selectFile(" ");
    return QFileDialog::exec();
}

void ImportDialog::reset() {
    _importButton.setEnabled(false);
}

void ImportDialog::saveCurrentFile(QString filename) {
    if (!filename.isEmpty() && QFileInfo(filename).isFile()) {
        _currentFile = filename;
        _importButton.setEnabled(true);
    } else {
        _currentFile.clear();
        _importButton.setEnabled(false);
    }
}

void ImportDialog::setLayout() {
    
    // set ObjectName used in qss for styling
    _importButton.setObjectName("importButton");
    _cancelButton.setObjectName("cancelButton");
    
    // set fixed size
    _importButton.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    _cancelButton.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    
    // hide unused embedded widgets in QFileDialog
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
    
    widget = findChild<QWidget*>("fileTypeCombo");
    widget->hide();
    
    widget = findChild<QWidget*>("fileTypeLabel");
    widget->hide();
    
    widget = findChild<QWidget*>("fileNameLabel");
    widget->hide();
    
    widget = findChild<QWidget*>("buttonBox");
    widget->hide();
 
    QSplitter* splitter = findChild<QSplitter*>("splitter");
    splitter->setHandleWidth(0);
    
    // remove blue outline on Mac
    widget = findChild<QWidget*>("sidebar");
    widget->setAttribute(Qt::WA_MacShowFocusRect, false);
    
    widget = findChild<QWidget*>("treeView");
    widget->setAttribute(Qt::WA_MacShowFocusRect, false);
    
    // remove reference to treeView
    widget = NULL;
    widget->deleteLater();
    
    switchToResourcesParentIfRequired();
    QFile styleSheet("resources/styles/import_dialog.qss");
    if (styleSheet.open(QIODevice::ReadOnly)) {
        setStyleSheet(styleSheet.readAll());
    }
    
}

void ImportDialog::setImportTypes() {

    switchToResourcesParentIfRequired();
    QFile config("resources/config/config.json");
    config.open(QFile::ReadOnly | QFile::Text);
    QJsonDocument document = QJsonDocument::fromJson(config.readAll());
    if (!document.isNull() && !document.isEmpty()) {

        QString importFormatsInfo;
        QString importFormatsFilterList;
        QHash<QString, QString> iconsMap;

        QJsonObject configObject = document.object();
        if (!configObject.isEmpty()) {
            QJsonArray fileFormats = configObject["importFormats"].toArray();
            int formatsCounter = 0;
            foreach (const QJsonValue& fileFormat, fileFormats) {
                QJsonObject fileFormatObject = fileFormat.toObject();

                QString ext(fileFormatObject["extension"].toString());
                QString description(fileFormatObject.value("description").toString());
                QString icon(fileFormatObject.value("icon").toString());

                if (formatsCounter > 0) {
                    importFormatsInfo.append(",");
                }

                // set ' or' on last import type text
                if (formatsCounter == fileFormats.count() - 1) {
                    importFormatsInfo.append(" or");
                }

                importFormatsFilterList.append(QString("*.%1 ").arg(ext));
                importFormatsInfo.append(" .").append(ext);
                iconsMap[ext] = icon;
                formatsCounter++;
            }
        }

        // set custom file icons
        setIconProvider(new HiFiIconProvider(iconsMap));
        setNameFilter(importFormatsFilterList);
        
#ifdef Q_OS_MAC
        QString cmdString = ("Command");
#else
        QString cmdString = ("Control");
#endif
        setLabelText(QFileDialog::LookIn, QString(IMPORT_INFO).arg(importFormatsInfo));
    }
}
