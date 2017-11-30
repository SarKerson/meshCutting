#include "SideBySideRenderWindowsQt.h"

#include <vtkDataObjectToTable.h>
#include <vtkElevationFilter.h>
#include "vtkGenericOpenGLRenderWindow.h"
#include <vtkPolyDataMapper.h>
#include <vtkQtTableView.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSphereSource.h>
#include <vtkCubeSource.h>
#include <vtkSmartPointer.h>
#include "functions.h"
#include <vtkInteractorStyleTrackballCamera.h>

#include <QToolBar>
#include <QMenuBar>
#include <QFileDialog>
#include <vtkSTLWriter.h>


#include <iostream>
#include <string.h>
using namespace std;


/**
 * scale of the plane to ensure the enough area
 */
double scale = 1;
bool CUTTING_MODE = 0;



vtkSmartPointer<vtkPolygonalSurfacePointPlacer> pointPlacer;

vtkSmartPointer<vtkPolyData> clipSource = NULL;

//std::vector<cv::Vec3f> vPoints1;
std::vector<double*> vPoints1;


vtkSmartPointer<vtkPlane> frontPlane = NULL;

/**
 * renderer in diff view port
 */
vtkSmartPointer<vtkRenderer> leftrenderer;
vtkSmartPointer<vtkRenderer> midrender;
vtkSmartPointer<vtkRenderer> rightrender;



/**
 * actors in diff renderer
 */
std::vector<vtkSmartPointer<vtkActor> > vleftActors;
std::vector<vtkSmartPointer<vtkActor> > vimportActor;
vtkSmartPointer<vtkActor> midActor = NULL;
vtkSmartPointer<vtkActor> rightActor = NULL;
vtkSmartPointer<vtkActor> clipperActor = NULL;   //----------------------------------------

/**
 * polydata of the actors
 */
std::vector<vtkSmartPointer<vtkPolyData> > vleftPolydatas;
std::vector<vtkSmartPointer<vtkPolyData> > vimportPolydatas;
vtkSmartPointer<vtkPolyData> mid_data;
vtkSmartPointer<vtkPolyData> right_data;
vtkSmartPointer<vtkTriangleFilter> trifilter;     //the output of trifilter is polydata of the clipper



/**
  *ear
  */
vtkSmartPointer<vtkPolyData> ear = NULL;






/**
 * the plane souce for points placing
 */
vtkSmartPointer<vtkActor> thePlaneActor = NULL;

double *center;
double *bounds;
double g_normal[3];
double g_origin[3];
double g_distance = 0;

/**
 * return a actor of a parallel plane; every time the actor will new a memory
 */
