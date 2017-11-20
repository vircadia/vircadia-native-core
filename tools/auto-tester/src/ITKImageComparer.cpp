//
//  ITKImageComparer.cpp
//
//  Created by Nissim Hadar on 18 Nov 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ITKImageComparer.h"

#include <itkRGBPixel.h>
#include <itkImage.h>
#include <itkRGBToLuminanceImageFilter.h>

ITKImageComparer::ITKImageComparer() {
    // These are smart pointers that do not need to be deleted
    actualImageReader = ReaderType::New();
    expectedImageReader = ReaderType::New();
}

float ITKImageComparer::compareImages(QString actualImageFilename, QString expectedImageFilename) const {
    actualImageReader->SetFileName(actualImageFilename.toStdString().c_str());
    expectedImageReader->SetFileName(expectedImageFilename.toStdString().c_str());

    // Images are converted to monochrome for comparison
    using MonochromePixelType = unsigned char;
    using MonochromeImageType = itk::Image<MonochromePixelType, Dimension>;
    using FilterType = itk::RGBToLuminanceImageFilter<RGBImageType, MonochromeImageType>;
    
    FilterType::Pointer actualImageToMonochrome = FilterType::New();
    FilterType::Pointer expectedImageToMonochrome = FilterType::New();

    actualImageToMonochrome->SetInput(actualImageReader->GetOutput());
    expectedImageToMonochrome->SetInput(expectedImageReader->GetOutput());
    
    return 0.0;
}
