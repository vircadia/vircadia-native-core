//
//  MarkerAcquisitionView.cpp
//  interface
//
//  Created by Kenneth Keiter on 12/12/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include "MarkerAcquisitionView.h"
#include <GLUT/glut.h>
#include <opencv2/opencv.hpp>

MarkerAcquisitionView::MarkerAcquisitionView(MarkerCapture* marker_capture_instance){
    capture = marker_capture_instance;
}

void MarkerAcquisitionView::show(){
    visible = true;
}

void MarkerAcquisitionView::hide(){
    visible = false;
}

void MarkerAcquisitionView::handle_key(unsigned char k){
    if(visible){
        switch(k){
            case 'a': // acquire new color target.
                acquired_color = reticle_color;
                capture->acquire_color(acquired_color);
                break;
        }
    }
}

void MarkerAcquisitionView::render(IplImage* frame){
    if(visible){
        GLint m_viewport[4];
        glGetIntegerv(GL_VIEWPORT, m_viewport);
        
        // display the image in the center of the display.
        CvPoint origin = cvPoint((m_viewport[2] / 2)-(frame->width / 2), (m_viewport[3] / 2)-(frame->height / 2));
        capture->glDrawIplImage(frame, origin.x, origin.y, 1.0, -1.0);
        
        // determine the average color within the reticle.
        reticle_color = mean_color(frame,
                                   frame->width / 2 - ACQ_VIEW_RETICLE_RADIUS,
                                   frame->height / 2 - ACQ_VIEW_RETICLE_RADIUS,
                                   ACQ_VIEW_RETICLE_RADIUS * 2, ACQ_VIEW_RETICLE_RADIUS * 2);
        
        // draw a targeting reticle in the center of the image.
        int reticle_x = origin.x + frame->width / 2 - ACQ_VIEW_RETICLE_RADIUS;
        int reticle_y = origin.y + frame->height / 2 - ACQ_VIEW_RETICLE_RADIUS;
        draw_targeting_reticle(reticle_x, reticle_y,(GLfloat)ACQ_VIEW_RETICLE_RADIUS, reticle_color);
    }
}

/***************************************************************
 PRIVATE
 **************************************************************/

void MarkerAcquisitionView::draw_targeting_reticle(GLfloat x, GLfloat y, GLfloat r, CvScalar color){
    static const double inc = M_PI / 12;
    static const double max = 2 * M_PI;
    glBegin(GL_LINE_LOOP);
    glColor3f(color.val[2] * 0.01, color.val[1] * 0.01, color.val[0] * 0.01); // scale color values
    // printf("Reticle color R:%f G:%f B:%f\n", color.val[2], color.val[1], color.val[0]);
    glLineWidth(2.0);
    for(double d = 0; d < max; d += inc){
        glVertex2f(cos(d) * r + x, sin(d) * r + y);
    }
    glEnd();
}

CvScalar MarkerAcquisitionView::mean_color(IplImage* img, int x, int y, int width, int height){
    CvRect _roi = cvGetImageROI(img);
    cvSetImageROI(img, cvRect(x, y, width, height));
    CvScalar color = cvAvg(img);
    cvSetImageROI(img, _roi); // replace it with the original, otherwise OpenCV won't deallocate properly.
    return color;
}