void parallelplane(vtkRenderer* ren1, vtkSmartPointer<vtkActor> & thePlaneActor/*vtkPlaneSource* plane*/)
{
  if (thePlaneActor) {
//      pointPlacer->RemoveAllProps();
      leftrenderer->RemoveActor(thePlaneActor);
  }
  vtkSmartPointer<vtkPlaneSource> plane = vtkSmartPointer<vtkPlaneSource>::New();
  double a_view[3] = { 0.0,0.0,0.0 };
  double b_view[3] = { 0.0,1.0,0.0 };
  double c_view[3] = { 1.0,0.0,0.0 };
  ren1->ViewToWorld(a_view[0], a_view[1], a_view[2]);
  ren1->ViewToWorld(b_view[0], b_view[1], b_view[2]);
  ren1->ViewToWorld(c_view[0], c_view[1], c_view[2]);
  double e1[3] = { c_view[0] - a_view[0],
    c_view[1] - a_view[1],
    c_view[2] - a_view[2] };
  double e2[3] = { b_view[0] - a_view[0],
    b_view[1] - a_view[1],
    b_view[2] - a_view[2] };
  double normal[3];
  vtkMath::Cross(e2, e1, normal);
  vtkMath::Normalize(normal);

  plane->SetNormal(normal);
//  plane->SetOrigin(center);
//  plane->SetCenter(-1.43055, 1.42658, -13.849);
  plane->Update();

  if (frontPlane != NULL) {
      //------------------------------------------------test-----------
        frontPlane->SetNormal(normal);

        g_origin[0] = a_view[0];
        g_origin[1] = a_view[1];
        g_origin[2] = a_view[2];
        g_normal[0] = normal[0];
        g_normal[1] = normal[1];
        g_normal[2] = normal[2];
      //--------------------------------------------------------------------
  }

  //1,0,0
  double center_actor[3] = { 0.0, 0.0, 0.0 }, trans[3] = { 0.0, 0.0, 0.0 }, center_plane[3];
  plane->GetCenter(center_plane);

  if (vPoints1.size() > 0) {
      //  vtkSmartPointer<vtkPolyData> & polydata = vleftPolydatas[vleftPolydatas.size() - 1];

        for (int i = 0; i < vPoints1.size(); ++i) {
            for (int j = 0; j < 3; ++j) {
                center_actor[j] += vPoints1[i][j];
            }
        }
        for (int i = 0; i < 3; ++i) {
            center_actor[i] /= vPoints1.size();
        }

        //  polydata->GetCenter(center_actor);
          for (int i = 0 ; i < 3; ++i) {
              trans[i] = center_actor[i] - center_plane[i];
          }
  }



  vtkSmartPointer<vtkTransform> transform2 =
      vtkSmartPointer<vtkTransform>::New();
  transform2->Translate( trans[0], trans[1], trans[2]);

  vtkSmartPointer<vtkTransformPolyDataFilter> tfilter2 =
    vtkSmartPointer<vtkTransformPolyDataFilter>::New();

  tfilter2->SetInputData(plane->GetOutput());
  tfilter2->SetTransform(transform2);
  tfilter2->Update();

  vtkSmartPointer<vtkPolyData> newplae = tfilter2->GetOutput();
  newplae->GetCenter(center_plane);
  cout << "plane-center: " << center_plane[0] << " " << center_plane[1] << " " << center_plane[2] << endl;

  vtkSmartPointer<vtkDataSetMapper> planeMapper =
    vtkSmartPointer<vtkDataSetMapper>::New();
  planeMapper->SetInputData(tfilter2->GetOutput());
  thePlaneActor = vtkSmartPointer<vtkActor>::New();
  thePlaneActor->SetMapper(planeMapper);
  thePlaneActor->GetProperty()->SetColor(1.0000, 1.0, 1.0);
  thePlaneActor->GetProperty()->SetInterpolationToFlat();
  thePlaneActor->GetProperty()->SetOpacity(0.1);
  thePlaneActor->GetProperty()->SetLighting(1);
//  thePlaneActor->SetPosition(-2.0, -2.0, -2.0);
  thePlaneActor->SetScale(scale * 5);
//  pointPlacer->AddProp(thePlaneActor);
//  pointPlacer->AddProp(vleftActors[vleftActors.size() - 1]);
}
/**
 *
 */

double calscale(vtkActor* clipActor)
{
  double bounds[6];
  for (int cc = 0; cc < 6; cc++)
  {
    bounds[cc] = clipActor->GetBounds()[cc];
    cout << bounds[cc] << endl;
  }
  double xscale = fabs(bounds[1] - bounds[0]);
  double yscale = fabs(bounds[3] - bounds[2]);
  double zscale = fabs(bounds[5] - bounds[4]);
  double scale = xscale*yscale;
  if (xscale*zscale > scale)
  {
    scale = xscale*zscale;
  }
  else if (yscale*zscale > scale)
  {
    scale = yscale*zscale;
  }
  return scale;
}
/**
 *
 */


double maximumLength(double* _bounds)
{
  std::vector<double> vbounds;
  vbounds.push_back(_bounds[1] - _bounds[0]);
  vbounds.push_back(_bounds[3] - _bounds[2]);
  vbounds.push_back(_bounds[5] - _bounds[4]);
  std::vector<double>::iterator max = std::max_element(vbounds.begin(), vbounds.end());
  return *max;
}



///*
//draw points on the two pararel planes
//*/
//void drawPoint(double* p, vtkRenderer* r)
//{
//  // Create a sphere
//    vtkSmartPointer<vtkSphereSource> sphereSource =
//      vtkSmartPointer<vtkSphereSource>::New();
//    sphereSource->SetCenter(p[0], p[1], p[2]);
//    sphereSource->SetRadius(0.3);
//    sphereSource->Update();


//    vtkSmartPointer<vtkDataSetMapper> m = vtkSmartPointer<vtkDataSetMapper>::New();
//    m->SetInputConnection(sphereSource->GetOutputPort());
//    vtkSmartPointer<vtkActor> a = vtkSmartPointer<vtkActor>::New();
//    a->SetMapper(m);
//    r->AddActor(a);
//}


