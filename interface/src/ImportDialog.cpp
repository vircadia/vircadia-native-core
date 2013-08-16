//
//  ImportDialog.cpp
//  hifi
//
//  Created by Clement Brisset on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
#include "ImportDialog.h"

#include <QStandardPaths>
#include <QGridLayout>

const QString WINDOW_NAME                         = QObject::tr("Import Voxels");
const QString IMPORT_TO_CLIPBOARD_CHECKBOX_STRING = QObject::tr("Import into clipboard");
const QString PREVIEW_CHECKBOX_STRING             = QObject::tr("Load preview");
const QString IMPORT_FILE_TYPES                   = QObject::tr("Sparse Voxel Octree Files, "
                                                                "Square PNG, "
                                                                "Schematic Files "
                                                                "(*.svo *.png *.schematic)");

const QString DESKTOP_LOCATION = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);

const glm::vec3 UP = glm::vec3(0, 1, 0);
const float ANGULAR_RATE = 0.02f;

class GLWidget : public QGLWidget {
public:
    GLWidget(QWidget* parent = NULL, VoxelSystem* voxelSystem = NULL);
    void setDraw(bool draw) {_draw = draw;}
    void setTargetCenter(glm::vec3 targetCenter) {_targetCenter = targetCenter;}

protected:
    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
    virtual void paintGL();

private:
    VoxelSystem* _voxelSystem;

    bool _draw;

    float _a;
    float _h;
    glm::vec3 _targetCenter;
};

GLWidget::GLWidget(QWidget *parent, VoxelSystem *voxelSystem)
    : QGLWidget(parent),
      _voxelSystem(voxelSystem),
      _draw(false),
      _a(0.0f),
      _h(0.0f),
      _targetCenter(0.5f, 0.5f, 0.5f) {
}

void GLWidget::initializeGL() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glShadeModel (GL_SMOOTH);
    glEnable(GL_DEPTH_TEST);

    if(_voxelSystem) {
        _voxelSystem->init();
    }

}

void GLWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, (float) w / h, 0.25f, 500);
}

void GLWidget::paintGL() {
    glEnable(GL_LINE_SMOOTH);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    _a += ANGULAR_RATE;
    gluLookAt(_targetCenter.x + (glm::length(_targetCenter) + 0.5f) * cos((double) _a),
              _targetCenter.y * 1.25f,
              _targetCenter.z + (glm::length(_targetCenter) + 0.5f) * sin((double) _a),
              _targetCenter.x, _targetCenter.y, _targetCenter.z,
              UP.x, UP.y, UP.z);


    if (_draw && _voxelSystem) {
        glBegin(GL_LINES);
        glColor3d(1, 1 ,1);
        glVertex3d(0, 0, 0);
        glVertex3d(1, 0, 0);

        glVertex3d(0, 0, 0);
        glVertex3d(0, 1, 0);

        glVertex3d(0, 0, 0);
        glVertex3d(0, 0, 1);


        glColor3d(0.4f, 0.4f ,0.4f);
        glVertex3d(2 * _targetCenter.x, 2 * _targetCenter.y,2 *  _targetCenter.z);
        glVertex3d(0                  , 2 * _targetCenter.y, 2 * _targetCenter.z);

        glVertex3d(2 * _targetCenter.x, 2 * _targetCenter.y, 2 * _targetCenter.z);
        glVertex3d(2 * _targetCenter.x, 0                  , 2 * _targetCenter.z);

        glVertex3d(2 * _targetCenter.x, 2 * _targetCenter.y, 2 * _targetCenter.z);
        glVertex3d(2 * _targetCenter.x, 2 * _targetCenter.y, 0                  );
        glEnd();

        _voxelSystem->render(false);
    }
}

ImportDialog::ImportDialog(QWidget *parent, VoxelSystem* voxelSystem)
    : QFileDialog(parent, WINDOW_NAME, DESKTOP_LOCATION, IMPORT_FILE_TYPES),
      _clipboardImportBox   (IMPORT_TO_CLIPBOARD_CHECKBOX_STRING, this),
      _previewBox           (PREVIEW_CHECKBOX_STRING, this),
      _previewBar           (this),
      _glPreview            (new GLWidget(this, voxelSystem)) {

    setOption(QFileDialog::DontUseNativeDialog, true);
    setFileMode(QFileDialog::ExistingFile);
    setViewMode(QFileDialog::Detail);

    QGridLayout* gridLayout = (QGridLayout*) layout();
    gridLayout->addWidget(&_clipboardImportBox, 2, 3);
    gridLayout->addWidget(&_previewBox        , 3, 3);
    gridLayout->addWidget(&_previewBar        , 0, 3);
    gridLayout->addWidget(_glPreview          , 1, 3);
    gridLayout->setColumnStretch(3, 1);

    _previewBar.setVisible(false);
    _previewBar.setRange(0, 100);
    _previewBar.setValue(0);

    connect(&_previewBox, SIGNAL(toggled(bool)), SLOT(preview(bool)));
    connect(this, SIGNAL(currentChanged(QString)), SLOT(saveCurrentFile(QString)));
    connect(&_glTimer, SIGNAL(timeout()), SLOT(timer()));

    connect(voxelSystem->getVoxelTree(), SIGNAL(importSize(float,float,float)), SLOT(setGLCamera(float, float, float)));
    connect(voxelSystem->getVoxelTree(), SIGNAL(importProgress(int)), &_previewBar, SLOT(setValue(int)));
}

int ImportDialog::exec() {
    _previewBox.setChecked(false);
    _previewBar.setVisible(false);

    int ret = QFileDialog::exec();


    ((GLWidget*) _glPreview)->setDraw(false);
    _glTimer.stop();
    _glPreview->updateGL();

    return ret;
}

void ImportDialog::setGLCamera(float x, float y, float z) {
    ((GLWidget*) _glPreview)->setTargetCenter(glm::vec3(x, y, z) / 2.0f);
}

void ImportDialog::reset() {
    _previewBar.setValue(0);
}

void ImportDialog::preview(bool wantPreview) {
    _previewBar.setValue(0);
    _previewBar.setVisible(wantPreview);
    ((GLWidget*) _glPreview)->setDraw(wantPreview);

    if (wantPreview) {
        emit previewActivated(_currentFile);
        _glTimer.start();
    } else {
        _glTimer.stop();
        _glPreview->updateGL();
    }
}

void ImportDialog::saveCurrentFile(QString filename) {
    _currentFile = filename;
}

void ImportDialog::timer() {
    _glPreview->updateGL();
    _glTimer.start(16);
}
