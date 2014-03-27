//
//  FramelessDialog.h
//  hifi
//
//  Created by Stojce Slavkovski on 2/20/14.
//
//

#ifndef __hifi__FramelessDialog__
#define __hifi__FramelessDialog__

#include <QDockWidget>
#include <QGridLayout>
#include <QString>
#include <QPainter>
#include <QStyleOptionTitleBar>
#include <QtCore/QTimer>

class FramelessDialog : public QDockWidget {
    Q_OBJECT
    
public:
    FramelessDialog(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    void setStyleSheetFile(const QString& fileName);

protected:
    /*
    virtual void mouseMoveEvent(QMouseEvent* mouseEvent);
    virtual void mousePressEvent(QMouseEvent* mouseEvent);
    virtual void mouseReleaseEvent(QMouseEvent* mouseEvent);
     */
    
private:
    bool _isResizing;

};

#endif /* defined(__hifi__FramelessDialog__) */