void addPoint(double* p, vtkRenderer* render)
{
  if(frontPlane == NULL) {
    cout << "plane" << endl;
    frontPlane = vtkSmartPointer<vtkPlane>::New();
/**
 * add the plane actor here for points placing
 */
    render->RemoveActor(thePlaneActor);
    parallelplane(render, thePlaneActor);
    render->AddActor(thePlaneActor);
    pointPlacer->AddProp(thePlaneActor);
  }
//  parallelplane(render, thePlaneActor);
//  render->AddActor(thePlaneActor);

  //front face

  double * point = new double[3];
  frontPlane->ProjectPoint(p, g_origin, g_normal, point);
  cout << point[0] << ", " << point[1] << ", " << point[2] << endl;
  double distance = sqrt(vtkMath::Distance2BetweenPoints(p, point));
  if (distance > g_distance) {
    g_distance = distance;
  }
  // cv::Vec3f vpoint(point[0], point[1], point[2]);
  vPoints1.push_back(point);

  render->Render();

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
  extrude->SetScaleFactor(1.5 * maximumLength(bounds) + 2 * g_distance);//1.5 * maximumLength(bounds) +

  trifilter = vtkSmartPointer<vtkTriangleFilter>::New();
  trifilter->SetInputConnection(extrude->GetOutputPort());



   vtkSmartPointer<vtkDataSetMapper> mapper =
    vtkSmartPointer<vtkDataSetMapper>::New();
  mapper->SetInputConnection(trifilter->GetOutputPort());

  clipperActor = vtkSmartPointer<vtkActor>::New();
  clipperActor->SetMapper(mapper);
  clipperActor->GetProperty()->SetColor(0.8900, 0.8100, 0.3400);

  leftrenderer->AddActor(clipperActor);

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
  booleanOperation->SetInputData( 1, vleftPolydatas[vleftPolydatas.size() - 1] );
  booleanOperation->SetInputData(0, trifilter->GetOutput()); // set the input data
  vtkSmartPointer<vtkPolyDataMapper> booleanOperationMapper =
  vtkSmartPointer<vtkPolyDataMapper>::New();
  booleanOperationMapper->SetInputConnection( booleanOperation->GetOutputPort() );
  booleanOperationMapper->ScalarVisibilityOff();
  midActor = vtkSmartPointer<vtkActor>::New();
  midActor->SetMapper( booleanOperationMapper );
  midrender->AddActor(midActor);
  mid_data = booleanOperation->GetOutput();


  vtkSmartPointer<vtkBooleanOperationPolyDataFilter> booleanOperation1 =
    vtkSmartPointer<vtkBooleanOperationPolyDataFilter>::New();
  booleanOperation1->SetOperationToDifference();
  booleanOperation1->SetInputData( 0, vleftPolydatas[vleftPolydatas.size() - 1] );
  booleanOperation1->SetInputData(1, trifilter->GetOutput());

  vtkSmartPointer<vtkPolyDataMapper> booleanOperationMapper1 =
  vtkSmartPointer<vtkPolyDataMapper>::New();
  booleanOperationMapper1->SetInputConnection( booleanOperation1->GetOutputPort() );
  booleanOperationMapper1->ScalarVisibilityOff();
  rightActor = vtkSmartPointer<vtkActor>::New();
  rightActor->SetMapper( booleanOperationMapper1 );
  rightrender->AddActor( rightActor );
  right_data = booleanOperation1->GetOutput();



}


/**
 * [clear all the points, and generate a new plane for points placing]
 * @param contour [the contour of a widget]
 */
void clearAllNodes(vtkOrientedGlyphContourRepresentation* contour, vtkRenderer* render)
{
  cout << "Delete all node" << endl;
  contour->ClearAllNodes();
  vPoints1.clear();
  frontPlane = NULL;
  leftrenderer->RemoveActor(thePlaneActor);
//  parallelplane(render, thePlaneActor);
  render->AddActor(thePlaneActor);
//  pointPlacer->AddProp(thePlaneActor);
}


///**
// * test mid and right
// */
//void showMid()
//{
//  vtkSmartPointer<vtkPolyDataMapper> booleanOperationMapper =
//  vtkSmartPointer<vtkPolyDataMapper>::New();
//  booleanOperationMapper->SetInputData(mid_data);
//  booleanOperationMapper->ScalarVisibilityOff();
//  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
//  actor->SetMapper( booleanOperationMapper );
//  midrender->AddActor(actor);
//  // cvWaitKey();
//  midrender->RemoveActor(actor);
//}

//void showRight()
//{
//  vtkSmartPointer<vtkPolyDataMapper> booleanOperationMapper =
//  vtkSmartPointer<vtkPolyDataMapper>::New();
//  booleanOperationMapper->SetInputData(right_data);
//  booleanOperationMapper->ScalarVisibilityOff();
//  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
//  actor->SetMapper( booleanOperationMapper );
//  rightrender->AddActor(actor);
//  // cvWaitKey();
//  rightrender->RemoveActor(actor);
//}



///*
//My extract function
//*/
//vtkPolyData* extractExtra(vtkPolyData *complete, vtkPolyData *part)
//{

