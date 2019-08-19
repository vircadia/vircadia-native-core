//
//  DockWidget.h
//  libraries/ui/src
//
//  Created by Dante Ruiz 05-07-2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_DockWidget_h
#define hifi_DockWidget_h

#include <QDockWidget>
#include <memory>

class QQuickView;
class QQuickItem;
class DockWidget : public QDockWidget {
Q_OBJECT
public:
    DockWidget(const QString& title, QWidget* parent = nullptr);
    ~DockWidget() = default;

    void setSource(const QUrl& url);
    QQuickItem* getRootItem() const;
    std::shared_ptr<QQuickView> getQuickView() const;

signals:
    void onResizeEvent();

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    std::shared_ptr<QQuickView> _quickView;
};

#endif
