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

public:
    static AnimStats* getInstance();

    AnimStats(QQuickItem* parent = nullptr);

    void updateStats(bool force = false);

    QStringList animAlphaValues() { return _animAlphaValues; }
    QStringList animVars() { return _animVarsList; }
    QStringList animStateMachines() { return _animStateMachines; }

public slots:
    void forceUpdateStats() { updateStats(true); }

signals:

    void animAlphaValuesChanged();
    void animVarsChanged();
    void animStateMachinesChanged();

private:
    QStringList _animAlphaValues;
    AnimContext::DebugAlphaMap _prevDebugAlphaMap;  // alpha values from previous frame
    std::map<QString, qint64> _animAlphaValueChangedTimers; // last time alpha value has changed

    QStringList _animVarsList;
    std::map<QString, QString> _prevAnimVars;  // anim vars from previous frame
    std::map<QString, qint64> _animVarChangedTimers; // last time animVar value has changed.

    QStringList _animStateMachines;
};

#endif // hifi_AnimStats_h