//  vtkSmartPointer<vtkPolyData> data = vtkSmartPointer<vtkPolyData>::New();
//  vtkSmartPointer<vtkBooleanOperationPolyDataFilter> booleanOperation =
//    vtkSmartPointer<vtkBooleanOperationPolyDataFilter>::New();
//     booleanOperation->SetOperationToDifference();
//  booleanOperation->SetInputData( 0, complete );
//    booleanOperation->SetInputData( 1, part );
//    data = booleanOperation->GetOutput();

//    return data;
//}


/*
get the polydata
*/
vtkSmartPointer<vtkPolyData> getPolyData(QString filename)
{

  vtkSmartPointer<vtkSTLReader> reader = vtkSmartPointer<vtkSTLReader>::New();
  reader->SetFileName(filename.toLatin1().data());
  reader->Update();

  vtkSmartPointer<vtkTransform> pTransform = vtkSmartPointer<vtkTransform>::New();
    pTransform->Scale(0.1, 0.1, 0.1);
    vtkSmartPointer<vtkTransformPolyDataFilter> pTransformPolyDataFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    pTransformPolyDataFilter->SetInputData(reader->GetOutput());
    pTransformPolyDataFilter->SetTransform(pTransform);
    pTransformPolyDataFilter->Update();
  return pTransformPolyDataFilter->GetOutput();
//    vtkSmartPointer<vtkSphereSource> source =
//          vtkSmartPointer<vtkSphereSource>::New();
//        source->SetCenter(2, 3, 4);
//        source->SetRadius(0.5);
//        source->SetPhiResolution(20.);
//        source->SetThetaResolution(20.);
//        source->Update();

//        return source->GetOutput();
}



// }
/*
get the normal of the line
*/
//void getNormal(const double * v1, const double * v2, double *p3)
//{

//  vtkMath::Cross(v1, v2, p3);
//  vtkMath::Normalize(p3);

//}


//vtkSmartPointer<vtkPlane> getPlane(double* o, double* n)
//{
//  vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
//  plane->SetOrigin(o);
//  plane->SetNormal(n);
//  return plane;
//}


// Constructor
SideBySideRenderWindowsQt::SideBySideRenderWindowsQt()
{
  this->setupUi(this);

  this->m_Connections = vtkSmartPointer<vtkEventQtSlotConnect>::New();
//add a qaction
  this->openfile_act = new QAction(tr("&Open"), this);
  this->openfile_act->setShortcut(QKeySequence::Open);
    this->savefile_act = new QAction(tr("&Save"), this);
    this->savefile_act->setShortcut(QKeySequence::Save);
 this->setStatusTip(tr("Open a file."));
//add the qaction to a menu
    this->fileMenu = menuBar()->addMenu(tr("&File"));
    this->fileMenu->addAction(openfile_act);
    this->fileMenu->addAction(savefile_act);

    this->acContour = NULL;

    key_0_act = NULL;
    key_1_act = NULL;
    key_2_act = NULL;
    key_c_act = NULL;
    key_k_act = NULL;

    key_s_act = NULL;
    key_a_act = NULL;
    key_u_act = NULL;

    QToolBar * toolBar = addToolBar(tr("&toolBar"));
    this->key_0_act = new QAction(tr("&Back"), this);
    this->key_k_act = new QAction(tr("&DeleteLastPoint"), this);
    this->key_c_act = new QAction(tr("&clearAllPoints"), this);
    this->key_1_act = new QAction(tr("&select1"), this);
    this->key_2_act = new QAction(tr("&select2"), this);
    this->key_n_act = new QAction(tr("&cut"), this);

    this->import_act = new QAction(tr("import"), this);
    this->key_s_act = new QAction(tr("objectMode"), this);
    this->key_a_act = new QAction(tr("cameraMode"), this);
    this->key_u_act = new QAction(tr("union"), this);
    toolBar->addAction(key_0_act);
    toolBar->addAction(key_c_act);
    toolBar->addAction(key_k_act);
    toolBar->addAction(key_n_act);
    toolBar->addAction(key_1_act);
    toolBar->addAction(key_2_act);
    toolBar->addAction(import_act);
    toolBar->addAction(key_s_act);
    toolBar->addAction(key_a_act);
    toolBar->addAction(key_u_act);

    /*
        init three render windows
    */
    renderWindowLeft = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    renderWindowMid = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    renderWindowRight = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();

      this->qvtkWidgetLeft->SetRenderWindow(renderWindowLeft);

      this->qvtkWidgetMid->SetRenderWindow(renderWindowMid);

      this->qvtkWidgetRight->SetRenderWindow(renderWindowRight);

      // A renderer and render window
      leftrenderer = vtkSmartPointer<vtkRenderer>::New();
      leftrenderer->SetBackground(0.3, 0.3, 0.3);
      midrender = vtkSmartPointer<vtkRenderer>::New();
      midrender->SetBackground(0.35, 0.35, 0.35);
      rightrender = vtkSmartPointer<vtkRenderer>::New();
      rightrender->SetBackground(0.4, 0.4, 0.4);


      // VTK/Qt wedded
      renderWindowLeft->AddRenderer(leftrenderer);
      renderWindowMid->AddRenderer(midrender);
      renderWindowRight->AddRenderer(rightrender);


      renderWindowLeft->Render();
      renderWindowMid->Render();
      renderWindowRight->Render();


  connect(this->actionExit, SIGNAL(triggered()), this, SLOT(slotExit()));
  connect(this->openfile_act, SIGNAL(triggered()), this, SLOT(openFile()));
  connect(this->savefile_act, SIGNAL(triggered()), this, SLOT(saveFile()));

  connect(this->key_0_act, SIGNAL(triggered()), this, SLOT(back()));
  connect(this->key_1_act, SIGNAL(triggered()), this, SLOT(selectFirst()));
  connect(this->key_2_act, SIGNAL(triggered()), this, SLOT(selectSecond()));
  connect(this->key_c_act, SIGNAL(triggered()), this, SLOT(clearAllPoints()));
  connect(this->key_k_act, SIGNAL(triggered()), this, SLOT(deleteLastPoint()));
  connect(this->key_n_act, SIGNAL(triggered()), this, SLOT(cut()));

  connect(this->key_s_act, SIGNAL(triggered()), this, SLOT(cameraMode()));
  connect(this->key_a_act, SIGNAL(triggered()), this, SLOT(objectMode()));
  connect(this->key_u_act, SIGNAL(triggered()), this, SLOT(makeUnion()));
  connect(this->import_act, SIGNAL(triggered()), this, SLOT(importFile()));


}

