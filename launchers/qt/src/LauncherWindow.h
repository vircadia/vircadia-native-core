#include "LauncherState.h"
#include <QQuickView>
#include <QPoint>
#include <memory>

class LauncherWindow : public QQuickView {
public:
    LauncherWindow();
    ~LauncherWindow() = default;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void setLauncherStatePtr(std::shared_ptr<LauncherState> launcherState) { _launcherState = launcherState; }

private:
    bool _drag { false };
    QPoint _previousMousePos;

    std::shared_ptr<LauncherState> _launcherState { nullptr };
};
