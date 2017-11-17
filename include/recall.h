#include "all_include.h"
#include <iostream>
using namespace std;


/**
 * a class that deal with points putting
 */
class PointsPut :public vtkCommand
{
public:
  
  static PointsPut* New()
  {
    return new PointsPut;
  }

  void SetObject(vtkOrientedGlyphContourRepresentation* contour)
  {
    acContour = contour;
  }
  virtual void Execute(vtkObject* caller, unsigned long eventId, void* callData)
  {
    double po[3] = {0,0,0};
    int num;
    if(eventId == vtkCommand::LeftButtonReleaseEvent) {
      num = acContour->GetNumberOfNodes();
      acContour->GetNthNodeWorldPosition(num-1,po);
      addPoint(po, acContour->GetRenderer());
    }
    vtkRenderWindowInteractor *iren = static_cast<vtkRenderWindowInteractor*>(caller);
    iren->Render();
  }


private:
  vtkOrientedGlyphContourRepresentation* acContour;
};



/**
 * a class that deal with the points deleting
 */
class PointDelete : public vtkCommand
{
public:
  static PointDelete* New()
  {
    return new PointDelete;
  }
  void SetObject(vtkOrientedGlyphContourRepresentation* contour)
  {
    acContour = contour;
  }
  virtual void Execute(vtkObject* caller, unsigned long eventId, void* callData)
  {
    vtkRenderWindowInteractor *iren = static_cast<vtkRenderWindowInteractor*>(caller);
    char getkey = iren->GetKeyCode();
    if (getkey == 'k') {
      cout << "Delete the last node" << endl;
      acContour->DeleteLastNode();
      vPoints1.pop_back();
      if (vPoints1.size() == 0) {
        cout << "no points left" << endl;
        clearAllNodes(acContour, leftrenderer);
      }
    }
    if (getkey == '1') {                    //move the actor in port_1 to port_0
      leftrenderer->RemoveActor(vleftActors[vleftActors.size() - 1]);
      leftrenderer->RemoveActor(clipperActor);
      leftrenderer->RemoveActor(thePlaneActor);
      midrender->RemoveActor(midActor);
      rightrender->RemoveActor(rightActor);
      vleftActors.push_back(midActor);
      vleftPolydatas.push_back(mid_data);
      clearAllNodes(acContour, leftrenderer);
      leftrenderer->AddActor(vleftActors[vleftActors.size() - 1]);
      pointPlacer->AddProp(vleftActors[vleftActors.size() - 1]);
    }
    if (getkey == '2') {                  //move the actor in port_2 to port_0
      leftrenderer->RemoveActor(vleftActors[vleftActors.size() - 1]);
      leftrenderer->RemoveActor(clipperActor);
      leftrenderer->RemoveActor(thePlaneActor);
      midrender->RemoveActor(midActor);
      rightrender->RemoveActor(rightActor);
      vleftActors.push_back(rightActor);
      vleftPolydatas.push_back(right_data);
      clearAllNodes(acContour, leftrenderer);
      leftrenderer->AddActor(vleftActors[vleftActors.size() - 1]);
      pointPlacer->AddProp(vleftActors[vleftActors.size() - 1]);
    }
    if (getkey == '0') {                  //return the last actor in the port_0
      assert(vleftActors.size() == vleftPolydatas.size());
      leftrenderer->RemoveActor(clipperActor);
      leftrenderer->RemoveActor(thePlaneActor);
      if (vleftActors.size() != 1) {
        vtkActor* actor = vleftActors[vleftActors.size() - 1];
        leftrenderer->RemoveActor(actor);
        vleftActors.pop_back();
        actor->Delete();

        vtkPolyData* polyData = vleftPolydatas[vleftPolydatas.size() - 1];
        vleftPolydatas.pop_back();
        polyData->Delete();

        leftrenderer->AddActor(vleftActors[vleftActors.size() - 1]);
        clearAllNodes(acContour, leftrenderer);
        pointPlacer->AddProp(vleftActors[vleftActors.size() - 1]);
      }

    }
    if (getkey == '9') {
      leftrenderer->RemoveActor(thePlaneActor);
    }
    if (getkey == 'n') {

      generateClipperData();

      generateClippedByBool();  //draw in mid and right

      midrender->SetActiveCamera(leftrenderer->GetActiveCamera());
      rightrender->SetActiveCamera(leftrenderer->GetActiveCamera());
      frontPlane = NULL;
    }
    
    iren->Render();
  }

private:
  vtkOrientedGlyphContourRepresentation* acContour;
};


class ReturnPositionSet : public vtkCommand
{
public:
  double po[3];
  int num;
  static ReturnPositionSet* New()
  {
    return new ReturnPositionSet;
  }
  void SetObject(vtkOrientedGlyphContourRepresentation* contour)
  {
    acContour = contour;
  }

  virtual void Execute(vtkObject* caller, unsigned long eventId, void* callData)
  {
    vtkRenderWindowInteractor *iren = static_cast<vtkRenderWindowInteractor*>(caller);
    char getkey = iren->GetKeyCode();
    if (getkey == 'r') {
      cout << "Display positions of all nodes" << endl;
      num = acContour->GetNumberOfNodes();
      for (int i = 0; i < num; i++)
      {
          acContour->GetNthNodeWorldPosition(i, po);
          std::cout << "The world position of the " << i << " node is:" << po[0] << " " << po[1] << " " << po[2] << std::endl;
      }
    }

    if (getkey == 'c') {
        clearAllNodes(acContour, leftrenderer);
    }

    if (getkey == 'k') {

    }
    iren->Render();
  }

private:
  vtkOrientedGlyphContourRepresentation* acContour;
};


void UpdatePlaneCollection(vtkPlaneCollection* ps, double po[3])
{
  vtkSmartPointer<vtkPlane> newlyadd =
    vtkSmartPointer<vtkPlane>::New();
  newlyadd->SetOrigin(po);
  ps->AddItem(newlyadd);
  cout << "point successfully added!" << endl;
}