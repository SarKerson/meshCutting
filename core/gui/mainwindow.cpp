#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "all_headers.h"

#include "../tetgen/include/tetgen.h"
#include "../cellPicker/include/cellPicker.h"

using namespace std;

static bool FIRSTOPEN = true;

vtkSmartPointer<vtkPolygonalSurfacePointPlacer> pointPlacer;

vtkSmartPointer<vtkPolyData> clipSource = NULL;

std::vector<double*> g_vclick_points1;


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



//data used to add the prosthesis
double left[4] = { 0.0,0.0,0.0,1.0 };
double right[4] = { 0.0,0.0,0.0,1.0 };
double leftn[4] = { 0.0,0.0,0.0,1.0 };
double rightn[4] = { 0.0,0.0,0.0,1.0 };
double enormal[3] = { 0,0,0 };

//the center of the model
double pcenter[3] = { 0,0,0 };

//the ID of the selected cells
int idl=0, idr=0, idz=0, idy=0, idp=0, ide=0;

//the data used to rotate
double u[3];
double angle = -3.1415926/12;


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
//    cout << bounds[cc] << endl;
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
//          cout << "generate a plane" << endl;
          frontPlane = vtkSmartPointer<vtkPlane>::New();
      /**
       * add the plane actor here for points placing
       */
          parallelplane(render);
        }

        double * point = new double[3];
        frontPlane->ProjectPoint(p, g_origin, g_normal, point);
//        cout << point[0] << ", " << point[1] << ", " << point[2] << endl;
        double distance = sqrt(vtkMath::Distance2BetweenPoints(p, point));
        if (distance > g_distance) {
          g_distance = distance;
        }
        // cv::Vec3f vpoint(point[0], point[1], point[2]);
        g_vclick_points1.push_back(point);

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
      for(int i = 0; i < g_vclick_points1.size(); ++i) {
        points->InsertNextPoint(g_vclick_points1[i][0], g_vclick_points1[i][1], g_vclick_points1[i][2]);
      }
      vtkSmartPointer<vtkPolygon> polygon4 = vtkSmartPointer<vtkPolygon>::New();
      polygon4->GetPointIds()->SetNumberOfIds(g_vclick_points1.size());
      for(int i = 0; i < g_vclick_points1.size(); ++i) {
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
      clipperActor->GetProperty()->SetOpacity(0.1);
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
  g_vclick_points1.clear();
  frontPlane = NULL;
}


/**
 * @brief get PolyData
 * @param filename
 * @return
 */
vtkSmartPointer<vtkPolyData> getPolyData(QString filename)
{
  // vtkSmartPointer<vtkOBJReader> reader = vtkSmartPointer<vtkOBJReader>::New();

  vtkSmartPointer<vtkSTLReader> reader = vtkSmartPointer<vtkSTLReader>::New();
  reader->SetFileName(filename.toLatin1().data());
  reader->Update();
  return reader->GetOutput();

}


//the class used to get information of cells.
class CellPickerInteractorStyle : public vtkInteractorStyleTrackballCamera
{
public:
  static CellPickerInteractorStyle*New();
  char mode;

  CellPickerInteractorStyle()
  {
    selectedMapper = vtkSmartPointer<vtkDataSetMapper>::New();
    selectedActor = vtkSmartPointer<vtkActor>::New();
  }
  void SetRenderer(vtkRenderer* crenderer) {
    renderer = crenderer;
  }
  void SetData(vtkPolyData* data) {
    polyData = data;
  }
  void SetMode(char input) {
    mode = input;
  }

