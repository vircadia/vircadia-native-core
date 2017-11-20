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
#include <itkTranslationTransform.h>
#include <itkRegularStepGradientDescentOptimizerv4.h>
#include <itkMeanSquaresImageToImageMetricv4.h>
#include <itkImageRegistrationMethodv4.h>

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
    
    // Setup registration components
    // The transform that will map the fixed image space into the moving image space
    using TransformType =  itk::TranslationTransform<double, Dimension>;

    // The optimizer explores the parameter space of the transform in search of optimal values of the metric
    using OptimizerType =  itk::RegularStepGradientDescentOptimizerv4<double>;

    // The metric will compare how well the two images match each other
    using MetricType =  itk::MeanSquaresImageToImageMetricv4<MonochromeImageType, MonochromeImageType>;

    //The registration method type is instantiated using the types of the fixed and moving images as well
    // as the output transform type.This class is responsible for interconnecting the components 
    using RegistrationType = itk::ImageRegistrationMethodv4<MonochromeImageType, MonochromeImageType, TransformType>;

    // Create registration components (smart pointers, so no need to delete)
    MetricType::Pointer metric = MetricType::New();
    OptimizerType::Pointer optimizer = OptimizerType::New();
    RegistrationType::Pointer registration = RegistrationType::New();

    // The comparison metric needs an interpreter.
    // The same interpolator is used for both images
    using LinearInterpolatorType = itk::LinearInterpolateImageFunction<MonochromeImageType, double>;
    LinearInterpolatorType::Pointer interpolator = LinearInterpolatorType::New();
    metric->SetFixedInterpolator(interpolator);
    metric->SetMovingInterpolator(interpolator);

    // Connect components
    registration->SetMetric(metric);
    registration->SetOptimizer(optimizer);
    registration->SetFixedImage(resultImageToMonochrome->GetOutput());
    registration->SetMovingImage(expectedImageToMonochrome->GetOutput());

    // Initialization
    TransformType::Pointer movingInitialTransform = TransformType::New();
    TransformType::ParametersType initialParameters(movingInitialTransform->GetNumberOfParameters());

    initialParameters[0] = 0.0; // Initial offset in mm along X
    initialParameters[1] = 0.0; // Initial offset in mm along Y

    movingInitialTransform->SetParameters(initialParameters);
    registration->SetMovingInitialTransform(movingInitialTransform);

    // Set optimizer parameters
    optimizer->SetLearningRate(4);
    optimizer->SetMinimumStepLength(0.001);
    optimizer->SetRelaxationFactor(0.5);
    optimizer->SetNumberOfIterations(200);

    // Define registration criteria
    const unsigned int numberOfLevels = 1;
    RegistrationType::ShrinkFactorsArrayType shrinkFactorsPerLevel;
    shrinkFactorsPerLevel.SetSize(1);
    shrinkFactorsPerLevel[0] = 1;
    RegistrationType::SmoothingSigmasArrayType smoothingSigmasPerLevel;
    smoothingSigmasPerLevel.SetSize(1);
    smoothingSigmasPerLevel[0] = 0;
    registration->SetNumberOfLevels(numberOfLevels);
    registration->SetSmoothingSigmasPerLevel(smoothingSigmasPerLevel);
    registration->SetShrinkFactorsPerLevel(shrinkFactorsPerLevel);

    // Activate the registration project
    try
    {
        registration->Update();
        return 0.0;
    } catch (itk::ExceptionObject & err) {
        return EXIT_FAILURE;
    }
}
