#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "all_headers.h"

using namespace std;

static bool FIRSTOPEN = true;

vtkSmartPointer<vtkPolygonalSurfacePointPlacer> pointPlacer;

vtkSmartPointer<vtkPolyData> clipSource = NULL;

//std::vector<cv::Vec3f> vPoints1;
std::vector<double*> vPoints1;


vtkSmartPointer<vtkPlane> frontPlane = NULL;

/**
 * renderer in diff view port
 */
vtkSmartPointer<vtkRenderer> leftrenderer = NULL;
vtkSmartPointer<vtkRenderer> midrender = NULL;
vtkSmartPointer<vtkRenderer> rightrender = NULL;


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
vtkSmartPointer<vtkPolyData> mid_data = NULL;
vtkSmartPointer<vtkPolyData> right_data = NULL;
vtkSmartPointer<vtkTriangleFilter> trifilter = NULL;     //the output of trifilter is polydata of the clipper


/**
  *ear
  */
vtkSmartPointer<vtkPolyData> ear = NULL;


/**
 * the plane souce for points placing
 */
vtkSmartPointer<vtkActor> thePlaneActor = NULL;

vtkSmartPointer<vtkMatrix4x4> remat = vtkSmartPointer<vtkMatrix4x4>::New();

double *center;
double *bounds;
double g_normal[3];
double g_origin[3];
double g_distance = 0;
/**
 * scale of the plane to ensure enough area
 */
double scale = 1;
bool CUTTING_MODE = 0;



/**
 *@brief remove the visible plane
 * [need re-render]
 */
void removeThePlane()
{
    if (thePlaneActor) {
        leftrenderer->RemoveActor(thePlaneActor);
        pointPlacer->RemoveViewProp(thePlaneActor);
        thePlaneActor = NULL;
    }
}


/**
 * @brief removeRecentActor
 * remove the point placing method on the recent actor
 * [need re-render]
 */
void removeRecentActor()
{
    if (vleftActors.size() > 0) {
        vtkSmartPointer<vtkActor> & actor = vleftActors[vleftActors.size() - 1];
        pointPlacer->RemoveViewProp(actor);
    }
}


/**
 * @brief generate a visible plane parallel to the curent screen
 * @param ren1
 * @param thePlaneActor
 * [need re-render]
 */
void parallelplane(vtkRenderer* ren1, vtkSmartPointer<vtkActor> & thePlaneActor)
{
  if (thePlaneActor) {
     removeThePlane();
  }
  thePlaneActor = vtkSmartPointer<vtkActor>::New();

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

  vtkSmartPointer<vtkDataSetMapper> planeMapper =
    vtkSmartPointer<vtkDataSetMapper>::New();
  planeMapper->SetInputData(plane->GetOutput());
  thePlaneActor = vtkSmartPointer<vtkActor>::New();
  thePlaneActor->SetMapper(planeMapper);
  thePlaneActor->GetProperty()->SetColor(1.0000, 1.0, 1.0);
  thePlaneActor->GetProperty()->SetInterpolationToFlat();
  thePlaneActor->GetProperty()->SetOpacity(0.1);
  thePlaneActor->GetProperty()->SetLighting(1);
  thePlaneActor->SetScale(scale * 5);
  pointPlacer->AddProp(thePlaneActor);
}


/**
 * @brief generate a plane parallel to the curent screen for cutting
 * @param ren1
 */
void parallelplane(vtkRenderer* ren1)
{

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


      //------------------------------------------------test-----------
    if (frontPlane) {
        frontPlane->SetNormal(normal);

        g_origin[0] = a_view[0];
        g_origin[1] = a_view[1];
        g_origin[2] = a_view[2];
        g_normal[0] = normal[0];
        g_normal[1] = normal[1];
        g_normal[2] = normal[2];
    }
      //--------------------------------------------------------------------

}