  //  virtual void Execute(vtkObject* caller, unsigned long eventId, void* callData)
  virtual void OnLeftButtonDown()
  {
    static int count = 0;

    int* pos = this->GetInteractor()->GetEventPosition();

    vtkSmartPointer<vtkCellPicker> picker =
      vtkSmartPointer<vtkCellPicker>::New();
    picker->SetTolerance(0.0005);
    picker->Pick(pos[0], pos[1], 0, leftrenderer);

    if (picker->GetCellId() != -1)
    {
      double normal[3];
      double pois[3];
      double poi1[3];
      double poi2[3];
      double poi3[3];

      cout << "CellID:" << picker->GetCellId() << endl;

      vtkSmartPointer<vtkIdList> list =
        vtkSmartPointer<vtkIdList>::New();
      polyData->GetCellPoints(picker->GetCellId(), list);
      polyData->GetPoint(list->GetId(0), poi1);
      polyData->GetPoint(list->GetId(1), poi2);
      polyData->GetPoint(list->GetId(2), poi3);
      pois[0] = (poi1[0] + poi2[0] + poi3[0]) / 3;
      pois[1] = (poi1[1] + poi2[1] + poi3[1]) / 3;
      pois[2] = (poi1[2] + poi2[2] + poi3[2]) / 3;

      if (count == 0 && mode == 'P') {
        ::left[0] = pois[0];
        ::left[1] = pois[1];
        ::left[2] = pois[2];
        cout << "the left center: " << ::left[0] << ", " << ::left[1] << ", " << ::left[2] << endl << endl;
        idl = picker->GetCellId();
      }
      else if (count == 1 && mode == 'P') {
        ::right[0] = pois[0];
        ::right[1] = pois[1];
        ::right[2] = pois[2];
        cout << "the right center: " << ::right[0] << ", " << ::right[1] << ", " << ::right[2] << endl << endl;
        idr = picker->GetCellId();
      }
      else if (count == 0 && mode == 'C') {
        leftn[0] = pois[0];
        leftn[1] = pois[1];
        leftn[2] = pois[2];
        cout << "the left center: " << leftn[0] << ", " << leftn[1] << ", " << leftn[2] << endl << endl;
        picker->GetPickNormal(normal);
        cout << "the normal: " << normal[0] << ", " << normal[1] << ", " << normal[2] << endl;
        enormal[0] = normal[0];
        enormal[1] = normal[1];
        enormal[2] = normal[2];
        idz = picker->GetCellId();
      }
      else if (count == 1 && mode == 'C') {
        rightn[0] = pois[0];
        rightn[1] = pois[1];
        rightn[2] = pois[2];
        cout << "the right center: " << rightn[0] << ", " << rightn[1] << ", " << rightn[2] << endl << endl;
        picker->GetPickNormal(normal);
        cout << "the normal: " << normal[0] << ", " << normal[1] << ", " << normal[2] << endl;
        idy = picker->GetCellId();
      }
      count = (count + 1) % 2;

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
      extractSelection->SetInputData(0, polyData);
      extractSelection->SetInputData(1, selection);
      extractSelection->Update();

      selectedMapper->SetInputData((vtkDataSet*)extractSelection->GetOutput());
      selectedActor->SetMapper(selectedMapper);
      selectedActor->GetProperty()->EdgeVisibilityOn();
      selectedActor->GetProperty()->SetEdgeColor(1, 0, 0);
      selectedActor->GetProperty()->SetLineWidth(3);

      this->Interactor->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->AddActor(selectedActor);
    }
    vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
  }

  vtkSmartPointer<vtkPolyData>      polyData;
  vtkSmartPointer<vtkDataSetMapper> selectedMapper;
  vtkSmartPointer<vtkActor>       selectedActor;
  vtkSmartPointer<vtkRenderer>    renderer;
};


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
    this->after_act = new QAction(tr("afterProcess"), this);
    this->cal_act = new QAction(tr("calculation"), this);

    this->key_m_act = new QAction(tr("chosePointsOnEar"), this); 
    this->key_a_act = new QAction(tr("chosePointsOnData"), this); 
    this->key_v_act = new QAction(tr("doTrans"), this); 
    this->key_plus_act = new QAction(tr("angle"), this);
    this->key_s_act = new QAction(tr("doRot"), this);
    this->pSizeBox = new QComboBox();

    for (int i = -15; i < 16; ++i)
      this->pSizeBox->addItem( QString::number( i ) );


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
    toolBar->addAction(after_act);
    toolBar->addAction(cal_act);
    toolBar->addAction(key_m_act);
    toolBar->addAction(key_a_act);
    toolBar->addAction(key_v_act);
    toolBar->addAction(key_s_act);
    toolBar->addAction(key_plus_act);
    toolBar->addWidget(this->pSizeBox);


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
    connect(this->after_act, SIGNAL(triggered()), this, SLOT(afterProc()));
    connect(this->cal_act, SIGNAL(triggered()), this, SLOT(calculation()));
    connect(this->key_m_act, SIGNAL(triggered()), this, SLOT(key_m()));
    connect(this->key_a_act, SIGNAL(triggered()), this, SLOT(key_a()));
    connect(this->key_v_act, SIGNAL(triggered()), this, SLOT(key_v()));
    connect(this->key_s_act, SIGNAL(triggered()), this, SLOT(key_s()));
    connect(this->key_plus_act, SIGNAL(triggered()), this, SLOT(key_plus()));
    connect( pSizeBox, SIGNAL( activated( QString ) ), this, SLOT( slotSize( QString ) ) );
