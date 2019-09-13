#include <QQuickView>
#include <QPoint>
#include <memory>

class LauncherWindow : public QQuickView {
public:
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    //void setLauncherState(std::shared_ptr<LauncherState> launcherState) { _launcherState = launcherState; }
private:
    bool _drag { false };
    QPoint _previousMousePos;

    ///std::shared_ptr<LauncherState> _launcherState { nullptr };
};
