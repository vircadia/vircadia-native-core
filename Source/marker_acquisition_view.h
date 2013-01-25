//
//  marker_acquisition_view.h
//  interface
//
//  Created by Kenneth Keiter on 12/12/12.
//  Copyright (c) 2012 Rosedale Lab. All rights reserved.
//

#ifndef __interface__marker_acquisition_view__
#define __interface__marker_acquisition_view__

#include <iostream>
#include <vector.h>
#include <opencv2/opencv.hpp>
#include <GLUT/glut.h>
#include <CVBlob/blob.h>
#include <CVBlob/BlobResult.h>
#include "markers.h"

#define ACQ_VIEW_RETICLE_RADIUS 10

class MarkerAcquisitionView{
public:
    MarkerCapture* capture;
    CvScalar reticle_color;
    CvScalar acquired_color;
    bool visible = false;
    
    MarkerAcquisitionView(MarkerCapture* marker_capture_instance);
    
    void show();
    void hide();
    void handle_key(unsigned char k);
    void render(IplImage* frame);
    
private:
    void draw_targeting_reticle(GLfloat x, GLfloat y, GLfloat r, CvScalar color);
    CvScalar mean_color(IplImage* img, int x, int y, int width, int height);
};

#endif /* defined(__interface__marker_acquisition_view__) */