void SideBySideRenderWindowsQt::mouse()
{

    if (acContour) {
        double po[3] = {0,0,0};
        int num;

      num = acContour->GetNumberOfNodes();
      acContour->GetNthNodeWorldPosition(num-1,po);
      if (CUTTING_MODE) addPoint(po, acContour->GetRenderer());
      renderWindowLeft->Render();
      renderWindowMid->Render();
      renderWindowRight->Render();
    }


}


void SideBySideRenderWindowsQt::keyboard()
{


    char getkey = this->renderWindowInteractor->GetKeyCode();

    if (getkey == '9') {
      leftrenderer->RemoveActor(thePlaneActor);
    }
    if (getkey == 'n') {

        if (acContour) {
            generateClipperData();

            generateClippedByBool();  //draw in mid and right

            midrender->SetActiveCamera(leftrenderer->GetActiveCamera());
            rightrender->SetActiveCamera(leftrenderer->GetActiveCamera());
            frontPlane = NULL;
        }
    }

    this->qvtkWidgetLeft->GetRenderWindow()->GetInteractor()->Render();
    this->qvtkWidgetMid->GetRenderWindow()->GetInteractor()->Render();
    this->qvtkWidgetRight->GetRenderWindow()->GetInteractor()->Render();
}


void SideBySideRenderWindowsQt::openFile()
{
    CUTTING_MODE = 1;
    frontPlane = NULL;
    clipSource = NULL;
    midActor = NULL;
    rightActor = NULL;
    cout << "init..." << endl;
    if (acContour != NULL) {
        acContour->ClearAllNodes();
    }
    vPoints1.clear();
    if (thePlaneActor) {
        leftrenderer->RemoveActor(thePlaneActor);
    }

    if (vleftActors.size() > 0) {
        leftrenderer->RemoveActor(vleftActors[vleftActors.size() - 1]);
    }
    vleftActors.clear();
    vleftPolydatas.clear();


    //pop, clear all datas to inition
    QString filename = QFileDialog :: getOpenFileName(this, NULL, NULL, "*.stl");

    clipSource = getPolyData(filename);


    assert(clipSource != NULL);

    // PolyData to process
    center = clipSource->GetCenter();
    cout << "center: " << center[0] << " " << center[1] << " " << center[2] << endl;
    bounds = clipSource->GetBounds();





    //  this->menuBar()->addMenu(this->fileMenu);


        //create mapper and actor for each polyData
        vtkSmartPointer<vtkDataSetMapper> sourceMapper =
          vtkSmartPointer<vtkDataSetMapper>::New();
        sourceMapper->SetInputData(clipSource);

        vtkSmartPointer<vtkActor> sourceActor =
          vtkSmartPointer<vtkActor>::New();
        sourceActor->SetMapper(sourceMapper);
        sourceActor->GetProperty()->SetColor(1.0000,0.3882,0.2784);
//        sourceActor->SetPosition(0, 0, 0);
        sourceActor->GetProperty()->SetInterpolationToFlat();
        /**
         *
         */
          scale = calscale(sourceActor);


          /**
           * add to the vector
           */
          vleftActors.push_back(sourceActor);
          vleftPolydatas.push_back(clipSource);


        leftrenderer->AddActor(sourceActor);

        leftrenderer->ResetCamera();
        midrender->ResetCamera();
        rightrender->ResetCamera();

        //------------------make a plane----------------------------
          parallelplane(leftrenderer, thePlaneActor);
          leftrenderer->AddActor(thePlaneActor);
        //---------------------------------------------------------------



        this->renderWindowInteractor = renderWindowLeft->GetInteractor();

    //----------------start call back----------------------------------

        this->acContour = vtkOrientedGlyphContourRepresentation::New();

        vtkContourWidget *contourWidget = vtkContourWidget::New();
        contourWidget->SetInteractor(renderWindowInteractor);
        contourWidget->SetRepresentation(acContour);
        pointPlacer = vtkSmartPointer<vtkPolygonalSurfacePointPlacer>::New();
        pointPlacer->AddProp(sourceActor);
        pointPlacer->AddProp(thePlaneActor);
        acContour->SetPointPlacer(pointPlacer);
        vtkLinearContourLineInterpolator * interpolator =
          vtkLinearContourLineInterpolator::New();
        acContour->SetLineInterpolator(interpolator);
        contourWidget->SetEnabled(1);

        renderWindowLeft->Render();

    //----------------end call back-------------------------------------------------


      // Set up action signals and slots
        this->m_Connections->Connect(renderWindowLeft->GetInteractor(),
                                     vtkCommand::LeftButtonReleaseEvent,
                                     this,
                                     SLOT(mouse()));
        this->m_Connections->Connect(renderWindowLeft->GetInteractor(),
                                     vtkCommand::KeyReleaseEvent,
                                     this,
                                     SLOT(keyboard()));
}

