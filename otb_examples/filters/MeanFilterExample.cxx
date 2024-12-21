//  The composite filter we will build combines three filters: a gradient
//  magnitude operator, which will calculate the first-order derivative of
//  the image; a thresholding step to select edges over a given strength;
//  and finally a rescaling filter, to ensure the resulting image data is
//  visible by scaling the intensity to the full spectrum of the output
//  image type.
//
//  Since this filter takes an image and produces another image (of
//  identical type), we will specialize the ImageToImageFilter:

//  Next we include headers for the component filters:

#include "itkGradientMagnitudeImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkThresholdImageFilter.h"
#include "itkUnaryFunctorImageFilter.h"

#include "itkNumericTraits.h"
#include "otbImage.h"

//  Now we can declare the filter itself.  It is within the OTB namespace,
//  and we decide to make it use the same image type for both input and
//  output, thus the template declaration needs only one parameter.
//  Deriving from \code{ImageToImageFilter} provides default behavior for
//  several important aspects, notably allocating the output image (and
//  making it the same dimensions as the input).

namespace otb {

template <class TImageType>
class ITK_EXPORT MeanFilterExample
    : public itk::ImageToImageFilter<TImageType, TImageType> {
public:
  //  Next we have the standard declarations, used for object creation with
  //  the object factory:

  using Self = MeanFilterExample<TImageType>;
  using Superclass = itk::ImageToImageFilter<TImageType, TImageType>;
  using Pointer = itk::SmartPointer<Self>;
  using ConstPointer = itk::SmartPointer<const Self>;

  /** Method for creation through object factory */
  itkNewMacro(Self);

  /** Run-time type information */
  itkTypeMacro(MeanFilterExample, itk::ImageToImageFilter);

  /** Display */
  void PrintSelf(std::ostream &os, itk::Indent indent) const override;

  using PixelType = typename TImageType::PixelType;

  itkGetMacro(Radius, unsigned int);
  itkSetMacro(Radius, unsigned int);

protected:
  MeanFilterExample();
  ~MeanFilterExample() override = default;

  //  Now we can declare the component filter types, templated over the
  //  enclosing image type:

protected:
  void GenerateData() override;

private:
  // As stated in documentation, we need to ensure that the filter is created
  // only by means of the factory methods
  MeanFilterExample(const Self &) = delete; // prevent copying operator
  void operator=(const Self &) = delete;    // prevent assignment operator

  // We declare the radius of the kernel (neighborhooditerators radius)
  unsigned int m_Radius;
};

} /* namespace otb */

//  The constructor sets up the pipeline, which involves creating the
//  stages, connecting them together, and setting default parameters.

namespace otb {

template <class TImageType> MeanFilterExample<TImageType>::MeanFilterExample() {
  m_Radius = 1;
}

//  The \code{GenerateData()} is where the composite magic happens.  First,
//  we connect the first component filter to the inputs of the composite
//  filter (the actual input, supplied by the upstream stage).  Then we
//  graft the output of the last stage onto the output of the composite,
//  which ensures the filter regions are updated.  We force the composite
//  pipeline to be processed by calling \code{Update()} on the final stage,
//  then graft the output back onto the output of the enclosing filter, so
//  it has the result available to the downstream filter.

template <class TImageType> void MeanFilterExample<TImageType>::GenerateData() {
  // Get input/output filter
  typename TImageType::ConstPointer inputImage = this->GetInput();
  typename TImageType::Pointer outputImage = this->GetOutput();

  outputImage->SetRegions(inputImage->GetLargestPossibleRegion());
  outputImage->Allocate();

  // Declare input/output iterators types
  using NeighborhoodIteratorType = itk::ConstNeighborhoodIterator<TImageType>;
  using OutputIteratorType = itk::ImageRegionIterator<TImageType>;

  // Declare and use radius type (assign private member m_Radius)
  typename NeighborhoodIteratorType::RadiusType radius;
  radius.Fill(m_Radius);

  // Declare input/output iterators referencing input/output channels
  NeighborhoodIteratorType inputIterator(radius, inputImage,
                                         inputImage->GetRequestedRegion());
  OutputIteratorType outputIterator(outputImage,
                                    outputImage->GetRequestedRegion());

  // Main iterator code
  for (inputIterator.GoToBegin(), outputIterator.GoToBegin();
       !inputIterator.IsAtEnd(); ++inputIterator, ++outputIterator) {
    float sum = 0.0;
    unsigned int count = 0;

    for (unsigned int i = 0; i < inputIterator.Size(); ++i) {
      sum += inputIterator.GetPixel(i);
      ++count;
    }

    PixelType meanValue =
        static_cast<PixelType>((count > 0) ? (sum / count) : 0.0);
    outputIterator.Set(meanValue);
  }
}

//  Finally we define the \code{PrintSelf} method, which (by convention)
//  prints the filter parameters.  Note how it invokes the superclass to
//  print itself first, and also how the indentation prefixes each line.
//
template <class TImageType>
void MeanFilterExample<TImageType>::PrintSelf(std::ostream &os,
                                              itk::Indent indent) const {
  Superclass::PrintSelf(os, indent);

  os << indent << "Radius:" << this->m_Radius << std::endl;
}

} /* end namespace otb */

//  It is important to note that in the above example, none of the internal
//  details of the pipeline were exposed to users of the class.  The interface
//  consisted of the Threshold parameter (which happened to change the value in
//  the component filter) and the regular ImageToImageFilter interface.  This
//  example pipeline is illustrated in
//  Figure~\ref{fig:CompositeExamplePipeline}.

#include "otbImageFileReader.h"
#include "otbImageFileWriter.h"

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cerr << "Usage: " << std::endl;
    std::cerr << argv[0] << "  inputImageFile  outputImageFile" << std::endl;
    return EXIT_FAILURE;
  }

  using ImageType = otb::Image<float, 2>;
  using ReaderType = otb::ImageFileReader<ImageType>;
  using WriterType = otb::ImageFileWriter<ImageType>;

  using FilterType = otb::MeanFilterExample<ImageType>;

  ReaderType::Pointer reader = ReaderType::New();
  WriterType::Pointer writer = WriterType::New();
  FilterType::Pointer filter = FilterType::New();

  reader->SetFileName(argv[1]);
  filter->SetInput(reader->GetOutput());
  filter->SetRadius(3);
  writer->SetInput(filter->GetOutput());
  writer->SetFileName(argv[2]);

  try {
    writer->Update();
  } catch (itk::ExceptionObject &e) {
    std::cerr << "Error: " << e << std::endl;
  }

  return EXIT_SUCCESS;
}
