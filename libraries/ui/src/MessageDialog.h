//
//  MessageDialog.h
//
//  Created by Bradley Austin Davis on 2015/04/14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_MessageDialog_h
#define hifi_MessageDialog_h

#include "OffscreenQmlDialog.h"

class MessageDialog : public OffscreenQmlDialog
{
    Q_OBJECT
    HIFI_QML_DECL

private:
    Q_ENUMS(Icon)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QString informativeText READ informativeText WRITE setInformativeText NOTIFY informativeTextChanged)
    Q_PROPERTY(QString detailedText READ detailedText WRITE setDetailedText NOTIFY detailedTextChanged)
    Q_PROPERTY(Icon icon READ icon WRITE setIcon NOTIFY iconChanged)
    Q_PROPERTY(StandardButtons standardButtons READ standardButtons WRITE setStandardButtons NOTIFY standardButtonsChanged)
    Q_PROPERTY(StandardButton clickedButton READ clickedButton NOTIFY buttonClicked)

public:
    enum Icon { NoIcon, Information, Warning, Critical, Question };

    enum ButtonRole {
        // keep this in sync with QDialogButtonBox::ButtonRole and QPlatformDialogHelper::ButtonRole
        InvalidRole = -1,
        AcceptRole,
        RejectRole,
        DestructiveRole,
        ActionRole,
        HelpRole,
        YesRole,
        NoRole,
        ResetRole,
        ApplyRole,

        NRoles
    };

    MessageDialog(QQuickItem* parent = 0);
    virtual ~MessageDialog();

    QString text() const;
    QString informativeText() const;
    QString detailedText() const;
    Icon icon() const;

public slots:
    virtual void setVisible(bool v);
    void setText(const QString& arg);
    void setInformativeText(const QString& arg);
    void setDetailedText(const QString& arg);
    void setIcon(Icon icon);
    void setStandardButtons(StandardButtons buttons);
    void setResultCallback(OffscreenUi::ButtonCallback callback);
    void click(StandardButton button);
    StandardButtons standardButtons() const;
    StandardButton clickedButton() const;

signals:
    void textChanged();
    void informativeTextChanged();
    void detailedTextChanged();
    void iconChanged();
    void standardButtonsChanged();
    void buttonClicked();
    void discard();
    void help();
    void yes();
    void no();
    void apply();
    void reset();

protected slots:
    virtual void click(StandardButton button, ButtonRole);
    virtual void accept();
    virtual void reject();

private:
    QString _title;
    QString _text;
    QString _informativeText;
    QString _detailedText;
    Icon _icon{ Information };
    StandardButtons _buttons;
    StandardButton _clickedButton{ NoButton };
    OffscreenUi::ButtonCallback _resultCallback;
};

#endif // hifi_MessageDialog_h