/**
 * @brief helpful function for calculating appropriate scale
 * @param clipActor
 * @return scale
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
double maximumLength(double* _bounds)
{
  std::vector<double> vbounds;
  vbounds.push_back(_bounds[1] - _bounds[0]);
  vbounds.push_back(_bounds[3] - _bounds[2]);
  vbounds.push_back(_bounds[5] - _bounds[4]);
  std::vector<double>::iterator max = std::max_element(vbounds.begin(), vbounds.end());
  return *max;
}
double mod()
{
  double p[3] = { bounds[1] - bounds[0],
                  bounds[3] - bounds[2],
                  bounds[5] - bounds[4] };
  return p[0] * p[0] + p[1] * p[1] + p[2] * p[2];
}



/**
 * @brief add points, call this every time set a point on the actor
 * @param p
 * @param render
 * [need re-render]
 * [frontPlane : NULL :when no points
 *          NOT-NULL: when add any point
 *          TURN-NULL: when cutting is done]
 */
void addPoint(double* p, vtkRenderer* render)
{

    if (CUTTING_MODE) {
        if(frontPlane == NULL) {
          cout << "generate a plane" << endl;
          frontPlane = vtkSmartPointer<vtkPlane>::New();
      /**
       * add the plane actor here for points placing
       */
          parallelplane(render);
        }

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
    }

}


/**
 * @brief generate ClipperData
 */
