#pragma once
#include <vtkCellPicker.h>
#include <vtkIdTypeArray.h>
#include <vtkProperty.h>
#include <vtkSelectionNode.h>
#include <vtkSelection.h>
#include <vtkExtractSelection.h>
#include <vtkObjectFactory.h>
#include <vtkRendererCollection.h>
#include <vtkDataSetSurfaceFilter.h>
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkDataSetMapper.h"
#include <vtkUnstructuredGrid.h>
#include <vtkRenderWindow.h>
#include "vtkRenderWindowInteractor.h"

// Catch mouse events
class MouseInteractorStyle : public vtkInteractorStyleTrackballCamera
{
  public:
  static MouseInteractorStyle* New();
 
  MouseInteractorStyle();
 
    virtual void OnLeftButtonDown();
 
    vtkSmartPointer<vtkUnstructuredGrid> Data;
    vtkSmartPointer<vtkDataSetMapper> selectedMapper;
    vtkSmartPointer<vtkActor> selectedActor;
 
};