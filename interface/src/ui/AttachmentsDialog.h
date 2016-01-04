//
//  AttachmentsDialog.h
//  interface/src/ui
//
//  Created by Andrzej Kapolka on 5/4/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AttachmentsDialog_h
#define hifi_AttachmentsDialog_h

#include <QDialog>
#include <QFrame>

#include <AvatarData.h>

class QComboBox;
class QDoubleSpinner;
class QLineEdit;
class QVBoxLayout;

/// Allows users to edit the avatar attachments.
class AttachmentsDialog : public QDialog {
    Q_OBJECT

public:
    AttachmentsDialog(QWidget* parent = nullptr);

    virtual void setVisible(bool visible);

public slots:

    void updateAttachmentData();

private slots:

    void addAttachment(const AttachmentData& data = AttachmentData());

private:

    QVBoxLayout* _attachments;
    QPushButton* _ok;
};

/// A panel controlling a single attachment.
class AttachmentPanel : public QFrame {
    Q_OBJECT

public:

    AttachmentPanel(AttachmentsDialog* dialog, const AttachmentData& data = AttachmentData());

    AttachmentData getAttachmentData() const;

private slots:

    void chooseModelURL();
    void setModelURL(const QString& url);
    void modelURLChanged();
    void jointNameChanged();
    void updateAttachmentData();

private:

    void applyAttachmentData(const AttachmentData& attachment);

    AttachmentsDialog* _dialog;
    QLineEdit* _modelURL;
    QComboBox* _jointName;
    QDoubleSpinBox* _translationX;
    QDoubleSpinBox* _translationY;
    QDoubleSpinBox* _translationZ;
    QDoubleSpinBox* _rotationX;
    QDoubleSpinBox* _rotationY;
    QDoubleSpinBox* _rotationZ;
    QDoubleSpinBox* _scale;
    QCheckBox* _isSoft;
    bool _applying;
};

#endif // hifi_AttachmentsDialog_h
