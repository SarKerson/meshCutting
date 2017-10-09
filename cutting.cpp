#include <vtkVersion.h>
#include <vtkSmartPointer.h>
#include <vtkSTLReader.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkDataSetMapper.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyData.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkClipClosedSurface.h>
#include <vtkClipPolyData.h>
#include <vtkPlane.h>
#include <vtkPlaneCollection.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkBooleanOperationPolyDataFilter.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkInteractorStyleTrackballActor.h>
#include <vtkNamedColors.h>
#include <vtkSphereSource.h>
#include <vtkOrientedGlyphContourRepresentation.h>
#include <vtkContourWidget.h>
#include <vtkPolygonalSurfacePointPlacer.h>
#include <vtkLinearContourLineInterpolator.h>
#include <vtkCellPicker.h>
#include <vtkCallbackCommand.h>
#include <vtkAreaPicker.h>
#include <vtkCommand.h>
#include <vtkCoordinate.h>
#include <vtkUnstructuredGrid.h>
#include <vtkPentagonalPrism.h>
#include <vtkPlaneSource.h>
#include <vtkImplicitPolyDataDistance.h>
#include <vtkFloatArray.h>
#include <vtkTableBasedClipDataSet.h>
#include <vtkRectilinearGridGeometryFilter.h>
#include <vtkPointData.h>
#include <vtkConeSource.h>
#include <vtkPolygon.h>
#include <vtkDelaunay2D.h>
#include <opencv2/opencv.hpp>
#include <vtkBox.h>
#include <vtkElevationFilter.h>
#include <vtkImplicitBoolean.h>
#include <vtkElevationFilter.h>
#include <vtkDelaunay3D.h>
#include <vtkImplicitDataSet.h>
#include <vtkTriangleFilter.h>
#include <vtkLinearExtrusionFilter.h>
#include <vtkMath.h>



vtkSmartPointer<vtkPolyData> clipSource;
vtkSmartPointer<vtkPolyData> clipperData;
std::vector<cv::Vec3f> vPoints1;


vtkSmartPointer<vtkTriangleFilter> trifilter;

vtkSmartPointer<vtkRenderer> leftrenderer;
vtkSmartPointer<vtkRenderer> midrender;
vtkSmartPointer<vtkRenderer> rightrender;
vtkSmartPointer<vtkPlane> frontPlane = NULL;

double *center;
double *bounds;
double g_normal[3];
double g_origin[3];
double g_distance = 0;



double maximumLength(double* _bounds)
{
  std::vector<double> vbounds;
  vbounds.push_back(_bounds[1] - _bounds[0]);
  vbounds.push_back(_bounds[3] - _bounds[2]);
  vbounds.push_back(_bounds[5] - _bounds[4]);
  std::vector<double>::iterator max = std::max_element(vbounds.begin(), vbounds.end());
  return *max;
}



/*
draw points on the two pararel planes
*/
void drawPoint(double* p, vtkRenderer* r) 
{
  // Create a sphere
    vtkSmartPointer<vtkSphereSource> sphereSource =
      vtkSmartPointer<vtkSphereSource>::New();
    sphereSource->SetCenter(p[0], p[1], p[2]);
    sphereSource->SetRadius(0.3);
    sphereSource->Update();
    

    vtkSmartPointer<vtkDataSetMapper> m = vtkSmartPointer<vtkDataSetMapper>::New();
    m->SetInputConnection(sphereSource->GetOutputPort());
    vtkSmartPointer<vtkActor> a = vtkSmartPointer<vtkActor>::New();
    a->SetMapper(m);
    r->AddActor(a);
}



