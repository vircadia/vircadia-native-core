//
//  ImageComparer.cpp
//
//  Created by Nissim Hadar on 18 Nov 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "ImageComparer.h"
#include "common.h"

#include <cmath>

// Computes SSIM - see https://en.wikipedia.org/wiki/Structural_similarity
// The value is computed for the luminance component and the average value is returned
double ImageComparer::compareImages(QImage resultImage, QImage expectedImage) const {
    const int L = 255; // (2^number of bits per pixel) - 1

    const double K1 { 0.01 };
    const double K2 { 0.03 };
    const double c1 = pow((K1 * L), 2);
    const double c2 = pow((K2 * L), 2);

    // First go over all full 8x8 blocks
    // This is done in 3 loops
    //      1) Read the pixels into a linear array (an optimization)
    //      2) Calculate mean
    //      3) Calculate variance and covariance
    //
    // p - pixel in expected image
    // q - pixel in result image
    //
    const int WIN_SIZE = 8;
    int x{ 0 };             // column index (start of block)
    int y{ 0 };             // row index (start of block
    
    // Pixels are processed in square blocks
    double p[WIN_SIZE * WIN_SIZE];
    double q[WIN_SIZE * WIN_SIZE];

    int windowCounter{ 0 };
    double ssim{ 0.0 };
    while (x < expectedImage.width()) {
        int lastX = x + WIN_SIZE - 1;
        if (lastX > expectedImage.width() - 1) {
            x -= (lastX - expectedImage.width());
        }

        while (y < expectedImage.height()) {
            int lastY = y + WIN_SIZE - 1;
            if (lastY > expectedImage.height() - 1) {
                y -= (lastY - expectedImage.height());
            }

            // Collect pixels into linear arrays
            int i{ 0 };
            for (int xx = 0; xx < WIN_SIZE && x + xx < expectedImage.width(); ++xx) {
                for (int yy = 0; yy < WIN_SIZE && y + yy < expectedImage.height(); ++yy) {
                    // Get pixels
                    QRgb pixelP = expectedImage.pixel(QPoint(x + xx, y + yy));
                    QRgb pixelQ = resultImage.pixel(QPoint(x + xx, y + yy));

                    // Convert to luminance
                    p[i] = R_Y * qRed(pixelP) + G_Y * qGreen(pixelP) + B_Y * qBlue(pixelP);
                    q[i] = R_Y * qRed(pixelQ) + G_Y * qGreen(pixelQ) + B_Y * qBlue(pixelQ);

                    ++i;
                }
            }
  
            // Calculate mean
            double mP{ 0.0 };    // average value of expected pixel
            double mQ{ 0.0 };    // average value of result pixel
            for (int j = 0; j < WIN_SIZE * WIN_SIZE; ++j) {
                mP += p[j];
                mQ += q[j];
            }
            mP /= (WIN_SIZE * WIN_SIZE);
            mQ /= (WIN_SIZE * WIN_SIZE);

            // Calculate variance and covariance
            double sigsqP{ 0.0 };
            double sigsqQ{ 0.0 };
            double sigPQ{ 0.0 };
            for (int j = 0; j < WIN_SIZE * WIN_SIZE; ++j) {
                sigsqP += pow((p[j] - mP), 2);
                sigsqQ += pow((q[j] - mQ), 2);

                sigPQ += (p[j] - mP) * (q[j] - mQ);
            }
            sigsqP /= (WIN_SIZE * WIN_SIZE);
            sigsqQ /= (WIN_SIZE * WIN_SIZE);
            sigPQ /= (WIN_SIZE * WIN_SIZE);

            double numerator = (2.0 * mP * mQ + c1) * (2.0 * sigPQ + c2);
            double denominator = (mP * mP + mQ * mQ + c1) * (sigsqP + sigsqQ + c2);

            ssim += numerator / denominator;
            ++windowCounter;

            y += WIN_SIZE;
        }

        x += WIN_SIZE;
        y = 0;
    }

    return ssim / windowCounter;
};