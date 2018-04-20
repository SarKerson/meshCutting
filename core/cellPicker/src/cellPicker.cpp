#include "../include/cellPicker.h"

 
MouseInteractorStyle::MouseInteractorStyle()
{
  selectedMapper = vtkSmartPointer<vtkDataSetMapper>::New();
  selectedActor = vtkSmartPointer<vtkActor>::New();
}

void MouseInteractorStyle::OnLeftButtonDown()
{
  // Get the location of the click (in window coordinates)
  int* pos = this->GetInteractor()->GetEventPosition();

  vtkSmartPointer<vtkCellPicker> picker =
    vtkSmartPointer<vtkCellPicker>::New();
  picker->SetTolerance(0.0005);

  // Pick from this location.
  picker->Pick(pos[0], pos[1], 0, this->GetDefaultRenderer());

  double* worldPosition = picker->GetPickPosition();
  std::cout << "Cell id is: " << picker->GetCellId() << std::endl;

  if(picker->GetCellId() != -1)
    {

    std::cout << "Pick position is: " << worldPosition[0] << " " << worldPosition[1]
              << " " << worldPosition[2] << endl;

    vtkSmartPointer<vtkIdTypeArray> ids =
      vtkSmartPointer<vtkIdTypeArray>::New();
    ids->SetNumberOfComponents(1);
    ids->InsertNextValue(picker->GetCellId());

    vtkSmartPointer<vtkSelectionNode> selectionNode =
      vtkSmartPointer<vtkSelectionNode>::New();
    selectionNode->SetFieldType(vtkSelectionNode::CELL);
    selectionNode->SetContentType(vtkSelectionNode::INDICES);
    selectionNode->SetSelectionList(ids);

    vtkSmartPointer<vtkSelection> selection =
      vtkSmartPointer<vtkSelection>::New();
    selection->AddNode(selectionNode);

    vtkSmartPointer<vtkExtractSelection> extractSelection =
      vtkSmartPointer<vtkExtractSelection>::New();
    extractSelection->SetInputData(0, this->Data);
    extractSelection->SetInputData(1, selection);
    extractSelection->Update();

    // In selection
    vtkSmartPointer<vtkUnstructuredGrid> selected =
      vtkSmartPointer<vtkUnstructuredGrid>::New();
    selected->ShallowCopy(extractSelection->GetOutput());

    std::cout << "There are " << selected->GetNumberOfPoints()
              << " points in the selection." << std::endl;
    std::cout << "There are " << selected->GetNumberOfCells()
              << " cells in the selection." << std::endl;


#if VTK_MAJOR_VERSION <= 5
    selectedMapper->SetInputConnection(
      selected->GetProducerPort());
#else
    selectedMapper->SetInputData(selected);
#endif

    selectedActor->SetMapper(selectedMapper);
    selectedActor->GetProperty()->EdgeVisibilityOn();
    selectedActor->GetProperty()->SetColor(1, 0, 0);
    selectedActor->GetProperty()->SetOpacity(0.5);

    this->Interactor->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->AddActor(selectedActor);

    }
  // Forward events
  vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
}