//
//  SvgButton.h
//  StackManagerQt/src/ui
//
//  Created by Mohammed Nafees on 10/20/14.
//  Copyright (c) 2014 High Fidelity. All rights reserved.
//

#ifndef hifi_SvgButton_h
#define hifi_SvgButton_h

#include <QWidget>
#include <QAbstractButton>
#include <QResizeEvent>

class SvgButton : public QAbstractButton
{
    Q_OBJECT
public:
    explicit SvgButton(QWidget* parent = 0);

    void setSvgImage(const QString& svg);

protected:
    virtual void enterEvent(QEvent*);
    virtual void paintEvent(QPaintEvent*);

private:
    QString _svgImage;

};

#endif