void SideBySideRenderWindowsQt::saveFile()
{
    //set a filename to save
     QString fileName = QFileDialog::getSaveFileName(this, tr("Open STL files") , "/home/", tr("STL files (*.stl)"));

     //write
     vtkSmartPointer<vtkSTLWriter> stlWriter =
       vtkSmartPointer<vtkSTLWriter>::New();
     stlWriter->SetFileName(fileName.toLatin1().data());
     stlWriter->SetInputData(vleftPolydatas[vleftPolydatas.size() - 1]);
     stlWriter->Write();
     cout << "done\n";
}

void SideBySideRenderWindowsQt::slotExit()
{
  qApp->exit();
}

void SideBySideRenderWindowsQt::deleteLastPoint()
{

    if (acContour)
    {
          cout << "Delete the last node" << endl;
        acContour->DeleteLastNode();
        vPoints1.pop_back();
        if (vPoints1.size() == 0) {
          cout << "no points left" << endl;
          clearAllNodes(acContour, leftrenderer);
          assert(frontPlane == NULL);
        }
        this->qvtkWidgetLeft->GetRenderWindow()->GetInteractor()->Render();
        this->qvtkWidgetMid->GetRenderWindow()->GetInteractor()->Render();
        this->qvtkWidgetRight->GetRenderWindow()->GetInteractor()->Render();
    }
}

void SideBySideRenderWindowsQt::clearAllPoints()
{
    if (acContour)
    {
        clearAllNodes(acContour, leftrenderer);
        this->qvtkWidgetLeft->GetRenderWindow()->GetInteractor()->Render();
        this->qvtkWidgetMid->GetRenderWindow()->GetInteractor()->Render();
        this->qvtkWidgetRight->GetRenderWindow()->GetInteractor()->Render();
    }
}

void SideBySideRenderWindowsQt::back()
{
    if (vleftActors.size() > 1)
    {
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
        assert(frontPlane == NULL);
        this->qvtkWidgetLeft->GetRenderWindow()->GetInteractor()->Render();
        this->qvtkWidgetMid->GetRenderWindow()->GetInteractor()->Render();
        this->qvtkWidgetRight->GetRenderWindow()->GetInteractor()->Render();
    }
}

void SideBySideRenderWindowsQt::selectFirst()
{
    if (midActor)
    {
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
        assert(frontPlane == NULL);
        this->qvtkWidgetLeft->GetRenderWindow()->GetInteractor()->Render();
        this->qvtkWidgetMid->GetRenderWindow()->GetInteractor()->Render();
        this->qvtkWidgetRight->GetRenderWindow()->GetInteractor()->Render();
    }
}

