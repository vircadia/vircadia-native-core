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

#include <glm/glm.hpp>

// Computes SSIM - see https://en.wikipedia.org/wiki/Structural_similarity
// The value is computed for the luminence component and the maximum value is returned
double ImageComparer::compareImages(QImage resultImage, QImage expectedImage) const {
    const double K1{ 0.01 };
    const double K2{ 0.03 };

    // Make sure the image is 8 bits per colour
    QImage::Format format = expectedImage.format();
    if (format != QImage::Format::Format_RGB32) {
        throw -1;
    }

    const int L = 255; // (2^number of bits per pixel) - 1
    const double c1 = pow((K1 * L), 2);
    const double c2 = pow((K2 * L), 2);

    // Coefficients for luminosity calculation
    const double RED_COEFFICIENT = 0.212655f;
    const double GREEN_COEFFICIENT = 0.715158f;
    const double BLUE_COEFFICIENT = 0.072187f;

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
    double ssimMax{ 0.0 };

    while (x < expectedImage.width()) {
        int lastX = x + WIN_SIZE;
        if (lastX > expectedImage.width()) {
            x -= (lastX - expectedImage.width());
        }

        while (y < expectedImage.height()) {
            int lastY = y + WIN_SIZE;
            if (lastY > expectedImage.height()) {
                y -= (lastY - expectedImage.height());
            }

            // Collect pixels
            int i{ 0 };
            double p[WIN_SIZE * WIN_SIZE];
            double q[WIN_SIZE * WIN_SIZE];
            for (int xx = 0; xx < WIN_SIZE; ++xx) {
                for (int yy = 0; yy < WIN_SIZE; ++yy) {
                    // Get pixels
                    QRgb pixelP = expectedImage.pixel(QPoint(xx, yy));
                    QRgb pixelQ = resultImage.pixel(QPoint(xx, yy));

                    // Convert to vec3
                    p[i] = 
                        RED_COEFFICIENT * qRed(pixelP) + GREEN_COEFFICIENT * qGreen(pixelP) + BLUE_COEFFICIENT * qBlue(pixelP);

                    q[i] = 
                        RED_COEFFICIENT * qRed(pixelQ) + GREEN_COEFFICIENT * qGreen(pixelQ) + BLUE_COEFFICIENT * qBlue(pixelQ);

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
                sigsqP += (p[j] - mP) * (p[j] - mP);
                sigsqQ += (q[j] - mQ) * (q[j] - mQ);

                sigPQ += (p[j] - mP) * (q[j] - mQ);
            }
            sigsqP /= (WIN_SIZE * WIN_SIZE);
            sigsqQ /= (WIN_SIZE * WIN_SIZE);
            sigPQ /= (WIN_SIZE * WIN_SIZE);

            double numerator = (2.0 * mP * mQ + c1) * (2.0 * sigPQ + c2);
            double denominator = (mP * mP + mQ * mQ + c1) * (sigsqP + sigsqQ + c2);

            double ssim = numerator / denominator;

            if (ssim > ssimMax) {
                ssimMax = ssim;
            }

            y += WIN_SIZE;
        }
        x += WIN_SIZE;
    }

    return ssimMax;
};