void addPoint(double* p, vtkRenderer* render)
{
  if(frontPlane == NULL) {
    cout << "plane" << endl;
    frontPlane = vtkSmartPointer<vtkPlane>::New();
    double a_view[3], b_view[3], c_view[3];
    a_view[0] = 0.0; a_view[1] = 0.0; a_view[2] = 0.0;
    b_view[0] = 0.0; b_view[1] = 1.0; b_view[2] = 0.0;
    c_view[0] = 1.0; c_view[1] = 0.0; c_view[2] = 0.0;
    
    render->ViewToWorld(a_view[0], a_view[1], a_view[2]);
    render->ViewToWorld(b_view[0], b_view[1], b_view[2]);
    render->ViewToWorld(c_view[0], c_view[1], c_view[2]);
    double e1[3] = { c_view[0] - a_view[0],
                     c_view[1] - a_view[1],
                     c_view[2] - a_view[2] };
    double e2[3] = { b_view[0] - a_view[0],
                     b_view[1] - a_view[1],
                     b_view[2] - a_view[2] };

    double normal[3];
    vtkMath::Cross(e2, e1, normal);
    vtkMath::Normalize(normal);


    g_normal[0] = normal[0]; g_normal[1] = normal[1]; g_normal[2] = normal[2];
    g_origin[0] = a_view[0]; g_origin[1] = a_view[1]; g_origin[2] = a_view[2];

    frontPlane->SetOrigin(a_view);
    frontPlane->SetNormal(normal);
  }

  //front face

  double point[3];
  frontPlane->ProjectPoint(p, g_origin, g_normal, point);
  double distance = vtkMath::Distance2BetweenPoints(p, point);
  if (distance > g_distance) {
    g_distance = distance;
  }
  cv::Vec3f vpoint(point[0], point[1], point[2]);
  vPoints1.push_back(vpoint);

  // drawPoint(point, render);
}


void generateClipperData()
{
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  for(int i = 0; i < vPoints1.size(); ++i) {
    points->InsertNextPoint(vPoints1[i][0], vPoints1[i][1], vPoints1[i][2]);
  }
  vtkSmartPointer<vtkPolygon> polygon4 = vtkSmartPointer<vtkPolygon>::New();
  polygon4->GetPointIds()->SetNumberOfIds(vPoints1.size());
  cout << vPoints1.size();
  for(int i = 0; i < vPoints1.size(); ++i) {
    polygon4->GetPointIds()->SetId(i, i);
  }

  vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
  cells->InsertNextCell(polygon4);
  vtkSmartPointer<vtkPolyData> triangle = vtkSmartPointer<vtkPolyData>::New();
  triangle->SetPoints(points);
  triangle->SetPolys(cells);

  vtkSmartPointer<vtkLinearExtrusionFilter> extrude = vtkSmartPointer<vtkLinearExtrusionFilter>::New();
  extrude->SetInputData(triangle);
  extrude->SetExtrusionTypeToNormalExtrusion();
  extrude->SetVector(g_normal[0], g_normal[1], g_normal[2]);
  extrude->SetScaleFactor(1.5 * maximumLength(bounds) + g_distance);

  trifilter = vtkSmartPointer<vtkTriangleFilter>::New();
  trifilter->SetInputConnection(extrude->GetOutputPort());


  
   vtkSmartPointer<vtkDataSetMapper> mapper = 
    vtkSmartPointer<vtkDataSetMapper>::New();
  mapper->SetInputConnection(trifilter->GetOutputPort());
 
  vtkSmartPointer<vtkActor> actor = 
    vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(0.8900, 0.8100, 0.3400);

  leftrenderer->AddActor(actor);

} 
  

double mod()
{
  double p[3] = { bounds[1] - bounds[0], 
                  bounds[3] - bounds[2],
                  bounds[5] - bounds[4] };
  return p[0] * p[0] + p[1] * p[1] + p[2] * p[2];
}