void SideBySideRenderWindowsQt::selectSecond()
{
    if (rightActor)
    {
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
        assert(frontPlane == NULL);
        this->qvtkWidgetLeft->GetRenderWindow()->GetInteractor()->Render();
        this->qvtkWidgetMid->GetRenderWindow()->GetInteractor()->Render();
        this->qvtkWidgetRight->GetRenderWindow()->GetInteractor()->Render();
    }
}

void SideBySideRenderWindowsQt::cut()
{
    if ( CUTTING_MODE == 1 && vPoints1.size() > 2)
    {
        generateClipperData();

        generateClippedByBool();  //draw in mid and right

        midrender->SetActiveCamera(leftrenderer->GetActiveCamera());
        rightrender->SetActiveCamera(leftrenderer->GetActiveCamera());
        frontPlane = NULL;
        this->qvtkWidgetLeft->GetRenderWindow()->GetInteractor()->Render();
        this->qvtkWidgetMid->GetRenderWindow()->GetInteractor()->Render();
        this->qvtkWidgetRight->GetRenderWindow()->GetInteractor()->Render();
    }
}

void SideBySideRenderWindowsQt::cameraMode()
{
    cout << "camera mode" << endl;
//    leftrenderer->RemoveActor(thePlaneActor);
//    thePlaneActor = NULL;
    vtkSmartPointer<vtkInteractorStyleTrackballCamera> style =
        vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    this->renderWindowInteractor->SetInteractorStyle(style);
}

void SideBySideRenderWindowsQt::objectMode()
{
    cout << "object mode" << endl;
//    leftrenderer->RemoveActor(thePlaneActor);
//    thePlaneActor = NULL;
    vtkSmartPointer<vtkInteractorStyleTrackballActor> style =
        vtkSmartPointer<vtkInteractorStyleTrackballActor>::New();
    this->renderWindowInteractor->SetInteractorStyle(style);
}

void SideBySideRenderWindowsQt::importFile()
{
     CUTTING_MODE = 0;
     leftrenderer->RemoveActor(thePlaneActor);

    //next version use mode to choose whether to edit or cut
    QString filename = QFileDialog :: getOpenFileName(this, NULL, NULL, "*.stl");

    ear = getPolyData(filename);
    assert(ear != NULL);

    vimportPolydatas.push_back(ear);

    vtkSmartPointer<vtkMatrix4x4> matr =
        vtkSmartPointer<vtkMatrix4x4>::New();
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (i == j) {
                matr->SetElement(i, j, 0.1);
            }
            else {
                matr->SetElement(i, j, 0.0);
            }
        }
    }
    matr->SetElement(3, 3, 1.0);
    vtkSmartPointer<vtkTransform> trans2 =
        vtkSmartPointer<vtkTransform>::New();
    trans2->SetMatrix(matr);
    vtkSmartPointer<vtkTransformPolyDataFilter> transdata2 =
        vtkSmartPointer<vtkTransformPolyDataFilter>::New();

    transdata2->SetInputData(ear);
    transdata2->SetTransform(trans2);
    transdata2->Update();
    ear = transdata2->GetOutput();

    vtkSmartPointer<vtkDataSetMapper> earMapper = vtkSmartPointer<vtkDataSetMapper>::New();
    earMapper->SetInputData(ear);
    cout << "--------" << "\n";
//    earMapper->Update();
    assert(earMapper != NULL);

    vtkSmartPointer<vtkActor> earActor1 = vtkSmartPointer<vtkActor>::New();
    earActor1->SetMapper(earMapper);
    earActor1->GetProperty()->SetColor(1.0000, 0.3882, 0.2784);
    earActor1->GetProperty()->SetInterpolationToFlat();
    vimportActor.push_back(earActor1);


//-----------------------------------------------
//    leftrenderer->AddActor(earActor1);
//    //create mapper and actor for each polyData

//    vtkSmartPointer<vtkDataSetMapper> sourceMapper =
//      vtkSmartPointer<vtkDataSetMapper>::New();
//    sourceMapper->SetInputData(vimportPolydatas[vimportPolydatas.size() - 1]);

//    vtkSmartPointer<vtkActor> sourceActor =
//      vtkSmartPointer<vtkActor>::New();
//    sourceActor->SetMapper(sourceMapper);
//    sourceActor->GetProperty()->SetColor(1.0000,0.3882,0.2784);
//        sourceActor->SetPosition(0, 0, 0);
//    sourceActor->SetOrigin(0, 0, 0);
//    sourceActor->GetProperty()->SetInterpolationToFlat();
//    vimportActor.push_back(sourceActor);

//    leftrenderer->AddActor(vimportActor[vimportActor.size() - 1]);
//    renderWindowLeft->Render();