void generateClipperData()
{
  if (CUTTING_MODE) {
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

}

void generateClippedByBool()
{

    if (CUTTING_MODE) {

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
}


/**
 * @brief get PolyData
 * @param filename
 * @return
 */
vtkSmartPointer<vtkPolyData> getPolyData(QString filename)
{

  vtkSmartPointer<vtkSTLReader> reader = vtkSmartPointer<vtkSTLReader>::New();
  reader->SetFileName(filename.toLatin1().data());
  reader->Update();
  return reader->GetOutput();

}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
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

    key_u_act = NULL;

    QToolBar * toolBar = addToolBar(tr("&toolBar"));
    this->key_0_act = new QAction(tr("&Back"), this);
    this->key_k_act = new QAction(tr("&DeleteLastPoint"), this);
    this->key_c_act = new QAction(tr("&clearAllPoints"), this);
    this->key_1_act = new QAction(tr("&select1"), this);
    this->key_2_act = new QAction(tr("&select2"), this);
    this->key_n_act = new QAction(tr("&cut"), this);

    this->import_act = new QAction(tr("import"), this);
    this->key_u_act = new QAction(tr("union"), this);
    this->addpaper_act = new QAction(tr("addPaper"), this);
    this->reset_act = new QAction(tr("reset"), this);

    toolBar->addAction(key_0_act);
    toolBar->addAction(key_c_act);
    toolBar->addAction(key_k_act);
    toolBar->addAction(key_n_act);
    toolBar->addAction(key_1_act);
    toolBar->addAction(key_2_act);
    toolBar->addAction(import_act);
    toolBar->addAction(key_u_act);
    toolBar->addAction(addpaper_act);
    toolBar->addAction(reset_act);

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

    connect(this->key_u_act, SIGNAL(triggered()), this, SLOT(makeUnion()));
    connect(this->import_act, SIGNAL(triggered()), this, SLOT(importFile()));
    connect(this->addpaper_act, SIGNAL(triggered()), this, SLOT(addPaper()));
    connect(this->reset_act, SIGNAL(triggered()), this, SLOT(reSet()));
//    connect(this->scale_down_act, SIGNAL(triggered()), this, SLOT(scaling_down()));
//    connect(this->scale_up_act, SIGNAL(triggered()), this, SLOT(scaling_up()));


}

void MainWindow::reSet()
{
    if (acContour != NULL) {
        acContour->ClearAllNodes();
    }
    vPoints1.clear();

    if (vleftActors.size() > 0) {
        leftrenderer->RemoveActor(vleftActors[vleftActors.size() - 1]);
        removeRecentActor();
    }
    if (thePlaneActor) {
        removeThePlane();
    }

    leftrenderer->RemoveAllViewProps();
    rightrender->RemoveAllViewProps();
    midrender->RemoveAllViewProps();


    vleftActors.clear();
    vleftPolydatas.clear();
    vimportPolydatas.clear();
    vimportActor.clear();

    thePlaneActor = NULL;
    ear = NULL;
    CUTTING_MODE = 1;
    frontPlane = NULL;
    clipSource = NULL;
    midActor = NULL;
    rightActor = NULL;
    this->acContour = NULL;
    midActor = NULL;
    rightActor = NULL;
    mid_data = NULL;
    right_data = NULL;
    trifilter = NULL;
    remat->Zero();

    reRenderAll();
}

void MainWindow::mouse()
{

    if (acContour) {
        double po[3] = {0,0,0};
        int num;

      num = acContour->GetNumberOfNodes();
      acContour->GetNthNodeWorldPosition(num-1,po);
      if (CUTTING_MODE) addPoint(po, acContour->GetRenderer());
    }
    reRenderAll();
}


void MainWindow::keyboard()
{

    char getkey = this->renderWindowInteractor->GetKeyCode();
    if (acContour) {

        if (getkey == 'n') {
            this->cut();
        }

        if (getkey == '1') {
            this->cameraMode();
        }

        if (getkey == '2') {
            this->objectMode();
        }

        if (getkey == ',') {
            this->scaling_down();
        }

        if (getkey == '.') {
            this->scaling_up();
        }
    }
    reRenderAll();

}

void MainWindow::openFile()
{
    if (acContour == NULL) {
        CUTTING_MODE = 1;
        frontPlane = NULL;
        clipSource = NULL;
        midActor = NULL;
        rightActor = NULL;
        cout << "init..." << endl;
        remat->Zero();


//        if (acContour != NULL) {
//            acContour->ClearAllNodes();
//        }
//        vPoints1.clear();

//        if (vleftActors.size() > 0) {
//            leftrenderer->RemoveActor(vleftActors[vleftActors.size() - 1]);
//            removeRecentActor();
//        }
//        if (thePlaneActor) {
//            removeThePlane();
//        }

//        vleftActors.clear();
//        vleftPolydatas.clear();


        //pop, clear all datas to inition
        QString filename = QFileDialog :: getOpenFileName(this, NULL, NULL, "*.stl");

        clipSource = getPolyData(filename);


        assert(clipSource != NULL);

        // PolyData to process
        center = clipSource->GetCenter();
        cout << "center: " << center[0] << " " << center[1] << " " << center[2] << endl;
        bounds = clipSource->GetBounds();

        //create mapper and actor for each polyData
        vtkSmartPointer<vtkDataSetMapper> sourceMapper =
        vtkSmartPointer<vtkDataSetMapper>::New();
        sourceMapper->SetInputData(clipSource);

        vtkSmartPointer<vtkActor> sourceActor =
        vtkSmartPointer<vtkActor>::New();
        sourceActor->SetMapper(sourceMapper);
        sourceActor->GetProperty()->SetColor(1.0000,0.3882,0.2784);
        sourceActor->GetProperty()->SetInterpolationToFlat();

        scale = calscale(sourceActor);

        // vector
        vleftActors.push_back(sourceActor);
        vleftPolydatas.push_back(clipSource);


        leftrenderer->AddActor(sourceActor);

        leftrenderer->ResetCamera();
        midrender->ResetCamera();
        rightrender->ResetCamera();

        //------------------make a plane----------------------------
//        parallelplane(leftrenderer);                  ??
        //---------------------------------------------------------------



        this->renderWindowInteractor = renderWindowLeft->GetInteractor();
        this->renderWindowInteractor->ReInitialize();

        //----------------start call back----------------------------------

        this->acContour = vtkOrientedGlyphContourRepresentation::New();

        vtkContourWidget *contourWidget = vtkContourWidget::New();
        contourWidget->SetInteractor(renderWindowInteractor);
        contourWidget->SetRepresentation(acContour);
        pointPlacer = vtkSmartPointer<vtkPolygonalSurfacePointPlacer>::New();
        pointPlacer->AddProp(sourceActor);
        acContour->SetPointPlacer(pointPlacer);
        vtkLinearContourLineInterpolator * interpolator =
        vtkLinearContourLineInterpolator::New();
        acContour->SetLineInterpolator(interpolator);
        contourWidget->SetEnabled(1);


        //----------------end call back-------------------------------------------------


    }
    if (FIRSTOPEN) {
        // Set up action signals and slots
        this->m_Connections->Connect(renderWindowLeft->GetInteractor(),
                 vtkCommand::LeftButtonReleaseEvent,
                 this,
                 SLOT(mouse()));
        this->m_Connections->Connect(renderWindowLeft->GetInteractor(),
                 vtkCommand::RightButtonReleaseEvent,
                 this,
                 SLOT(mouse()));
        this->m_Connections->Connect(renderWindowLeft->GetInteractor(),
                 vtkCommand::KeyReleaseEvent,
                 this,
                 SLOT(keyboard()));
        FIRSTOPEN = false;
    }
    reRenderAll();
}


/**
 * @brief MainWindow::save File
 */
void MainWindow::saveFile()
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

/**
 * @brief MainWindow::slotExit
 */
void MainWindow::slotExit()
{
  qApp->exit();
}

void MainWindow::deleteLastPoint()
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
    }
    reRenderAll();
}

