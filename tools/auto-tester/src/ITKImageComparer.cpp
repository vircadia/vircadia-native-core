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

#include <itkRGBToLuminanceImageFilter.h>
#include <itkTestingComparisonImageFilter.h>

ITKImageComparer::ITKImageComparer() {
    // These are smart pointers that do not need to be deleted
    resultImageReader = ReaderType::New();
    expectedImageReader = ReaderType::New();
}

float ITKImageComparer::compareImages(QString resultImageFilename, QString expectedImageFilename) const {
    resultImageReader->SetFileName(resultImageFilename.toStdString().c_str());
    expectedImageReader->SetFileName(expectedImageFilename.toStdString().c_str());

    // Images are converted to monochrome for comparison
    using MonochromePixelType = double;
    using MonochromeImageType = itk::Image<MonochromePixelType, Dimension>;
    using FilterType = itk::RGBToLuminanceImageFilter<RGBImageType, MonochromeImageType>;
    
    FilterType::Pointer resultImageToMonochrome = FilterType::New();
    FilterType::Pointer expectedImageToMonochrome = FilterType::New();

    resultImageToMonochrome->SetInput(resultImageReader->GetOutput());
    expectedImageToMonochrome->SetInput(expectedImageReader->GetOutput());

    using DiffType = itk::Testing::ComparisonImageFilter<MonochromeImageType, MonochromeImageType>;
    DiffType::Pointer diff = DiffType::New();
    
    diff->SetTestInput(resultImageToMonochrome->GetOutput());
    diff->SetValidInput(expectedImageToMonochrome->GetOutput());

    const double INTENSITY_TOLERANCE = 0.01;
    diff->SetDifferenceThreshold(INTENSITY_TOLERANCE);

    const double RADIUS_TOLERANCE = 2.0;
    diff->SetToleranceRadius(RADIUS_TOLERANCE);

    diff->UpdateLargestPossibleRegion();

    return (float)diff->GetNumberOfPixelsWithDifferences();
}
