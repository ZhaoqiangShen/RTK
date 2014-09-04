/*=========================================================================
 *
 *  Copyright RTK Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#ifndef __rtkConjugateGradientImageFilter_txx
#define __rtkConjugateGradientImageFilter_txx

#include "rtkConjugateGradientImageFilter.h"

namespace rtk
{

template<typename OutputImageType>
ConjugateGradientImageFilter<OutputImageType>::ConjugateGradientImageFilter()
{
  this->SetNumberOfRequiredInputs(2);
  
  m_NumberOfIterations = 1;
  m_MeasureExecutionTimes = false;

  m_A = ConjugateGradientOperatorType::New();
}

template<typename OutputImageType>
void ConjugateGradientImageFilter<OutputImageType>::SetX(const OutputImageType* OutputImage)
{
  this->SetNthInput(0, const_cast<OutputImageType*>(OutputImage));
}

template<typename OutputImageType>
void ConjugateGradientImageFilter<OutputImageType>::SetB(const OutputImageType* OutputImage)
{
  this->SetNthInput(1, const_cast<OutputImageType*>(OutputImage));
}

template<typename OutputImageType>
typename ConjugateGradientImageFilter<OutputImageType>::OutputImagePointer 
ConjugateGradientImageFilter<OutputImageType>::GetX()
{
  return static_cast< OutputImageType * >
          ( this->itk::ProcessObject::GetInput(0) );
}

template<typename OutputImageType>
typename ConjugateGradientImageFilter<OutputImageType>::OutputImagePointer 
ConjugateGradientImageFilter<OutputImageType>::GetB()
{
  return static_cast< OutputImageType * >
          ( this->itk::ProcessObject::GetInput(1) );
}

template<typename OutputImageType>
void ConjugateGradientImageFilter<OutputImageType>
::SetA(ConjugateGradientOperatorPointerType _arg )
{
  this->m_A = _arg;
  this->Modified();
}

template<typename OutputImageType>
void ConjugateGradientImageFilter<OutputImageType>
::GenerateInputRequestedRegion()
{
  // Input 0 is the X of AX=B
  typename Superclass::InputImagePointer inputPtr0 =
    const_cast< OutputImageType * >( this->GetInput(0) );
  if ( !inputPtr0 ) return;
  inputPtr0->SetRequestedRegion( this->GetOutput()->GetRequestedRegion() );

  // Input 1 is the B of AX=B
  typename Superclass::InputImagePointer inputPtr1 =
    const_cast< OutputImageType * >( this->GetInput(1) );
  if ( !inputPtr1 ) return;
  inputPtr1->SetRequestedRegion( this->GetOutput()->GetRequestedRegion() );
}

template<typename OutputImageType>
void ConjugateGradientImageFilter<OutputImageType>
::GenerateData()
{
  itk::TimeProbe CGTimeProbe;

  if(m_MeasureExecutionTimes)
    {
    std::cout << "Starting conjugate gradient initialization"<< std::endl;
    CGTimeProbe.Start();
    }

  // Initialization
  m_A->SetX(this->GetX());
  m_A->ReleaseDataFlagOn();

  typename SubtractFilterType::Pointer SubtractFilter = SubtractFilterType::New();
  SubtractFilter->SetInput(0, this->GetB());
  SubtractFilter->SetInput(1, m_A->GetOutput());
  SubtractFilter->Update();

  typename GetP_kPlusOne_FilterType::Pointer GetP_kPlusOne_Filter = GetP_kPlusOne_FilterType::New();
  typename GetR_kPlusOne_FilterType::Pointer GetR_kPlusOne_Filter = GetR_kPlusOne_FilterType::New();
  typename GetX_kPlusOne_FilterType::Pointer GetX_kPlusOne_Filter = GetX_kPlusOne_FilterType::New();

  // Compute P_zero = R_zero
  typename OutputImageType::Pointer P_zero = SubtractFilter->GetOutput();
  P_zero->DisconnectPipeline();

  // Compute AP_zero
  m_A->SetX(P_zero);

  GetR_kPlusOne_Filter->SetRk(P_zero);
  GetR_kPlusOne_Filter->SetPk(P_zero);
  GetR_kPlusOne_Filter->SetAPk(m_A->GetOutput());

  GetP_kPlusOne_Filter->SetR_kPlusOne(GetR_kPlusOne_Filter->GetOutput());
  GetP_kPlusOne_Filter->SetRk(P_zero);
  GetP_kPlusOne_Filter->SetPk(P_zero);

  GetX_kPlusOne_Filter->SetXk(this->GetX());
  GetX_kPlusOne_Filter->SetPk(P_zero);

  // Define the smart pointers that will be used with DisconnectPipeline()
  typename OutputImageType::Pointer R_kPlusOne;
  typename OutputImageType::Pointer P_kPlusOne;
  typename OutputImageType::Pointer X_kPlusOne;

  // Start the iterative procedure
  for (int iter=0; iter<m_NumberOfIterations; iter++)
    {

    if(iter>0)
      {
      R_kPlusOne = GetR_kPlusOne_Filter->GetOutput();
      R_kPlusOne->DisconnectPipeline();

      P_kPlusOne = GetP_kPlusOne_Filter->GetOutput();
      P_kPlusOne->DisconnectPipeline();

      X_kPlusOne = GetX_kPlusOne_Filter->GetOutput();
      X_kPlusOne->DisconnectPipeline();

      m_A->SetX(P_kPlusOne);

      GetR_kPlusOne_Filter->SetRk(R_kPlusOne);
      GetR_kPlusOne_Filter->SetPk(P_kPlusOne);
      GetR_kPlusOne_Filter->SetAPk(m_A->GetOutput());

      GetP_kPlusOne_Filter->SetRk(R_kPlusOne);
      GetP_kPlusOne_Filter->SetPk(P_kPlusOne);
      GetP_kPlusOne_Filter->SetR_kPlusOne(GetR_kPlusOne_Filter->GetOutput());

      GetX_kPlusOne_Filter->SetPk(P_kPlusOne);
      GetX_kPlusOne_Filter->SetXk(X_kPlusOne);

      P_zero->ReleaseData();
      }

    m_A->Update();
    GetR_kPlusOne_Filter->Update();
    GetX_kPlusOne_Filter->SetAlphak(GetR_kPlusOne_Filter->GetAlphak());
    GetX_kPlusOne_Filter->Update();
    GetP_kPlusOne_Filter->SetSquaredNormR_k(GetR_kPlusOne_Filter->GetSquaredNormR_k());
    GetP_kPlusOne_Filter->SetSquaredNormR_kPlusOne(GetR_kPlusOne_Filter->GetSquaredNormR_kPlusOne());
    GetP_kPlusOne_Filter->Update();
    }

  this->GraftOutput(GetX_kPlusOne_Filter->GetOutput());

  // Release the data from internal filters
  R_kPlusOne->ReleaseData();
  P_kPlusOne->ReleaseData();
  X_kPlusOne->ReleaseData();
  GetR_kPlusOne_Filter->GetOutput()->ReleaseData();
  GetP_kPlusOne_Filter->GetOutput()->ReleaseData();
  m_A->GetOutput()->ReleaseData();
  SubtractFilter->GetOutput()->ReleaseData();
}

}// end namespace


#endif
