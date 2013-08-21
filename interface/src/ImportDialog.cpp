//
//  ImportDialog.cpp
//  hifi
//
//  Created by Clement Brisset on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
#include "ImportDialog.h"
#include "Application.h"

#include <QStandardPaths>
#include <QGridLayout>
#include <QMouseEvent>

const QString WINDOW_NAME                         = QObject::tr("Import Voxels");
const QString IMPORT_BUTTON_NAME                  = QObject::tr("Import");
const QString IMPORT_TO_CLIPBOARD_CHECKBOX_STRING = QObject::tr("Import into clipboard");
const QString PREVIEW_CHECKBOX_STRING             = QObject::tr("Load preview");
const QString IMPORT_FILE_TYPES                   = QObject::tr("Sparse Voxel Octree Files, "
                                                                "Square PNG, "
                                                                "Schematic Files "
                                                                "(*.svo *.png *.schematic)");

const QString DESKTOP_LOCATION = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);


const glm::vec3 UP_VECT = glm::vec3(0, 1, 0);
const float ANGULAR_RATE = 0.02f;
const float VERTICAL_ANGLE = M_PI_4 / 2.0f;
const float RETURN_RATE = 0.02f;
const float NEAR_CLIP = 0.5f;
const float FAR_CLIP = 10.0f;
const float FIELD_OF_VIEW = 60.0f;

class GLWidget : public QGLWidget {
public:
    GLWidget(QWidget* parent = NULL, VoxelSystem* voxelSystem = NULL);
    void setDraw(bool draw) {_draw = draw;}
    void setTargetCenter(glm::vec3 targetCenter) {_targetCenter = targetCenter;}

protected:
    virtual void initializeGL();
    virtual void resizeGL(int width, int height);
    virtual void paintGL();

    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);

private:
    VoxelSystem* _voxelSystem;

    bool _draw;

    double _a; // horizontal angle of the camera to the center of the object
    double _h; // vertical angle of the camera to the center of the object
    glm::vec3 _targetCenter;

    bool _pressed;
    int _mouseX;
    int _mouseY;
};

GLWidget::GLWidget(QWidget *parent, VoxelSystem *voxelSystem)
    : QGLWidget(parent, Application::getInstance()->getGLWidget()),
      _voxelSystem(voxelSystem),
      _draw(false),
      _a(0.0f),
      _h(VERTICAL_ANGLE),
      _targetCenter(0.5f, 0.5f, 0.5f),
      _pressed(false),
      _mouseX(0),
      _mouseY(0) {
}

void GLWidget::initializeGL() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glShadeModel (GL_SMOOTH);
    glEnable(GL_DEPTH_TEST);
}

void GLWidget::resizeGL(int width, int height) {
    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(FIELD_OF_VIEW,
                   (float) width / height,
                   NEAR_CLIP,
                   FAR_CLIP);
}

void GLWidget::paintGL() {
    glEnable(GL_LINE_SMOOTH);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (!_pressed) {
        _a += ANGULAR_RATE;
        _h = (1.0f - RETURN_RATE) * _h + RETURN_RATE * VERTICAL_ANGLE;
    }

    gluLookAt(_targetCenter.x + (glm::length(_targetCenter) + NEAR_CLIP) * cos(_a),
              _targetCenter.y + (glm::length(_targetCenter) + NEAR_CLIP) * sin(_h),
              _targetCenter.z + (glm::length(_targetCenter) + NEAR_CLIP) * sin(_a),
              _targetCenter.x, _targetCenter.y, _targetCenter.z,
              UP_VECT.x, UP_VECT.y, UP_VECT.z);


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
        glVertex3d(2 * _targetCenter.x, 2 * _targetCenter.y, 2 * _targetCenter.z);
        glVertex3d(0                  , 2 * _targetCenter.y, 2 * _targetCenter.z);

        glVertex3d(2 * _targetCenter.x, 2 * _targetCenter.y, 2 * _targetCenter.z);
        glVertex3d(2 * _targetCenter.x, 0                  , 2 * _targetCenter.z);

        glVertex3d(2 * _targetCenter.x, 2 * _targetCenter.y, 2 * _targetCenter.z);
        glVertex3d(2 * _targetCenter.x, 2 * _targetCenter.y, 0                  );
        glEnd();

        _voxelSystem->render(false);
    }
}


