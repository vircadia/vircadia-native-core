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
#include <QGridLayout>
#include <QString>
#include <QPainter>
#include <QStyleOptionTitleBar>

class FramelessDialog : public QDialog {
    Q_OBJECT
    
public:
    FramelessDialog(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~FramelessDialog();
    void setStyleSheetFile(const QString& fileName);

protected:
    virtual void mouseMoveEvent(QMouseEvent* mouseEvent);
    virtual void mousePressEvent(QMouseEvent* mouseEvent);
    virtual void mouseReleaseEvent(QMouseEvent* mouseEvent);
    virtual void showEvent(QShowEvent* event);

private:
    bool _isResizing;

};

#endif /* defined(__hifi__FramelessDialog__) */