void generateClippedByBool()
{


  /*
    fist, get the intersection, show on the mid renderer
  */
  vtkSmartPointer<vtkBooleanOperationPolyDataFilter> booleanOperation =  
    vtkSmartPointer<vtkBooleanOperationPolyDataFilter>::New(); 
  booleanOperation->SetOperationToIntersection();
  booleanOperation->SetInputData( 1, clipSource );
  booleanOperation->SetInputData(0, trifilter->GetOutput()); // set the input data  
  vtkSmartPointer<vtkPolyDataMapper> booleanOperationMapper =  
  vtkSmartPointer<vtkPolyDataMapper>::New();  
  booleanOperationMapper->SetInputConnection( booleanOperation->GetOutputPort() );  
  booleanOperationMapper->ScalarVisibilityOff();  
  vtkSmartPointer<vtkActor> booleanOperationActor = vtkSmartPointer<vtkActor>::New();  
  booleanOperationActor->SetMapper( booleanOperationMapper );
  midrender->AddActor(booleanOperationActor);


  vtkSmartPointer<vtkBooleanOperationPolyDataFilter> booleanOperation1 =  
    vtkSmartPointer<vtkBooleanOperationPolyDataFilter>::New(); 
  booleanOperation1->SetOperationToDifference();
  booleanOperation1->SetInputData( 0, clipSource );
  booleanOperation1->SetInputData(1, trifilter->GetOutput());

  vtkSmartPointer<vtkPolyDataMapper> booleanOperationMapper1 =  
  vtkSmartPointer<vtkPolyDataMapper>::New();  
  booleanOperationMapper1->SetInputConnection( booleanOperation1->GetOutputPort() );  
  booleanOperationMapper1->ScalarVisibilityOff();  
  vtkSmartPointer<vtkActor> booleanOperationActor1 = vtkSmartPointer<vtkActor>::New();  
  booleanOperationActor1->SetMapper( booleanOperationMapper1 );
  rightrender->AddActor(booleanOperationActor1);

}


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
    else if(eventId == vtkCommand::RightButtonReleaseEvent) {
      num = acContour->GetNumberOfNodes();
      acContour->GetNthNodeWorldPosition(num-1,po);
      addPoint(po, acContour->GetRenderer());

      generateClipperData();

      generateClippedByBool();  //draw in mid and right
      midrender->SetActiveCamera(leftrenderer->GetActiveCamera());
      rightrender->SetActiveCamera(leftrenderer->GetActiveCamera());

      vtkRenderWindowInteractor *iren = static_cast<vtkRenderWindowInteractor*>(caller);
      iren->Render();
    }
  }

  // void setCutter(vtkSmartPointer<PlanesCutter> cutter) {
  //   this->cutter = cutter;
  // }

private:
  vtkOrientedGlyphContourRepresentation* acContour;
  // vtkSmartPointer<PlanesCutter> cutter;
};



/*
vtkSmartPointer<myCallback> callback = vtkSmartPointer<myCallback>::New();
callback->SetObject(contourRep);
iren->AddObserver(vtkCommand::LeftButtonReleaseEvent, callback);
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
      iren->Render();
    }
  }
  // void setCutter(vtkSmartPointer<PlanesCutter> cutter) {
  //   this->cutter = cutter;
  // }

private:
  vtkOrientedGlyphContourRepresentation* acContour;
  // vtkSmartPointer<PlanesCutter> cutter;
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
  // void GetPlane(vtkPlaneCollection* planes)
  // {
  //   PlaneSet = planes;
  // }
  virtual void Execute(vtkObject* caller, unsigned long eventId, void* callData)
  {
    vtkRenderWindowInteractor *iren = static_cast<vtkRenderWindowInteractor*>(caller);
    char getkey = iren->GetKeyCode();
    if (getkey == 'r') {
      cout << "Display positions of all nodes" << endl;
      num = acContour->GetNumberOfNodes();
      // PlaneSet->RemoveAllItems();
      for (int i = 0; i < num; i++)
      {
          acContour->GetNthNodeWorldPosition(i, po);
          std::cout << "The world position of the " << i << " node is:" << po[0] << " " << po[1] << " " << po[2] << std::endl;
        // UpdatePlaneCollection(PlaneSet, po);
      }
    }

    if (getkey == 'c') {
        cout << "Delete all node" << endl;
        acContour->ClearAllNodes();
        vPoints1.clear();
        frontPlane = NULL;
        iren->Render();
    }
  }
  // void setCutter(vtkSmartPointer<PlanesCutter> cutter) {
  //   this->cutter = cutter;
  // }
private:
  vtkOrientedGlyphContourRepresentation* acContour;
  // vtkSmartPointer<PlanesCutter> cutter;
};


void UpdatePlaneCollection(vtkPlaneCollection* ps, double po[3])
{
  vtkSmartPointer<vtkPlane> newlyadd =
    vtkSmartPointer<vtkPlane>::New();
  newlyadd->SetOrigin(po);
  ps->AddItem(newlyadd);
  cout << "point successfully added!" << endl;
}


/*
My extract function
*/
vtkPolyData* extractExtra(vtkPolyData *complete, vtkPolyData *part)
{

  vtkSmartPointer<vtkPolyData> data = vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkBooleanOperationPolyDataFilter> booleanOperation =
    vtkSmartPointer<vtkBooleanOperationPolyDataFilter>::New();
     booleanOperation->SetOperationToDifference();
  booleanOperation->SetInputData( 0, complete );
    booleanOperation->SetInputData( 1, part );  
    data = booleanOperation->GetOutput();
    return data;
}