//--------------------------------------------------------------------

//    vtkSmartPointer<vtkMatrix4x4> matr =
//        vtkSmartPointer<vtkMatrix4x4>::New();
//    for (int i = 0; i < 4; i++) {
//        for (int j = 0; j < 4; j++) {
//            if (i == j) {
//                matr->SetElement(i, j, 0.1);
//            }
//            else {
//                matr->SetElement(i, j, 0.0);
//            }
//        }
//    }
//    matr->SetElement(3, 3, 1.0);
//    vtkSmartPointer<vtkTransform> trans2 =
//        vtkSmartPointer<vtkTransform>::New();
//    trans2->SetMatrix(matr);
//    vtkSmartPointer<vtkTransformPolyDataFilter> transdata2 =
//        vtkSmartPointer<vtkTransformPolyDataFilter>::New();
//    transdata2->SetInputData(ear);
//    transdata2->SetTransform(trans2);
//    transdata2->Update();
//    ear = transdata2->GetOutput();
//    assert(ear != NULL);


//    earMapper->SetInputData(ear);
////    earMapper->Update();
//    assert(earMapper != NULL);
//    earActor1->SetMapper(earMapper);
//    earActor1->GetProperty()->SetColor(1.0000, 0.3882, 0.2784);
//    earActor1->GetProperty()->SetInterpolationToFlat();


////-----------------------------------------------
    leftrenderer->AddActor(earActor1);

}

void SideBySideRenderWindowsQt::makeUnion()
{
    cout << "processing union" << endl;

    vtkSmartPointer<vtkMatrix4x4> mat =
        vtkSmartPointer<vtkMatrix4x4>::New();

    vtkSmartPointer<vtkActor> & earActor1 = vimportActor[vimportActor.size() - 1];
    earActor1->GetMatrix(mat);
    vtkSmartPointer<vtkTransform> trans =
        vtkSmartPointer<vtkTransform>::New();
    trans->SetMatrix(mat);
    vtkSmartPointer<vtkPolyData>  ear1 =
        vtkSmartPointer<vtkPolyData>::New();
    ear1 = vimportPolydatas[vimportPolydatas.size() - 1];
    vtkSmartPointer<vtkTransformPolyDataFilter> transdata =
        vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    transdata->SetInputData(ear1);
    transdata->SetTransform(trans);
    transdata->Update();
    ear1 = transdata->GetOutput();
    /*vtkSmartPointer<vtkDataSetMapper> earMapper =
        vtkSmartPointer<vtkDataSetMapper>::New();
    earMapper->SetInputData(ear);
    earActor->SetMapper(earMapper);
    leftrenderer->AddActor(earActor);*/

    vtkSmartPointer<vtkMatrix4x4> mat1 =
        vtkSmartPointer<vtkMatrix4x4>::New();
    vtkSmartPointer<vtkActor> & sActor = vleftActors[vleftActors.size() - 1];
    sActor->GetMatrix(mat1);

    vtkSmartPointer<vtkTransform> trans1 =
        vtkSmartPointer<vtkTransform>::New();
    trans1->SetMatrix(mat1);
    vtkSmartPointer<vtkPolyData> source =
        vtkSmartPointer<vtkPolyData>::New();
    source = clipSource;
    vtkSmartPointer<vtkTransformPolyDataFilter> transdata1 =
        vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    transdata1->SetInputData(source);
    transdata1->SetTransform(trans1);
    transdata1->Update();
    clipSource = transdata1->GetOutput();

    vtkSmartPointer<vtkBooleanOperationPolyDataFilter> booleanOperation =
        vtkSmartPointer<vtkBooleanOperationPolyDataFilter>::New();
    booleanOperation->SetOperationToUnion();
    booleanOperation->SetInputData(0, clipSource); // set the input data
    booleanOperation->SetInputData(1, ear1);

    vtkSmartPointer<vtkPolyDataMapper> booleanOperationMapper =
        vtkSmartPointer<vtkPolyDataMapper>::New();
    booleanOperationMapper->SetInputConnection(booleanOperation->GetOutputPort());
    booleanOperationMapper->ScalarVisibilityOff();

    vtkSmartPointer<vtkActor> booleanOperationActor =
        vtkSmartPointer<vtkActor>::New();
    booleanOperationActor->SetMapper(booleanOperationMapper);

    string file = "Union.stl";
    vtkSmartPointer<vtkSTLWriter> writer =
        vtkSmartPointer<vtkSTLWriter>::New();
    writer->SetInputConnection(booleanOperation->GetOutputPort());
    writer->SetFileName(file.c_str());
    writer->Write();

    leftrenderer->AddActor(booleanOperationActor);
}