void MainWindow::clearAllPoints()
{
    if (acContour)
    {
        clearAllNodes(acContour, leftrenderer);
    }
    reRenderAll();
}


/**
 * @brief MainWindow::go backwards
 */
void MainWindow::back()
{
    if (vleftActors.size() > 1)
    {
        assert(vleftActors.size() == vleftPolydatas.size());
        leftrenderer->RemoveActor(clipperActor);
        if (thePlaneActor)
            removeThePlane();
        if (vleftActors.size() != 1) {
          removeRecentActor();  //------remove placer on actor--

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
    }
    reRenderAll();
}


/**
 * @brief SideBySideRenderWindowsQt::select First window(mid-window)
 */
void MainWindow::selectFirst()
{
    if (midActor)
    {
        leftrenderer->RemoveActor(vleftActors[vleftActors.size() - 1]);
        leftrenderer->RemoveActor(clipperActor);
        removeRecentActor();
        if (thePlaneActor)
            removeThePlane();
        midrender->RemoveActor(midActor);
        rightrender->RemoveActor(rightActor);
        vleftActors.push_back(midActor);
        vleftPolydatas.push_back(mid_data);
        clearAllNodes(acContour, leftrenderer);
        leftrenderer->AddActor(vleftActors[vleftActors.size() - 1]);
        pointPlacer->AddProp(vleftActors[vleftActors.size() - 1]);
        assert(frontPlane == NULL);
    }
    reRenderAll();
}


/**
 * @brief SideBySideRenderWindowsQt::select Second
 */
void MainWindow::selectSecond()
{
    if (rightActor)
    {
        removeRecentActor();
        leftrenderer->RemoveActor(vleftActors[vleftActors.size() - 1]);
        leftrenderer->RemoveActor(clipperActor);
        if (thePlaneActor)
            removeThePlane();
        midrender->RemoveActor(midActor);
        rightrender->RemoveActor(rightActor);
        vleftActors.push_back(rightActor);
        vleftPolydatas.push_back(right_data);
        clearAllNodes(acContour, leftrenderer);
        leftrenderer->AddActor(vleftActors[vleftActors.size() - 1]);
        pointPlacer->AddProp(vleftActors[vleftActors.size() - 1]);
        assert(frontPlane == NULL);
    }
    reRenderAll();
}


void MainWindow::cut()
{
    if ( CUTTING_MODE == 1 && vPoints1.size() > 2)
    {
        generateClipperData();
        generateClippedByBool();  //draw in mid and right

        midrender->SetActiveCamera(leftrenderer->GetActiveCamera());
        rightrender->SetActiveCamera(leftrenderer->GetActiveCamera());
        frontPlane = NULL;
        if (thePlaneActor)
            removeThePlane();
    }
    reRenderAll();
}


void MainWindow::cameraMode()
{
    cout << "camera mode" << endl;
    vtkSmartPointer<vtkInteractorStyleTrackballCamera> style =
        vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    this->renderWindowInteractor->SetInteractorStyle(style);
}

void MainWindow::objectMode()
{
    cout << "object mode" << endl;
    vtkSmartPointer<vtkInteractorStyleTrackballActor> style =
        vtkSmartPointer<vtkInteractorStyleTrackballActor>::New();
    this->renderWindowInteractor->SetInteractorStyle(style);
}

void MainWindow::importFile()
{
    CUTTING_MODE = 0;
    if (thePlaneActor)
        removeThePlane();

    //next version use mode to choose whether to edit or cut
    QString filename = QFileDialog :: getOpenFileName(this, NULL, NULL, "*.stl");

    vtkSmartPointer<vtkPolyData> ear = getPolyData(filename);
    assert(ear != NULL);

    vimportPolydatas.push_back(ear);

    vtkSmartPointer<vtkDataSetMapper> earMapper = vtkSmartPointer<vtkDataSetMapper>::New();
    earMapper->SetInputData(ear);

    assert(earMapper != NULL);

    vtkSmartPointer<vtkActor> earActor1 = vtkSmartPointer<vtkActor>::New();
    earActor1->SetMapper(earMapper);
    earActor1->GetProperty()->SetColor(1.0000, 0.3882, 0.2784);
    earActor1->GetProperty()->SetInterpolationToFlat();
    vimportActor.push_back(earActor1);
    leftrenderer->AddActor(earActor1);

    reRenderAll();

}



void MainWindow::makeUnion()
{
    if (CUTTING_MODE == 0 && vimportPolydatas.size() > 0) {
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

        vtkSmartPointer<vtkMatrix4x4> mat1 =
            vtkSmartPointer<vtkMatrix4x4>::New();
        vtkSmartPointer<vtkActor> & sActor = vleftActors[vleftActors.size() - 1];
        sActor->GetMatrix(mat1);

        vtkSmartPointer<vtkTransform> trans1 =
            vtkSmartPointer<vtkTransform>::New();
        trans1->SetMatrix(mat1);
        vtkSmartPointer<vtkPolyData> source =
            vtkSmartPointer<vtkPolyData>::New();
        source = vleftPolydatas[vleftPolydatas.size() - 1];
        vtkSmartPointer<vtkTransformPolyDataFilter> transdata1 =
            vtkSmartPointer<vtkTransformPolyDataFilter>::New();
        transdata1->SetInputData(source);
        transdata1->SetTransform(trans1);
        transdata1->Update();
        source = transdata1->GetOutput();

        vtkSmartPointer<vtkBooleanOperationPolyDataFilter> booleanOperation =
            vtkSmartPointer<vtkBooleanOperationPolyDataFilter>::New();
        booleanOperation->SetOperationToUnion();
        booleanOperation->SetInputData(0, source); // set the input data
        booleanOperation->SetInputData(1, ear1);

        vtkSmartPointer<vtkPolyDataMapper> booleanOperationMapper =
            vtkSmartPointer<vtkPolyDataMapper>::New();
        booleanOperationMapper->SetInputConnection(booleanOperation->GetOutputPort());
        booleanOperationMapper->ScalarVisibilityOff();

        vtkSmartPointer<vtkActor> booleanOperationActor =
            vtkSmartPointer<vtkActor>::New();
        booleanOperationActor->SetMapper(booleanOperationMapper);

        leftrenderer->RemoveAllViewProps();

        leftrenderer->AddActor(booleanOperationActor);

        booleanOperation->Update();
        vleftPolydatas.push_back(booleanOperation->GetOutput());
        vleftActors.push_back(booleanOperationActor);
    }
    reRenderAll();
}


void MainWindow::addPaper()
{
    parallelplane(leftrenderer, thePlaneActor);
    leftrenderer->AddActor(thePlaneActor);
    reRenderAll();
}

/**
 * @brief reRenderAll
 * rerender all the render window
 */
void MainWindow::reRenderAll()
{
    this->qvtkWidgetLeft->GetRenderWindow()->GetInteractor()->Render();
    this->qvtkWidgetMid->GetRenderWindow()->GetInteractor()->Render();
    this->qvtkWidgetRight->GetRenderWindow()->GetInteractor()->Render();
}


void MainWindow::scaling_down()
{
    vtkSmartPointer<vtkPolyData>  ear = vimportPolydatas[vimportPolydatas.size() - 1];

    int flag = 0;
    vtkSmartPointer<vtkMatrix4x4> remat1 = vtkSmartPointer<vtkMatrix4x4>::New();
    vtkSmartPointer<vtkActor> & earActor = vimportActor[vimportActor.size() - 1];
    earActor->GetMatrix(remat1);
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (remat1->GetElement(i, j) == remat->GetElement(i, j)) {
                flag = flag + 1;
            }
            else {
                remat->SetElement(i, j, remat1->GetElement(i, j));
            }
        }
    }
    if (flag != 16) {
        vtkSmartPointer<vtkTransform> trans =
            vtkSmartPointer<vtkTransform>::New();
        trans->SetMatrix(remat1);
        vtkSmartPointer<vtkTransformPolyDataFilter> transdata =
            vtkSmartPointer<vtkTransformPolyDataFilter>::New();
        transdata->SetInputData(ear);
        transdata->SetTransform(trans);
        transdata->Update();
        ear = transdata->GetOutput();
    }

    vtkSmartPointer<vtkMatrix4x4> matr = vtkSmartPointer<vtkMatrix4x4>::New();
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (i == j) {
                matr->SetElement(i, j, 0.9);
            }
            else {
                matr->SetElement(i, j, 0.0);
            }
        }
    }
    matr->SetElement(3, 3, 1.0);
    vtkSmartPointer<vtkTransform> trans =
        vtkSmartPointer<vtkTransform>::New();
    trans->SetMatrix(matr);
    vtkSmartPointer<vtkTransformPolyDataFilter> transdata =
        vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    transdata->SetInputData(ear);
    transdata->SetTransform(trans);
    transdata->Update();
    ear = transdata->GetOutput();

    vimportPolydatas.pop_back();
    vimportPolydatas.push_back(ear);

    vtkSmartPointer<vtkDataSetMapper> earMapper =
        vtkSmartPointer<vtkDataSetMapper>::New();
    earMapper->SetInputData(ear);

    vtkSmartPointer<vtkActor> earActor1 = vtkSmartPointer<vtkActor>::New();
    earActor1->SetMapper(earMapper);
    earActor1->GetProperty()->SetColor(1.0000, 0.3882, 0.2784);
    earActor1->GetProperty()->SetInterpolationToFlat();
    leftrenderer->RemoveActor(vimportActor[vimportActor.size() - 1]);   //last actor
    vimportActor.pop_back();
    vimportActor.push_back(earActor1);
    leftrenderer->AddActor(earActor1);

    reRenderAll();
}