/*
get the polydata
*/
vtkSmartPointer<vtkPolyData> getPolyData() 
{
  // // Create a sphere
  //   vtkSmartPointer<vtkSphereSource> sphereSource =
  //     vtkSmartPointer<vtkSphereSource>::New();
  //   sphereSource->SetCenter(0.0, 0.0, 0.0);
  //   sphereSource->SetRadius(1.0);
  //   sphereSource->SetThetaResolution(100);
  //   sphereSource->SetPhiResolution(100);
  //   sphereSource->Update();
  //   return sphereSource->GetOutput();
  



  vtkSmartPointer<vtkSTLReader> reader = vtkSmartPointer<vtkSTLReader>::New();
  reader->SetFileName("/home/sar/Desktop/My_test/vtk_code/data/Implant.stl");
  reader->Update();

  vtkSmartPointer<vtkTransform> pTransform = vtkSmartPointer<vtkTransform>::New();
    pTransform->Scale(0.1, 0.1, 0.1);
    vtkSmartPointer<vtkTransformPolyDataFilter> pTransformPolyDataFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    pTransformPolyDataFilter->SetInputData(reader->GetOutput());
    pTransformPolyDataFilter->SetTransform(pTransform);
    pTransformPolyDataFilter->Update();
  return pTransformPolyDataFilter->GetOutput();
}



/*
get the normal of the line
*/
void getNormal(const cv::Vec3f v1, const cv::Vec3f v2, double *p3)
{
  cv::Vec3f vn = cv::normalize(v1.cross(v2));
  for(size_t i = 0; i < 3; ++i) {
    p3[i] = vn[i];
  }


}

void updateNormal(double* n1, double* n2)
{
  cv::Vec3f v1(n1[0], n1[1], n1[2]),
            v2(n2[0], n2[1], n2[2]);

}


vtkSmartPointer<vtkPlane> getPlane(double* o, double* n)
{
  vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
  plane->SetOrigin(o);
  plane->SetNormal(n);
  return plane;
}



/*
get the collection of planes
*/
vtkSmartPointer<vtkPlaneCollection> getPlanesCollection(double* center, 
                                                        double* bounds, 
                                                        vtkContourRepresentation* contourRep = NULL) 
{
  //cout << sizeof(center) << "#" << sizeof(bounds) << "#" << (contourRep == NULL ? "null" : "notNUll") << endl;
  if(contourRep) {
    int num;
    double po[3];
    num=contourRep->GetNumberOfNodes();
    double origin[num][3];
    double normal[num][3];
    contourRep->GetNthNodeWorldPosition(0, origin[0]);
    double p[2][3];
    for (int i = 1; i < num; i++) {
      contourRep->GetNthNodeWorldPosition(i, origin[i]);
      cv::Vec3f v0(origin[i - 1][0], origin[i - 1][1], 1/*origin[i - 1][2]*/),
                v1(origin[i][0], origin[i][1], 1/*origin[i][2]*/),
                v0_(origin[i - 1][0], origin[i - 1][1], 0);
      cv::Vec3f e1 = v1 - v0,
                e2 = v0_ - v0;
      getNormal(e1, e2, normal[i - 1]);
    }
    contourRep->GetNthNodeWorldPosition(0, p[0]);
    contourRep->GetNthNodeWorldPosition(num - 1, p[1]);

    cv::Vec3f v0(origin[0][0], origin[0][1], 1/*origin[i - 1][2]*/),
              v1(origin[num - 1][0], origin[num - 1][1], 1/*origin[i][2]*/),
              v0_(origin[num - 1][0], origin[num - 1][1], 0);
    cv::Vec3f e1 = v1 - v0,
              e2 = v0_ - v0;
    getNormal(e1, e2, normal[num - 1]);

    vtkSmartPointer<vtkPlaneCollection> planes =
      vtkSmartPointer<vtkPlaneCollection>::New();
    for(size_t i = 0; i < num - 1; ++i) {
      origin[i][2] = 1;
      planes->AddItem(getPlane(origin[i], normal[i]));
    }
    origin[num - 1][2] = 1;
    normal[num - 1][0] = - normal[num - 1][0];
    normal[num - 1][1] = - normal[num - 1][1];
    planes->AddItem(getPlane(origin[num - 1], normal[num - 1]));
    return planes;
  }
  else {
    cout << "no points" << endl;
    return NULL;
  }
}
 




