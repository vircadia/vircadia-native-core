//
//  Created by Anthony J. Thibault 2018/08/06
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimStats_h
#define hifi_AnimStats_h

#include <OffscreenQmlElement.h>
#include <AnimContext.h>

class AnimStats : public QQuickItem {
    Q_OBJECT
    HIFI_QML_DECL

    Q_PROPERTY(QStringList animAlphaValues READ animAlphaValues NOTIFY animAlphaValuesChanged)
    Q_PROPERTY(QStringList animVars READ animVars NOTIFY animVarsChanged)
    Q_PROPERTY(QStringList animStateMachines READ animStateMachines NOTIFY animStateMachinesChanged)
    Q_PROPERTY(QString positionText READ positionText NOTIFY positionTextChanged)
    Q_PROPERTY(QString rotationText READ rotationText NOTIFY rotationTextChanged)
    Q_PROPERTY(QString velocityText READ velocityText NOTIFY velocityTextChanged)
    Q_PROPERTY(QString recenterText READ recenterText NOTIFY recenterTextChanged)
    Q_PROPERTY(QString sittingText READ sittingText NOTIFY sittingTextChanged)
    Q_PROPERTY(QString walkingText READ walkingText NOTIFY walkingTextChanged)

public:
    static AnimStats* getInstance();

    AnimStats(QQuickItem* parent = nullptr);

    void updateStats(bool force = false);

    QStringList animAlphaValues() const { return _animAlphaValues; }
    QStringList animVars() const { return _animVarsList; }
    QStringList animStateMachines() const { return _animStateMachines; }

    QString positionText() const { return _positionText; }
    QString rotationText() const { return _rotationText; }
    QString velocityText() const { return _velocityText; }
    QString recenterText() const { return _recenterText; }
    QString sittingText() const { return _sittingText; }
    QString walkingText() const { return _walkingText; }

public slots:
    void forceUpdateStats() { updateStats(true); }

signals:

    void animAlphaValuesChanged();
    void animVarsChanged();
    void animStateMachinesChanged();
    void positionTextChanged();
    void rotationTextChanged();
    void velocityTextChanged();
    void recenterTextChanged();
    void sittingTextChanged();
    void walkingTextChanged();

private:
    QStringList _animAlphaValues;
    AnimContext::DebugAlphaMap _prevDebugAlphaMap;  // alpha values from previous frame
    std::map<QString, qint64> _animAlphaValueChangedTimers; // last time alpha value has changed

    QStringList _animVarsList;
    std::map<QString, QString> _prevAnimVars;  // anim vars from previous frame
    std::map<QString, qint64> _animVarChangedTimers; // last time animVar value has changed.

    QStringList _animStateMachines;

    QString _positionText;
    QString _rotationText;
    QString _velocityText;
    QString _recenterText;
    QString _sittingText;
    QString _walkingText;
};

#endif // hifi_AnimStats_h
