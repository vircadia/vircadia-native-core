//
//  FramelessDialog.h
//  hifi
//
//  Created by Stojce Slavkovski on 2/20/14.
//
//

#ifndef __hifi__FramelessDialog__
#define __hifi__FramelessDialog__

#include <QDialog>
#include <QPainter>
#include <QStyleOptionTitleBar>
#include <QGridLayout>
#include <QLabel>

class FramelessDialog : public QDialog {
    Q_OBJECT
    
public:
    FramelessDialog(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~FramelessDialog();

};

#endif /* defined(__hifi__FramelessDialog__) */
