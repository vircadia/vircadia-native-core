//
//  MismatchWindow.h
//
//  Created by Nissim Hadar on 9 Nov 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_MismatchWindow_h
#define hifi_MismatchWindow_h

#include "ui_MismatchWindow.h"

#include "../common.h"

class MismatchWindow : public QDialog, public Ui::MismatchWindow {
    Q_OBJECT

public:
    MismatchWindow(QWidget *parent = Q_NULLPTR);

    void setTestResult(TestResult testResult);

    UserResponse getUserResponse() { return _userResponse; }

    QPixmap computeDiffPixmap(QImage expectedImage, QImage resultImage);
    QPixmap getComparisonImage();

private slots:
    void on_passTestButton_clicked();
    void on_failTestButton_clicked();
    void on_abortTestsButton_clicked();

private:
    UserResponse _userResponse{ USER_RESPONSE_INVALID };

    QPixmap _diffPixmap;
};


#endif // hifi_MismatchWindow_h