void MainWindow::scaling_up()
{
    vtkSmartPointer<vtkPolyData>  ear = vimportPolydatas[vimportPolydatas.size() - 1];

    int flag = 0;
    vtkSmartPointer<vtkMatrix4x4> remat1 = vtkSmartPointer<vtkMatrix4x4>::New();
    vtkSmartPointer<vtkActor> & earActor = vimportActor[vimportActor.size() - 1];
    earActor->GetMatrix(remat1);
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (remat1->GetElement(i, j) == remat->GetElement(i, j)) {
                flag = flag + 1;
            }
            else {
                remat->SetElement(i, j, remat1->GetElement(i, j));
            }
        }
    }
    if (flag != 16) {
        vtkSmartPointer<vtkTransform> trans =
            vtkSmartPointer<vtkTransform>::New();
        trans->SetMatrix(remat1);
        vtkSmartPointer<vtkTransformPolyDataFilter> transdata =
            vtkSmartPointer<vtkTransformPolyDataFilter>::New();
        transdata->SetInputData(ear);
        transdata->SetTransform(trans);
        transdata->Update();
        ear = transdata->GetOutput();
    }


    vtkSmartPointer<vtkMatrix4x4> matr =
        vtkSmartPointer<vtkMatrix4x4>::New();
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (i == j) {
                matr->SetElement(i, j, 1.1);
            }
            else {
                matr->SetElement(i, j, 0.0);
            }
        }
    }
    matr->SetElement(3, 3, 1.0);
    vtkSmartPointer<vtkTransform> trans =
        vtkSmartPointer<vtkTransform>::New();
    trans->SetMatrix(matr);
    vtkSmartPointer<vtkTransformPolyDataFilter> transdata =
        vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    transdata->SetInputData(ear);
    transdata->SetTransform(trans);
    transdata->Update();
    ear = transdata->GetOutput();

    vimportPolydatas.pop_back();
    vimportPolydatas.push_back(ear);

    vtkSmartPointer<vtkDataSetMapper> earMapper =
        vtkSmartPointer<vtkDataSetMapper>::New();
    earMapper->SetInputData(ear);

    vtkSmartPointer<vtkActor> earActor1 = vtkSmartPointer<vtkActor>::New();
    earActor1->SetMapper(earMapper);
    earActor1->GetProperty()->SetColor(1.0000, 0.3882, 0.2784);
    earActor1->GetProperty()->SetInterpolationToFlat();
    leftrenderer->RemoveActor(vimportActor[vimportActor.size() - 1]);   //last actor
    vimportActor.pop_back();
    vimportActor.push_back(earActor1);
    leftrenderer->AddActor(earActor1);

    reRenderAll();

}








