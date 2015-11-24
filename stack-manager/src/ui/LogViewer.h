//
//  LogViewer.h
//  StackManagerQt/src/ui
//
//  Created by Mohammed Nafees on 07/10/14.
//  Copyright (c) 2014 High Fidelity. All rights reserved.
//

#ifndef hifi_LogViewer_h
#define hifi_LogViewer_h

#include <QWidget>
#include <QTextEdit>

class LogViewer : public QWidget
{
    Q_OBJECT
public:
    explicit LogViewer(QWidget* parent = 0);
    
    void clear();

    void appendStandardOutput(const QString& output);
    void appendStandardError(const QString& error);

private:
    QTextEdit* _outputView;
    QTextEdit* _errorView;
};

#endif