//    connect(this->scale_down_act, SIGNAL(triggered()), this, SLOT(scaling_down()));
//    connect(this->scale_up_act, SIGNAL(triggered()), this, SLOT(scaling_up()));


}

void MainWindow::slotSize(QString str)
{
   angle = str.toFloat();
   cout << "angle:" << angle << endl;
   angle = 3.1415926 / (180 / angle);
}

void MainWindow::reSet()
{
    if (acContour != NULL) {
        acContour->ClearAllNodes();
    }
    g_vclick_points1.clear();

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

void MainWindow::afterProc()
{
  reSet();
  CUTTING_MODE = false;
  remat->Zero();
  QString filename = QFileDialog :: getOpenFileName(this, NULL, NULL, "*.*");

  if (filename != "") {
      tetgenio in, out;
      QByteArray ba = filename.toLatin1();
      in.load_stl(ba.data());
      vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
      vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();


      tetrahedralize("apq1.1", &in, &out);
      int *mtetrahedron = out.tetrahedronlist;
      double *pointslist = out.pointlist;

      double arr[3];
      for (int i = 0; i < out.numberofpoints; ++i) {
        int idx = i * 3;
        arr[0] = pointslist[idx];
        arr[1] = pointslist[idx + 1];
        arr[2] = pointslist[idx + 2];
        points->InsertPoint(i + 1, arr);
      }

      vtkIdType ids[4];
      // set points
      vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid = 
        vtkSmartPointer<vtkUnstructuredGrid>::New();
      unstructuredGrid->SetPoints(points);
      for (int i = 0; i < out.numberoftetrahedra; ++i) {
        int idx = i * 4; 
        ids[0] = mtetrahedron[idx];
        ids[1] = mtetrahedron[idx + 1];
        ids[2] = mtetrahedron[idx + 2];
        ids[3] = mtetrahedron[idx + 3];
        unstructuredGrid->InsertNextCell(VTK_TETRA, 4, ids); 
      }

      vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
      mapper->SetInputData(unstructuredGrid);

      vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
      actor->SetMapper(mapper);
      actor->GetProperty()->SetColor(0.194,0.562, 0.75);

      leftrenderer->AddActor(actor);
      leftrenderer->ResetCamera();
      midrender->ResetCamera();
      rightrender->ResetCamera();


      this->renderWindowInteractor = renderWindowLeft->GetInteractor();
      this->renderWindowInteractor->ReInitialize();

      //----------------start call back----------------------------------

      MouseInteractorStyle *style = new MouseInteractorStyle();
      style->SetDefaultRenderer(leftrenderer);
      style->Data = unstructuredGrid;
      this->renderWindowInteractor->SetInteractorStyle(style);
      this->renderWindowInteractor->Start();
      reRenderAll();
      //----------------end call back-------------------------------------------------
  }
  else {
      cout << "invalid file." << endl;
      return;
  }
}


void MainWindow::calculation()
{

}


/**
 * 添加鼠标交互，用于画点并自动形成多边形，因此需要判断[轮廓交互器-acContour]是否为空
 * (https://www.vtk.org/doc/nightly/html/classvtkOrientedGlyphContourRepresentation.html)
 */
void MainWindow::mouse()
{

    if (this->acContour) {
        double po[3] = {0,0,0};
        int num;

      num = acContour->GetNumberOfNodes();
      acContour->GetNthNodeWorldPosition(num-1,po);
      if (CUTTING_MODE) addPoint(po, acContour->GetRenderer());
    }
    reRenderAll();
}

void MainWindow::key_m()
{
  if (vimportPolydatas.size() > 0) {
    cout << "select two pieces on the ear" << endl;
    CellPickerInteractorStyle *CPIStyle = new CellPickerInteractorStyle();
    CPIStyle->SetRenderer(leftrenderer);
    CPIStyle->SetData(vimportPolydatas[vimportPolydatas.size() - 1]);
    CPIStyle->SetMode('C');
    this->renderWindowInteractor->SetInteractorStyle(CPIStyle);
  }
  reRenderAll();
}

void MainWindow::key_a()
{
  if (vleftPolydatas.size() > 0) {
    cout << "select two pieces on the object" << endl;
    CellPickerInteractorStyle *CPIStyle = new CellPickerInteractorStyle();
    CPIStyle->SetRenderer(leftrenderer);
    CPIStyle->SetData(vleftPolydatas[vleftPolydatas.size() - 1]);
    CPIStyle->SetMode('P');
    this->renderWindowInteractor->SetInteractorStyle(CPIStyle); 
  }
  reRenderAll();
}


/**
 * 耳朵旋转角度的调整
 */
void MainWindow::key_plus()
{
  cout << "type in the angle you want to adjust the ear (deafault angle is 15): ";
  string in;
  cin >> in;
  angle = stod(in);
  angle = 3.1415926 / (180 / angle);
}

void MainWindow::key_s()
{
  if (vimportPolydatas.size() > 0) {
    assert(vimportPolydatas.size() == vimportActor.size());
    vtkSmartPointer<vtkPolyData> earData = vimportPolydatas[vimportPolydatas.size() - 1];
    vtkSmartPointer<vtkActor> earActor = vimportActor[vimportActor.size() - 1];
    
    double poib1[3], poib2[3], poib3[3];
    vtkSmartPointer<vtkIdList> listb =
      vtkSmartPointer<vtkIdList>::New();
    earData->GetCellPoints(idy, listb);
    earData->GetPoint(listb->GetId(0), poib1);
    earData->GetPoint(listb->GetId(1), poib2);
    earData->GetPoint(listb->GetId(2), poib3);
    u[0] = (poib1[0] + poib2[0] + poib3[0]) / 3;
    u[1] = (poib1[1] + poib2[1] + poib3[1]) / 3;
    u[2] = (poib1[2] + poib2[2] + poib3[2]) / 3;
    double norm = sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
    u[0] = u[0] / norm;
    u[1] = u[1] / norm;
    u[2] = u[2] / norm;

    vtkSmartPointer<vtkTransformPolyDataFilter> transdata =
      vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    vtkSmartPointer<vtkDataSetMapper> sourceMapper =
      vtkSmartPointer<vtkDataSetMapper>::New();
    vtkSmartPointer<vtkMatrix4x4> rmat = vtkSmartPointer<vtkMatrix4x4>::New();
    rmat->SetElement(0, 0, cos(angle) + u[0] * u[0] * (1 - cos(angle)));
    rmat->SetElement(0, 1, u[0] * u[1] * (1 - cos(angle)) - u[2] * sin(angle));
    rmat->SetElement(0, 2, u[1] * sin(angle) + u[0] * u[2] * (1 - cos(angle)));

    rmat->SetElement(1, 0, u[2] * sin(angle) + u[0] * u[1] * (1 - cos(angle)));
    rmat->SetElement(1, 1, cos(angle) + u[1] * u[1] * (1 - cos(angle)));
    rmat->SetElement(1, 2, -u[0] * sin(angle) + u[1] * u[2] * (1 - cos(angle)));

    rmat->SetElement(2, 0, -u[1] * sin(angle) + u[0] * u[2] * (1 - cos(angle)));
    rmat->SetElement(2, 1, u[0] * sin(angle) + u[1] * u[2] * (1 - cos(angle)));
    rmat->SetElement(2, 2, cos(angle) + u[2] * u[2] * (1 - cos(angle)));

    rmat->SetElement(3, 3, 1);

    vtkSmartPointer<vtkTransform> ro =
      vtkSmartPointer<vtkTransform>::New();
    ro->SetMatrix(rmat);
    transdata->SetInputData(earData);
    transdata->SetTransform(ro);
    transdata->Update();
    earData->DeepCopy(transdata->GetOutput());

    leftrenderer->RemoveActor(earActor);
    sourceMapper->SetInputData(earData);
    earActor->SetMapper(sourceMapper);
    leftrenderer->AddActor(earActor);
  }
  reRenderAll();
}

void MainWindow::key_v()
{
  if (vimportPolydatas.size() > 0) {
    assert(vimportPolydatas.size() == vimportActor.size());
    vtkSmartPointer<vtkPolyData> earData = vimportPolydatas[vimportPolydatas.size() - 1];
    vtkSmartPointer<vtkActor> earActor = vimportActor[vimportActor.size() - 1];
    cout << "Complete the transformation" << endl;
    double shrink = 1;
    double p1 = ::left[0] - ::right[0];
    double p2 = ::left[1] - ::right[1];
    double p3 = ::left[2] - ::right[2];
    double c1 = leftn[0] - rightn[0];
    double c2 = leftn[1] - rightn[1];
    double c3 = leftn[2] - rightn[2];

    shrink = sqrt((p1*p1 + p2 * p2 + p3 * p3) / (c1*c1 + c2 * c2 + c3 * c3));
    cout << "the ratio: " << shrink << endl;

    vtkSmartPointer<vtkTransform> trans =
      vtkSmartPointer<vtkTransform>::New();
    vtkSmartPointer<vtkTransformPolyDataFilter> transdata =
      vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    vtkSmartPointer<vtkDataSetMapper> sourceMapper =
      vtkSmartPointer<vtkDataSetMapper>::New();

    //the shrink
    vtkSmartPointer<vtkMatrix4x4> smat = vtkSmartPointer<vtkMatrix4x4>::New();
    //smat->Zero();
    smat->SetElement(0, 0, shrink);
    smat->SetElement(1, 1, shrink);
    smat->SetElement(2, 2, shrink);
    smat->SetElement(3, 3, 1);
    smat->MultiplyPoint(leftn, leftn);
    smat->MultiplyPoint(rightn, rightn);

    trans->SetMatrix(smat);
    transdata->SetInputData(earData);
    transdata->SetTransform(trans);
    transdata->Update();
    earData->DeepCopy(transdata->GetOutput());

    /**///translate both source and ear to the ordinary, then rotate the ear

    vtkSmartPointer<vtkTransform> trss =
      vtkSmartPointer<vtkTransform>::New();
    trss->Translate(-::left[0], -::left[1], -::left[2]);
    transdata->SetInputData(vleftPolydatas[vleftPolydatas.size() - 1]);
    transdata->SetTransform(trss);
    transdata->Update();
    vleftPolydatas[vleftPolydatas.size() - 1]->DeepCopy(transdata->GetOutput());
    trss->MultiplyPoint(::left, ::left);
    trss->MultiplyPoint(::right, ::right);

    vtkSmartPointer<vtkTransform> trs =
      vtkSmartPointer<vtkTransform>::New();
    trs->Translate(-leftn[0], -leftn[1], -leftn[2]);
    transdata->SetInputData(earData);
    transdata->SetTransform(trs);
    transdata->Update();
    earData->DeepCopy(transdata->GetOutput());
    trs->MultiplyPoint(leftn, leftn);
    trs->MultiplyPoint(rightn, rightn);

    double vectorBefore[3];
    double vectorAfter[3];
    double poib1[3], poib2[3], poib3[3];
    double angle;
    double before;
    double after;
    double norm;
    /**/vtkSmartPointer<vtkIdList> listb =
      vtkSmartPointer<vtkIdList>::New();
    earData->GetCellPoints(idy, listb);
    earData->GetPoint(listb->GetId(0), poib1);
    earData->GetPoint(listb->GetId(1), poib2);
    earData->GetPoint(listb->GetId(2), poib3);
    vectorBefore[0] = (poib1[0] + poib2[0] + poib3[0]) / 3;
    cout << vectorBefore[0] << endl;
    vectorBefore[1] = (poib1[1] + poib2[1] + poib3[1]) / 3;
    cout << vectorBefore[1] << endl;
    vectorBefore[2] = (poib1[2] + poib2[2] + poib3[2]) / 3;
    cout << vectorBefore[2] << endl;

    vtkSmartPointer<vtkIdList> lista =
      vtkSmartPointer<vtkIdList>::New();
    vleftPolydatas[vleftPolydatas.size() - 1]->GetCellPoints(idr, lista);
    vleftPolydatas[vleftPolydatas.size() - 1]->GetPoint(lista->GetId(0), poib1);
    vleftPolydatas[vleftPolydatas.size() - 1]->GetPoint(lista->GetId(1), poib2);
    vleftPolydatas[vleftPolydatas.size() - 1]->GetPoint(lista->GetId(2), poib3);
    cout << "the goal vector is: " << endl;
    vectorAfter[0] = (poib1[0] + poib2[0] + poib3[0]) / 3;
    cout << vectorAfter[0] << endl;
    vectorAfter[1] = (poib1[1] + poib2[1] + poib3[1]) / 3;
    cout << vectorAfter[1] << endl;
    vectorAfter[2] = (poib1[2] + poib2[2] + poib3[2]) / 3;
    cout << vectorAfter[2] << endl;

    before = sqrt(vectorBefore[0] * vectorBefore[0] + vectorBefore[1] * vectorBefore[1] + vectorBefore[2] * vectorBefore[2]);
    after = sqrt(vectorAfter[0] * vectorAfter[0] + vectorAfter[1] * vectorAfter[1] + vectorAfter[2] * vectorAfter[2]);
    vtkMath::Cross(vectorBefore, vectorAfter, u);
    angle = acos((vectorBefore[0]* vectorAfter[0]+ vectorBefore[1] * vectorAfter[1]+ vectorBefore[2] * vectorAfter[2]) / (before*after));

    norm = sqrt(u[0]*u[0]+u[1]*u[1]+u[2]*u[2]);
    u[0] = u[0] / norm;
    u[1] = u[1] / norm;
    u[2] = u[2] / norm;
    vtkSmartPointer<vtkMatrix4x4> rmat = vtkSmartPointer<vtkMatrix4x4>::New();
    rmat->SetElement(0, 0, cos(angle) + u[0] * u[0] * (1 - cos(angle)));
    rmat->SetElement(0, 1, u[0] * u[1] * (1 - cos(angle)) - u[2] * sin(angle));
    rmat->SetElement(0, 2, u[1] * sin(angle) + u[0] * u[2] * (1 - cos(angle)));

    rmat->SetElement(1, 0, u[2] * sin(angle) + u[0] * u[1] * (1 - cos(angle)));
    rmat->SetElement(1, 1, cos(angle) + u[1] * u[1] * (1 - cos(angle)));
    rmat->SetElement(1, 2, -u[0] * sin(angle) + u[1] * u[2] * (1 - cos(angle)));

    rmat->SetElement(2, 0, -u[1] * sin(angle) + u[0] * u[2] * (1 - cos(angle)));
    rmat->SetElement(2, 1, u[0] * sin(angle) + u[1] * u[2] * (1 - cos(angle)));
    rmat->SetElement(2, 2, cos(angle) + u[2] * u[2] * (1 - cos(angle)));

    rmat->SetElement(3, 3, 1);

    vtkSmartPointer<vtkTransform> ro =
      vtkSmartPointer<vtkTransform>::New();
    ro->SetMatrix(rmat);
    transdata->SetInputData(earData);
    transdata->SetTransform(ro);
    transdata->Update();
    earData->DeepCopy(transdata->GetOutput());

    double poi1[3], poi2[3], poi3[3], poisz[3], poisy[3];
    vtkSmartPointer<vtkIdList> list =
      vtkSmartPointer<vtkIdList>::New();
    cout << "the ear vector is:" << endl;
    earData->GetCellPoints(idz, list);
    earData->GetPoint(list->GetId(0), poi1);
    earData->GetPoint(list->GetId(1), poi2);
    earData->GetPoint(list->GetId(2), poi3);
    poisz[0] = (poi1[0] + poi2[0] + poi3[0]) / 3;
    poisz[1] = (poi1[1] + poi2[1] + poi3[1]) / 3;
    poisz[2] = (poi1[2] + poi2[2] + poi3[2]) / 3;
    earData->GetCellPoints(idy, list);
    earData->GetPoint(list->GetId(0), poi1);
    earData->GetPoint(list->GetId(1), poi2);
    earData->GetPoint(list->GetId(2), poi3);
    poisy[0] = (poi1[0] + poi2[0] + poi3[0]) / 3;
    poisy[1] = (poi1[1] + poi2[1] + poi3[1]) / 3;
    poisy[2] = (poi1[2] + poi2[2] + poi3[2]) / 3;
    cout << poisy[0] - poisz[0] << endl;
    cout << poisy[1] - poisz[1] << endl;
    cout << poisy[2] - poisz[2] << endl;

    cout << "the ear vector is:" << endl;
    earData->GetCellPoints(idz, list);
    earData->GetPoint(list->GetId(0), poi1);
    earData->GetPoint(list->GetId(1), poi2);
    earData->GetPoint(list->GetId(2), poi3);
    poisz[0] = (poi1[0] + poi2[0] + poi3[0]) / 3;
    poisz[1] = (poi1[1] + poi2[1] + poi3[1]) / 3;
    poisz[2] = (poi1[2] + poi2[2] + poi3[2]) / 3;
    earData->GetCellPoints(idy, list);
    earData->GetPoint(list->GetId(0), poi1);
    earData->GetPoint(list->GetId(1), poi2);
    earData->GetPoint(list->GetId(2), poi3);
    poisy[0] = (poi1[0] + poi2[0] + poi3[0]) / 3;
    poisy[1] = (poi1[1] + poi2[1] + poi3[1]) / 3;
    poisy[2] = (poi1[2] + poi2[2] + poi3[2]) / 3;
    cout << poisy[0] - poisz[0] << endl;
    cout << poisy[1] - poisz[1] << endl;
    cout << poisy[2] - poisz[2] << endl;

    leftrenderer->RemoveActor(earActor);
    vtkPolyData* polyData = vimportPolydatas[vimportPolydatas.size() - 1];
    vimportPolydatas.pop_back();
    polyData->Delete();
    vtkSmartPointer<vtkPolyData> temp = vtkSmartPointer<vtkPolyData>::New();
    temp->DeepCopy(transdata->GetOutput());
    vimportPolydatas.push_back(temp);
    sourceMapper->SetInputData(temp);
    earActor->SetMapper(sourceMapper);
    leftrenderer->AddActor(earActor);

  }
  reRenderAll();
}


/**
 * 绑定快捷键
 */
void MainWindow::keyboard() 
{

    char getkey = this->renderWindowInteractor->GetKeyCode();
    if (acContour) {

        if (getkey == 'n') {  // 已经画好点，按n进行切割
            this->cut();
        }

        if (getkey == '1') {  // 相机视角，即寻常视角
            this->cameraMode();
        }

        if (getkey == '2') {  // 物体视角，对物体可以进行旋转平移等操作
            this->objectMode();
        }

        if (getkey == ',') {  // 缩小物体（目前只支持单体导入单体操作，即只对一个导入体(import进来的)进行操作）
            this->scaling_down();
        }

        if (getkey == '.') {  // 放大物体
            this->scaling_up();
        }

        if (getkey == 'm') {  
            this->key_m();
        }

        if (getkey == 'a') {
           this->key_a();
        }

        if (getkey == 'v') {
            this->key_v();
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

        //pop, clear all datas to inition
        QString filename = QFileDialog :: getOpenFileName(this, NULL, NULL, "*.*");

        if (filename != "") {
            clipSource = getPolyData(filename);


            assert(clipSource != NULL);

            // PolyData to process
            center = clipSource->GetCenter();
    //        cout << "center: " << center[0] << " " << center[1] << " " << center[2] << endl;
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
        else {
            cout << "invalid file." << endl;
            return;
        }

    }
    if (FIRSTOPEN) {    // 因为可以重复点击“打开文件”，若非第一次打开，则没必要再绑定鼠标键盘事件
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
    reRenderAll();    // 每次需要重新渲染
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
        g_vclick_points1.pop_back();
        if (g_vclick_points1.size() == 0) {
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
    if ( CUTTING_MODE == 1 && g_vclick_points1.size() > 2)
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
    removeRecentActor();
    if (thePlaneActor)
        removeThePlane();

    //next version use mode to choose whether to edit or cut
    QString filename = QFileDialog :: getOpenFileName(this, NULL, NULL, "*.*");

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








