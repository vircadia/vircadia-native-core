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

class QCheckBox;
class QPushButton;
class QResizeEvent;
class AbstractLoggerInterface;

class LogDialog : public BaseLogDialog {
    Q_OBJECT

public:
    LogDialog(QWidget* parent, AbstractLoggerInterface* logger);

private slots:
    void handleRevealButton();
    void handleExtraDebuggingCheckbox(int);

protected:
    void resizeEvent(QResizeEvent* event) override;
    QString getCurrentLog() override;
    
private:
    QCheckBox* _extraDebuggingBox;
    QPushButton* _revealLogButton;

    AbstractLoggerInterface* _logger;
};

#endif // hifi_LogDialog_h