void GLWidget::mousePressEvent(QMouseEvent* event) {
    _pressed = true;
    _mouseX = event->globalX();
    _mouseY = event->globalY();
}

void GLWidget::mouseMoveEvent(QMouseEvent* event) {
    _a += (M_PI * (event->globalX() - _mouseX)) / height();
    _h += (M_PI * (event->globalY() - _mouseY)) / height();
    _h = glm::clamp(_h, -M_PI_4, M_PI_4);

    _mouseX = event->globalX();
    _mouseY = event->globalY();
}

void GLWidget::mouseReleaseEvent(QMouseEvent* event) {
    _pressed = false;
}

ImportDialog::ImportDialog(QWidget *parent, VoxelSystem* voxelSystem)
    : QFileDialog(parent, WINDOW_NAME, DESKTOP_LOCATION, IMPORT_FILE_TYPES),
      _importButton      (IMPORT_BUTTON_NAME, this),
      _clipboardImportBox(IMPORT_TO_CLIPBOARD_CHECKBOX_STRING, this),
      _previewBox        (PREVIEW_CHECKBOX_STRING, this),
      _previewBar        (this),
      _glPreview         (new GLWidget(this, voxelSystem)) {

    setOption(QFileDialog::DontUseNativeDialog, true);
    setFileMode(QFileDialog::ExistingFile);
    setViewMode(QFileDialog::Detail);

    QGridLayout* gridLayout = (QGridLayout*) layout();
    gridLayout->addWidget(&_importButton      , 2, 2);
    gridLayout->addWidget(&_clipboardImportBox, 2, 3);
    gridLayout->addWidget(&_previewBox        , 3, 3);
    gridLayout->addWidget(&_previewBar        , 0, 3);
    gridLayout->addWidget(_glPreview          , 1, 3);
    gridLayout->setColumnStretch(3, 1);

    _previewBar.setVisible(false);
    _previewBar.setRange(0, 100);
    _previewBar.setValue(0);

    connect(&_importButton, SIGNAL(pressed()), SLOT(import()));
    connect(&_previewBox, SIGNAL(toggled(bool)), SIGNAL(previewToggled(bool)));
    connect(&_previewBox, SIGNAL(toggled(bool)), SLOT(preview(bool)));

    connect(this, SIGNAL(currentChanged(QString)), SLOT(saveCurrentFile(QString)));
    connect(&_glTimer, SIGNAL(timeout()), SLOT(timer()));

    connect(voxelSystem, SIGNAL(importSize(float,float,float)), SLOT(setGLCamera(float, float, float)));
    connect(voxelSystem, SIGNAL(importProgress(int)), &_previewBar, SLOT(setValue(int)));
}

ImportDialog::~ImportDialog() {
    delete _glPreview;
}

void ImportDialog::import() {
    _importButton.setDisabled(true);
    _clipboardImportBox.setDisabled(true);
    _previewBox.setDisabled(true);

    _previewBar.setValue(0);
    _previewBar.setVisible(true);

    emit accepted();
}

void ImportDialog::accept() {
    QFileDialog::accept();
}

void ImportDialog::reject() {
    QFileDialog::reject();
}

int ImportDialog::exec() {
    return QFileDialog::exec();
}

void ImportDialog::setGLCamera(float x, float y, float z) {
    _glPreview->setTargetCenter(glm::vec3(x, y, z) / 2.0f);
}

void ImportDialog::reset() {
    _previewBox.setChecked(false);
    _previewBar.setVisible(false);
    _previewBar.setValue(0);
    _importButton.setEnabled(true);
    _clipboardImportBox.setEnabled(true);
    _previewBox.setEnabled(true);

    _glTimer.stop();
    _glPreview->setDraw(false);
    _glPreview->updateGL();
}

void ImportDialog::preview(bool wantPreview) {
    _previewBar.setValue(0);
    _previewBar.setVisible(wantPreview);
    _glPreview->setDraw(wantPreview);

    if (wantPreview) {
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
