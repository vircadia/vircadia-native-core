#include <QQuickView>
#include <QPoint>

class LauncherWindow : public QQuickView {
public:
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    bool _drag { false };
    QPoint _previousMousePos;
};