//
// Demonstrate the use of clipping of polygonal data
//

int main (int argc, char *argv[])
{
  // PolyData to process
  
  
  clipSource = getPolyData();

  center = clipSource->GetCenter();
  bounds = clipSource->GetBounds();




  //create mapper and actor for each polyData
  vtkSmartPointer<vtkDataSetMapper> sourceMapper =
    vtkSmartPointer<vtkDataSetMapper>::New();
  sourceMapper->SetInputData(clipSource);
  // clipMapper1->SetInputConnection(clipper->GetOutputPort());

  vtkSmartPointer<vtkActor> sourceActor =
    vtkSmartPointer<vtkActor>::New();
  sourceActor->SetMapper(sourceMapper);
  sourceActor->GetProperty()->SetColor(1.0000,0.3882,0.2784);
  sourceActor->SetPosition(0, 0, 0);
  sourceActor->SetOrigin(0, 0, 0);
  sourceActor->GetProperty()->SetInterpolationToFlat();




  // A renderer and render window
  leftrenderer = vtkSmartPointer<vtkRenderer>::New();
  leftrenderer->SetViewport(0.0, 0.0, 0.4, 1.0);
  leftrenderer->SetBackground(0, 0, 1);
  midrender = vtkSmartPointer<vtkRenderer>::New();
  midrender->SetViewport(0.4, 0.0, 0.7, 1.0);
  midrender->SetBackground(0, 1, 0);
  rightrender = vtkSmartPointer<vtkRenderer>::New();
  rightrender->SetViewport(0.7, 0, 1.0, 1.0);
  rightrender->SetBackground(1, 0, 0);



  vtkSmartPointer<vtkRenderWindow> renderWindow = 
    vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->AddRenderer(leftrenderer);
  renderWindow->AddRenderer(midrender);
  renderWindow->AddRenderer(rightrender);
  leftrenderer->AddActor(sourceActor);
  leftrenderer->ResetCamera();
  midrender->ResetCamera();
  rightrender->ResetCamera();

  renderWindow->SetSize(720, 240);

  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor = 
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);
//----------------start call back----------------------------------

  vtkOrientedGlyphContourRepresentation *contourRep =
    vtkOrientedGlyphContourRepresentation::New();

  vtkContourWidget *contourWidget = vtkContourWidget::New();
  contourWidget->SetInteractor(renderWindowInteractor);
  contourWidget->SetRepresentation(contourRep);
  vtkPolygonalSurfacePointPlacer * pointPlacer
    = vtkPolygonalSurfacePointPlacer::New();
  pointPlacer->AddProp(sourceActor);
  contourRep->SetPointPlacer(pointPlacer);
  vtkLinearContourLineInterpolator * interpolator =
    vtkLinearContourLineInterpolator::New();
  contourRep->SetLineInterpolator(interpolator);
  contourWidget->SetEnabled(1);

  vtkSmartPointer<PointDelete> poidel =
    vtkSmartPointer<PointDelete>::New();
  poidel->SetObject(contourRep);

  vtkSmartPointer<PointsPut> poput =
    vtkSmartPointer<PointsPut>::New();
  poput->SetObject(contourRep);

  vtkSmartPointer<ReturnPositionSet> poiset =
    vtkSmartPointer<ReturnPositionSet>::New();
  poiset->SetObject(contourRep);


  renderWindowInteractor->AddObserver(vtkCommand::KeyReleaseEvent, poidel);
  renderWindowInteractor->AddObserver(vtkCommand::KeyReleaseEvent, poiset);
  renderWindowInteractor->AddObserver(vtkCommand::LeftButtonReleaseEvent, poput);
  renderWindowInteractor->AddObserver(vtkCommand::RightButtonReleaseEvent, poput);
//----------------end call back-------------------------------------------------




  renderWindow->Render();
  renderWindowInteractor->Initialize();
  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}