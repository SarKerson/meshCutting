#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "all_headers.h"
#include "../tetgen/include/tetgen.h"
#include "../cellPicker/include/cellPicker.h"
using namespace std;

static bool FIRSTOPEN = true;                 // 是否为第一次打开文件，因为可以重复点击“打开文件”，若非第一次打开，则没必要再绑定鼠标键盘事件
static bool CUTTING_MODE = false;             // 当且仅当执行openFile并且成功时，CUTTING_MODE为true，进入切割模式


vtkSmartPointer<vtkPolygonalSurfacePointPlacer> g_points_placer = NULL;  // 用于在物体上进行着点交互，g_points_placer->AddProp(sourceActor);，即在actor上添加着点交互
vtkSmartPointer<vtkPolyData> g_click_source_data = NULL;  // 被切割物体的源数据
std::vector<double*> g_vclick_points;           // 存储用户绘制的点，每个点为(x, y, z)
vtkSmartPointer<vtkPlane> g_front_plane = NULL; // add_paper方法添加的平行于屏幕的平面，用于投影用户绘制的点，以形成柱体
vtkSmartPointer<vtkActor> g_front_plane_actor = NULL;   // g_front_plane对应的actor

/**
 * 不同窗口的renderer
 */
vtkSmartPointer<vtkRenderer> leftrenderer = NULL;
vtkSmartPointer<vtkRenderer> midrender = NULL;
vtkSmartPointer<vtkRenderer> rightrender = NULL;


/**
 * 维护不同renderer的所有actor，以vector形式保存历史actor数据，以便实现back()操作的回滚
 */
std::vector<vtkSmartPointer<vtkActor> > g_vopen_data_actors;
std::vector<vtkSmartPointer<vtkActor> > g_vimport_data_actors;


/**
 * 维护不同actor对应的所有polydata，以vector形式保存，以便实现actors回滚后的cut()等数据操作
 */
std::vector<vtkSmartPointer<vtkPolyData> > g_vopen_data;
std::vector<vtkSmartPointer<vtkPolyData> > g_vimport_data;


vtkSmartPointer<vtkActor> g_mid_actor = NULL;      // 当前右上方对应的actor
vtkSmartPointer<vtkActor> g_right_actor = NULL;    // 当前右下方对应的actor
vtkSmartPointer<vtkActor> g_clipper_actor = NULL;  // 用户绘制的多边形对应的柱体，用于切割g_click_source_data


vtkSmartPointer<vtkPolyData> g_mid_data = NULL;   // 当前右上方actor对应的polydata
vtkSmartPointer<vtkPolyData> g_right_data = NULL; // 当前右下方actor对应的polydata
vtkSmartPointer<vtkTriangleFilter> g_clipper_trifilter = NULL;  // clipper的output即为用户绘制的多边形对应的柱体的polydata, 调用如：g_clipper_trifilter->GetOutput()


vtkSmartPointer<vtkMatrix4x4> remat = vtkSmartPointer<vtkMatrix4x4>::New();


double *g_bounds;         // 用于动态计算柱体的长度，保证柱体能够完全包围需切割部分（若相切或柱体过小，则无法和源数据(切割对象)作交集运算）
double g_distance = 0;    // 计算切割对象的对角线长度，与g_bounds配合


/**
 * 以下几个变量用于平行于屏幕的平面g_front_plane的构建。
 */
double g_normal[3];       // 该平面的法向量
double g_origin[3];       // 该平面上某一点的坐标
double g_scal = 1;         // 平面的缩放倍数，保证平面足够大


//data used to add the prosthesis
double left[4] = { 0.0, 0.0, 0.0, 1.0 };
double right[4] = { 0.0, 0.0, 0.0, 1.0 };
double leftn[4] = { 0.0, 0.0, 0.0, 1.0 };
double rightn[4] = { 0.0, 0.0, 0.0, 1.0 };
double enormal[3] = { 0, 0, 0 };

//the center of the model
double pcenter[3] = { 0,0,0 };

//the ID of the selected cells
int idl=0, idr=0, idz=0, idy=0, idp=0, ide=0;

//the data used to rotate
double u[3];
double angle = -3.1415926/12;


/**
 *@brief 将平面 g_front_plane 移除
 * [need re-render]
 */
void removeThePlane()
{
    if (g_front_plane_actor) {
        leftrenderer->RemoveActor(g_front_plane_actor);
        g_points_placer->RemoveViewProp(g_front_plane_actor);
        g_front_plane_actor = NULL;
    }
}


/**
 * @brief  remove the points placing method on the recent actor
 * [need re-render]
 */
void rmRecentActorPoints()
{
    if (g_vopen_data_actors.size() > 0) {
        vtkSmartPointer<vtkActor> & actor = g_vopen_data_actors.back();  // 获取leftrender中最顶端的actor
        g_points_placer->RemoveViewProp(actor);                          // 删除该actor的points placing操作（包括已经绘制的点）
    }
}


/**
 * @brief generate a visible plane parallel to the curent screen
 * 原理为，获取屏幕向右及向上两个向量的世界坐标，叉乘得到法向量，任何一点作为平面上的点，即可构建一个平行于屏幕的平面
 * @param ren 某窗口的renderer
 * [need re-render]
 */
void parallelplane(vtkRenderer* ren, vtkSmartPointer<vtkActor> & front_plane_actor)
{
  if (front_plane_actor) {    // 若当前平面非空，则删除当前平面
     removeThePlane();
  }
  front_plane_actor = vtkSmartPointer<vtkActor>::New();

  vtkSmartPointer<vtkPlaneSource> plane = vtkSmartPointer<vtkPlaneSource>::New();

  double a_view[3] = { 0.0,0.0,0.0 };
  double b_view[3] = { 0.0,1.0,0.0 }; 
  double c_view[3] = { 1.0,0.0,0.0 };

  // 以下将view坐标转换为世界坐标，view坐标可以理解为屏幕的坐标，右为x正向，上为y正向，里为z正向
  ren->ViewToWorld(a_view[0], a_view[1], a_view[2]);  
  ren->ViewToWorld(b_view[0], b_view[1], b_view[2]);
  ren->ViewToWorld(c_view[0], c_view[1], c_view[2]);

  // 获取方向向右及向上两个向量的世界坐标表示e1, e2
  double e1[3] = { c_view[0] - a_view[0],
    c_view[1] - a_view[1],
    c_view[2] - a_view[2] };
  double e2[3] = { b_view[0] - a_view[0],
    b_view[1] - a_view[1],
    b_view[2] - a_view[2] };

  double normal[3];
  vtkMath::Cross(e2, e1, normal); // 叉乘得到法向量
  vtkMath::Normalize(normal);     // 法向量归一化

  plane->SetNormal(normal);       // 设置平面的法向量
  plane->Update();

  if (g_front_plane != NULL) {
      g_front_plane->SetNormal(normal);
      g_origin[0] = a_view[0];
      g_origin[1] = a_view[1];
      g_origin[2] = a_view[2];
      g_normal[0] = normal[0];
      g_normal[1] = normal[1];
      g_normal[2] = normal[2];
  }

  vtkSmartPointer<vtkDataSetMapper> planeMapper =
    vtkSmartPointer<vtkDataSetMapper>::New();
  planeMapper->SetInputData(plane->GetOutput());
  front_plane_actor = vtkSmartPointer<vtkActor>::New();
  front_plane_actor->SetMapper(planeMapper);
  front_plane_actor->GetProperty()->SetColor(1.0000, 1.0, 1.0);
  front_plane_actor->GetProperty()->SetInterpolationToFlat();
  front_plane_actor->GetProperty()->SetOpacity(0.1);
  front_plane_actor->GetProperty()->SetLighting(1);
  front_plane_actor->SetScale(g_scal * 5);     // 获得足够大的平面
  g_points_placer->AddProp(front_plane_actor);
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

  if (g_front_plane != NULL) {
    g_front_plane->SetNormal(normal);
    g_origin[0] = a_view[0];
    g_origin[1] = a_view[1];
    g_origin[2] = a_view[2];
    g_normal[0] = normal[0];
    g_normal[1] = normal[1];
    g_normal[2] = normal[2];
  }
}


/**
 * @brief helpful function for calculating appropriate scal
 * @param clipActor
 * @return scal
 */
double calscale(vtkActor* clipActor)
{
  double g_bounds[6];
  for (int cc = 0; cc < 6; cc++)
  {
    g_bounds[cc] = clipActor->GetBounds()[cc];
//    cout << g_bounds[cc] << endl;
  }
  double xscale = fabs(g_bounds[1] - g_bounds[0]);
  double yscale = fabs(g_bounds[3] - g_bounds[2]);
  double zscale = fabs(g_bounds[5] - g_bounds[4]);
  double scal = xscale*yscale;
  if (xscale*zscale > scal)
  {
    scal = xscale*zscale;
  }
  else if (yscale*zscale > scal)
  {
    scal = yscale*zscale;
  }
  return scal;
}
double maximumLength(double* _g_bounds)
{
  std::vector<double> vg_bounds;
  vg_bounds.push_back(_g_bounds[1] - _g_bounds[0]);
  vg_bounds.push_back(_g_bounds[3] - _g_bounds[2]);
  vg_bounds.push_back(_g_bounds[5] - _g_bounds[4]);
  std::vector<double>::iterator max = std::max_element(vg_bounds.begin(), vg_bounds.end());
  return *max;
}
/**
 * 计算向量的模长
 * @return 模长
 */
double mod()
{
  double p[3] = { g_bounds[1] - g_bounds[0],
                  g_bounds[3] - g_bounds[2],
                  g_bounds[5] - g_bounds[4] };
  return p[0] * p[0] + p[1] * p[1] + p[2] * p[2];
}



/**
 * @brief 添加点，每次用户点击都会调用该函数
 * @param p
 * @param render
 * [need re-render]
 * [g_front_plane : NULL :when no points
 *          NOT-NULL: when add any point
 *          TURN-NULL: when cutting is done]
 */
void addPoint(double* p, vtkRenderer* render)
{

    if (CUTTING_MODE) {
        if(g_front_plane == NULL) {                                   // 生成平面
          g_front_plane = vtkSmartPointer<vtkPlane>::New();
          parallelplane(render);
        }

        double * point = new double[3];
        g_front_plane->ProjectPoint(p, g_origin, g_normal, point);    // 在p点沿向量 g_normal 投影到平面 g_front_plane 上
        double distance = sqrt(vtkMath::Distance2BetweenPoints(p, point));  // 计算p到投影点的距离
        if (distance > g_distance) {
          g_distance = distance;
        }
        g_vclick_points.push_back(point);

        render->Render();
    }

}


/**
 * 由用户绘制的点生成柱体，用于切割
 * 这里维护两个全局变量: g_clipper_trifilter, g_clipper_actor
 */
void generateClipperData()
{
  if (CUTTING_MODE) {
      vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
      // 遍历用户绘制的点集，用于生成柱体
      for(int i = 0; i < g_vclick_points.size(); ++i) {
        points->InsertNextPoint(g_vclick_points[i][0], g_vclick_points[i][1], g_vclick_points[i][2]);
      }
      vtkSmartPointer<vtkPolygon> polygon = vtkSmartPointer<vtkPolygon>::New();
      polygon->GetPointIds()->SetNumberOfIds(g_vclick_points.size());
      for(int i = 0; i < g_vclick_points.size(); ++i) {
        polygon->GetPointIds()->SetId(i, i);
      }

      vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
      cells->InsertNextCell(polygon);
      vtkSmartPointer<vtkPolyData> triangle = vtkSmartPointer<vtkPolyData>::New();
      triangle->SetPoints(points);
      triangle->SetPolys(cells);

      vtkSmartPointer<vtkLinearExtrusionFilter> extrude = vtkSmartPointer<vtkLinearExtrusionFilter>::New();
      extrude->SetInputData(triangle);
      extrude->SetExtrusionTypeToNormalExtrusion();
      extrude->SetVector(g_normal[0], g_normal[1], g_normal[2]);
      extrude->SetScaleFactor(1.5 * maximumLength(g_bounds) + 2 * g_distance);//1.5 * maximumLength(g_bounds) +

      g_clipper_trifilter = vtkSmartPointer<vtkTriangleFilter>::New();
      g_clipper_trifilter->SetInputConnection(extrude->GetOutputPort());

       vtkSmartPointer<vtkDataSetMapper> mapper =
        vtkSmartPointer<vtkDataSetMapper>::New();
      mapper->SetInputConnection(g_clipper_trifilter->GetOutputPort());

      g_clipper_actor = vtkSmartPointer<vtkActor>::New();
      g_clipper_actor->SetMapper(mapper);
      g_clipper_actor->GetProperty()->SetColor(0.8900, 0.8100, 0.3400);
      g_clipper_actor->GetProperty()->SetOpacity(0.1);
      leftrenderer->AddActor(g_clipper_actor);
  }

}

/**
 * 由生成的柱体，与被切割物体进行布尔运算，生成结果数据
 */
void generateClippedData()
{

    if (CUTTING_MODE) {

        /*
          与运算及亦或运算，得到两个结果 g_mid_data, g_mid_actor及 g_right_data, g_right_actor
          注意：为了方便，mid指的是右上，right指的是右下
        */
        vtkSmartPointer<vtkBooleanOperationPolyDataFilter> booleanOperation =
          vtkSmartPointer<vtkBooleanOperationPolyDataFilter>::New();
        booleanOperation->SetOperationToIntersection();
        booleanOperation->SetInputData( 1, g_vopen_data[g_vopen_data.size() - 1] );
        booleanOperation->SetInputData(0, g_clipper_trifilter->GetOutput()); // set the input data
        vtkSmartPointer<vtkPolyDataMapper> booleanOperationMapper =
        vtkSmartPointer<vtkPolyDataMapper>::New();
        booleanOperationMapper->SetInputConnection( booleanOperation->GetOutputPort() );
        booleanOperationMapper->ScalarVisibilityOff();
        g_mid_actor = vtkSmartPointer<vtkActor>::New();
        g_mid_actor->SetMapper( booleanOperationMapper );
        midrender->AddActor(g_mid_actor);
        g_mid_data = booleanOperation->GetOutput();


        vtkSmartPointer<vtkBooleanOperationPolyDataFilter> booleanOperation1 =
          vtkSmartPointer<vtkBooleanOperationPolyDataFilter>::New();
        booleanOperation1->SetOperationToDifference();
        booleanOperation1->SetInputData( 0, g_vopen_data[g_vopen_data.size() - 1] );
        booleanOperation1->SetInputData(1, g_clipper_trifilter->GetOutput());
        vtkSmartPointer<vtkPolyDataMapper> booleanOperationMapper1 =
        vtkSmartPointer<vtkPolyDataMapper>::New();
        booleanOperationMapper1->SetInputConnection( booleanOperation1->GetOutputPort() );
        booleanOperationMapper1->ScalarVisibilityOff();
        g_right_actor = vtkSmartPointer<vtkActor>::New();
        g_right_actor->SetMapper( booleanOperationMapper1 );
        rightrender->AddActor( g_right_actor );
        g_right_data = booleanOperation1->GetOutput();
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
  g_vclick_points.clear();
  g_front_plane = NULL;
}


/**
 * @brief 返回输入的stl文件的polydata数据
 * @param filename
 * @return polydata数据
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
    this->setupUi(this); // 加载UI
    this->m_Connections = vtkSmartPointer<vtkEventQtSlotConnect>::New();   // 用于绑定鼠标键盘事件

    // 添加菜单
    this->openfile_act = new QAction(tr("&Open"), this);
    this->openfile_act->setShortcut(QKeySequence::Open);
    this->savefile_act = new QAction(tr("&Save"), this);
    this->savefile_act->setShortcut(QKeySequence::Save);
    this->setStatusTip(tr("Open a file."));
    this->fileMenu = menuBar()->addMenu(tr("&File"));
    this->fileMenu->addAction(openfile_act);
    this->fileMenu->addAction(savefile_act);

    this->acContour = NULL;

    // 添加按钮，每个按钮对应一个名字，该名字将显示在工具栏上
    QToolBar * toolBar = addToolBar(tr("toolBar"));
    this->back_act = new QAction(tr("back"), this);
    this->delete_last_act = new QAction(tr("DeleteLastPoint"), this);
    this->clear_all_act = new QAction(tr("clearAllPoints"), this);
    this->select_first_act = new QAction(tr("select1"), this);
    this->select_second_act = new QAction(tr("select2"), this);
    this->cut_act = new QAction(tr("cut"), this);
    this->import_act = new QAction(tr("import"), this);
    this->union_act = new QAction(tr("union"), this);
    this->add_paper_act = new QAction(tr("addPaper"), this);
    this->reset_act = new QAction(tr("reset"), this);
    this->after_process_act = new QAction(tr("afterProcess"), this);
    this->cal_act = new QAction(tr("calculation"), this);
    this->chose_points_ear_act = new QAction(tr("chosePointsOnEar"), this); 
    this->chose_points_data_act = new QAction(tr("chosePointsOnData"), this); 
    this->ear_data_trans_act = new QAction(tr("doTrans"), this); 
    this->ear_rotation_act = new QAction(tr("doRot"), this);

    // 添加下拉菜单
    this->pSizeBox = new QComboBox();   
    for (int i = -15; i < 16; ++i)
      this->pSizeBox->addItem( QString::number( i ) );

    // 添加按钮到工具栏
    toolBar->addAction(reset_act);
    toolBar->addAction(add_paper_act);
    toolBar->addAction(back_act);
    toolBar->addAction(delete_last_act);
    toolBar->addAction(clear_all_act);
    toolBar->addAction(cut_act);
    toolBar->addAction(select_first_act);
    toolBar->addAction(select_second_act);
    toolBar->addAction(import_act);
    toolBar->addAction(union_act);
    toolBar->addAction(chose_points_ear_act);
    toolBar->addAction(chose_points_data_act);
    toolBar->addAction(ear_data_trans_act);
    toolBar->addAction(ear_rotation_act);
    toolBar->addWidget(this->pSizeBox);
    toolBar->addAction(after_process_act);
    toolBar->addAction(cal_act);
    

    /*
    初始化renderwindows
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

    renderWindowLeft->AddRenderer(leftrenderer);
    renderWindowMid->AddRenderer(midrender);
    renderWindowRight->AddRenderer(rightrender);

    renderWindowLeft->Render();
    renderWindowMid->Render();
    renderWindowRight->Render();

    // 添加按钮到函数的映射
    connect(this->actionExit, SIGNAL(triggered()), this, SLOT(slotExit()));
    connect(this->openfile_act, SIGNAL(triggered()), this, SLOT(openFile()));
    connect(this->savefile_act, SIGNAL(triggered()), this, SLOT(saveFile()));
    connect(this->back_act, SIGNAL(triggered()), this, SLOT(back()));
    connect(this->select_first_act, SIGNAL(triggered()), this, SLOT(selectFirst()));
    connect(this->select_second_act, SIGNAL(triggered()), this, SLOT(selectSecond()));
    connect(this->clear_all_act, SIGNAL(triggered()), this, SLOT(clearAllPoints()));
    connect(this->delete_last_act, SIGNAL(triggered()), this, SLOT(deleteLastPoint()));
    connect(this->cut_act, SIGNAL(triggered()), this, SLOT(cut()));
    connect(this->union_act, SIGNAL(triggered()), this, SLOT(makeUnion()));
    connect(this->import_act, SIGNAL(triggered()), this, SLOT(importFile()));
    connect(this->add_paper_act, SIGNAL(triggered()), this, SLOT(addPaper()));
    connect(this->reset_act, SIGNAL(triggered()), this, SLOT(reSet()));
    connect(this->after_process_act, SIGNAL(triggered()), this, SLOT(afterProc()));
    connect(this->cal_act, SIGNAL(triggered()), this, SLOT(calculation()));
    connect(this->chose_points_ear_act, SIGNAL(triggered()), this, SLOT(key_m()));
    connect(this->chose_points_data_act, SIGNAL(triggered()), this, SLOT(key_a()));
    connect(this->ear_data_trans_act, SIGNAL(triggered()), this, SLOT(key_v()));
    connect(this->ear_rotation_act, SIGNAL(triggered()), this, SLOT(key_s()));
    connect( pSizeBox, SIGNAL( activated( QString ) ), this, SLOT( slotSize( QString ) ) );

}


/**
 * 下拉菜单事件改变全局变量angle的值，作为doRot()的输入
 * @param str 角度
 */
void MainWindow::slotSize(QString str)
{
   angle = str.toFloat();
   cout << "angle:" << angle << endl;
   angle = 3.1415926 / (180 / angle);
}


/**
 * 重置界面，清空所有数据，CUTTING_MODE置true，进入切割状态
 */
void MainWindow::reSet()
{
    if (acContour != NULL) {  // 清空所有绘制节点（显示在屏幕上的点）
        acContour->ClearAllNodes();
    }
    g_vclick_points.clear();  // 清空所有绘制节点的源数据

    if (g_vopen_data_actors.size() > 0) {
        leftrenderer->RemoveActor(g_vopen_data_actors.back());
        rmRecentActorPoints();
    }
    if (g_front_plane_actor) {
        removeThePlane();
    }

    leftrenderer->RemoveAllViewProps();
    rightrender->RemoveAllViewProps();
    midrender->RemoveAllViewProps();


    g_vopen_data_actors.clear();
    g_vopen_data.clear();
    g_vimport_data.clear();
    g_vimport_data_actors.clear();

    g_front_plane_actor = NULL;
    CUTTING_MODE = 1;
    g_front_plane = NULL;
    g_click_source_data = NULL;
    g_mid_actor = NULL;
    g_right_actor = NULL;
    this->acContour = NULL;
    g_mid_actor = NULL;
    g_right_actor = NULL;
    g_mid_data = NULL;
    g_right_data = NULL;
    g_clipper_trifilter = NULL;
    remat->Zero();

    reRenderAll();
}


/**
 * 预期将union后的数据，即 g_vopen_data.back()，用tetgen进行四面体化，
 * 并后台调用abaqus进行受力分析，并将结果push_back到 g_vopen_data 并刷新显示
 *
 * 目前该函数的功能是：
 * 打开某一个文件，调用tetgen进行四面体化并显示。
 * 
 * ！不用union运算结果，即 g_vopen_data.back() 的原因是，由于ear的数据（自己用3dsmask绘制的）是非delaunay三角面片，
 * vtk的布尔运算后也无法为delaunay。而tetgen无法对非规则三维数据作很好的四面体化。但是，可以保证的是，对于规则的三围数据
 * 是可以进行正常的四面体化。
 */
void MainWindow::afterProc()
{
  // reSet();
  // CUTTING_MODE = false;
  // remat->Zero();
  // QString filename = QFileDialog :: getOpenFileName(this, NULL, NULL, "*.*");

  // if (filename != "") {
  //     tetgenio in, out;
  //     QByteArray ba = filename.toLatin1();
  //     in.load_stl(ba.data());
  //     vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  //     vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();


  //     tetrahedralize("apq1.1", &in, &out);
  //     int *mtetrahedron = out.tetrahedronlist;
  //     double *pointslist = out.pointlist;

  //     double arr[3];
  //     for (int i = 0; i < out.numberofpoints; ++i) {
  //       int idx = i * 3;
  //       arr[0] = pointslist[idx];
  //       arr[1] = pointslist[idx + 1];
  //       arr[2] = pointslist[idx + 2];
  //       points->InsertPoint(i + 1, arr);
  //     }

  //     vtkIdType ids[4];
  //     // set points
  //     vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid = 
  //       vtkSmartPointer<vtkUnstructuredGrid>::New();
  //     unstructuredGrid->SetPoints(points);
  //     for (int i = 0; i < out.numberoftetrahedra; ++i) {
  //       int idx = i * 4; 
  //       ids[0] = mtetrahedron[idx];
  //       ids[1] = mtetrahedron[idx + 1];
  //       ids[2] = mtetrahedron[idx + 2];
  //       ids[3] = mtetrahedron[idx + 3];
  //       unstructuredGrid->InsertNextCell(VTK_TETRA, 4, ids); 
  //     }

  //     vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
  //     mapper->SetInputData(unstructuredGrid);

  //     vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  //     actor->SetMapper(mapper);
  //     actor->GetProperty()->SetColor(0.194,0.562, 0.75);

  //     leftrenderer->AddActor(actor);
  //     leftrenderer->ResetCamera();
  //     midrender->ResetCamera();
  //     rightrender->ResetCamera();


  //     this->renderWindowInteractor = renderWindowLeft->GetInteractor();
  //     this->renderWindowInteractor->ReInitialize();

  //     //----------------start call back----------------------------------

  //     MouseInteractorStyle *style = new MouseInteractorStyle();
  //     style->SetDefaultRenderer(leftrenderer);
  //     style->Data = unstructuredGrid;
  //     this->renderWindowInteractor->SetInteractorStyle(style);
  //     this->renderWindowInteractor->Start();
  //     reRenderAll();
  //     //----------------end call back-------------------------------------------------
  // }
  // else {
  //     cout << "invalid file." << endl;
  //     return;
  // }
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
  if (g_vimport_data.size() > 0) {
    cout << "select two pieces on the ear" << endl;
    CellPickerInteractorStyle *CPIStyle = new CellPickerInteractorStyle();
    CPIStyle->SetRenderer(leftrenderer);
    CPIStyle->SetData(g_vimport_data[g_vimport_data.size() - 1]);
    CPIStyle->SetMode('C');
    this->renderWindowInteractor->SetInteractorStyle(CPIStyle);
  }
  reRenderAll();
}

void MainWindow::key_a()
{
  if (g_vopen_data.size() > 0) {
    cout << "select two pieces on the object" << endl;
    CellPickerInteractorStyle *CPIStyle = new CellPickerInteractorStyle();
    CPIStyle->SetRenderer(leftrenderer);
    CPIStyle->SetData(g_vopen_data[g_vopen_data.size() - 1]);
    CPIStyle->SetMode('P');
    this->renderWindowInteractor->SetInteractorStyle(CPIStyle); 
  }
  reRenderAll();
}


void MainWindow::key_s()
{
  if (g_vimport_data.size() > 0) {
    assert(g_vimport_data.size() == g_vimport_data_actors.size());
    vtkSmartPointer<vtkPolyData> earData = g_vimport_data[g_vimport_data.size() - 1];
    vtkSmartPointer<vtkActor> earActor = g_vimport_data_actors[g_vimport_data_actors.size() - 1];
    
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
  if (g_vimport_data.size() > 0) {
    assert(g_vimport_data.size() == g_vimport_data_actors.size());
    vtkSmartPointer<vtkPolyData> earData = g_vimport_data[g_vimport_data.size() - 1];
    vtkSmartPointer<vtkActor> earActor = g_vimport_data_actors[g_vimport_data_actors.size() - 1];
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
    transdata->SetInputData(g_vopen_data[g_vopen_data.size() - 1]);
    transdata->SetTransform(trss);
    transdata->Update();
    g_vopen_data[g_vopen_data.size() - 1]->DeepCopy(transdata->GetOutput());
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
    g_vopen_data[g_vopen_data.size() - 1]->GetCellPoints(idr, lista);
    g_vopen_data[g_vopen_data.size() - 1]->GetPoint(lista->GetId(0), poib1);
    g_vopen_data[g_vopen_data.size() - 1]->GetPoint(lista->GetId(1), poib2);
    g_vopen_data[g_vopen_data.size() - 1]->GetPoint(lista->GetId(2), poib3);
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
    vtkPolyData* polyData = g_vimport_data[g_vimport_data.size() - 1];
    g_vimport_data.pop_back();
    polyData->Delete();
    vtkSmartPointer<vtkPolyData> temp = vtkSmartPointer<vtkPolyData>::New();
    temp->DeepCopy(transdata->GetOutput());
    g_vimport_data.push_back(temp);
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


/**
 * 打开文件，执行以下步骤操作：
 * 1. 导入 g_click_source_data
 * 2. 维护 g_bounds及 g_scal 用于恰当大小的平面及柱体的生成
 * 3. 将导入的 g_click_source_data 对应的actor及data添加到 g_vopen_data_actors 及 g_vopen_data 中，以便back()操作回滚历史数据
 * 4. 添加着点回调
 */
void MainWindow::openFile()
{
    if (acContour == NULL) {
        CUTTING_MODE = 1;
        g_front_plane = NULL;
        g_click_source_data = NULL;
        g_mid_actor = NULL;
        g_right_actor = NULL;
        remat->Zero();

        //pop, clear all datas to inition
        QString filename = QFileDialog :: getOpenFileName(this, NULL, NULL, "*.stl");

        if (filename != "") {
            g_click_source_data = getPolyData(filename);

            assert(g_click_source_data != NULL);

            g_bounds = g_click_source_data->GetBounds();

            //create mapper and actor for each polyData
            vtkSmartPointer<vtkDataSetMapper> sourceMapper = vtkSmartPointer<vtkDataSetMapper>::New();
            sourceMapper->SetInputData(g_click_source_data);

            vtkSmartPointer<vtkActor> sourceActor =
            vtkSmartPointer<vtkActor>::New();
            sourceActor->SetMapper(sourceMapper);
            sourceActor->GetProperty()->SetColor(1.0000,0.3882,0.2784);
            sourceActor->GetProperty()->SetInterpolationToFlat();

            g_scal = calscale(sourceActor);


            g_vopen_data_actors.push_back(sourceActor);
            g_vopen_data.push_back(g_click_source_data);


            leftrenderer->AddActor(sourceActor);

            leftrenderer->ResetCamera();
            midrender->ResetCamera();
            rightrender->ResetCamera();

            this->renderWindowInteractor = renderWindowLeft->GetInteractor();
            this->renderWindowInteractor->ReInitialize();

            //----------------添加着点回调操作----------------------------------
            this->acContour = vtkOrientedGlyphContourRepresentation::New();
            vtkSmartPointer<vtkContourWidget> contourWidget = vtkSmartPointer<vtkContourWidget>::New();
            contourWidget->SetInteractor(renderWindowInteractor);
            contourWidget->SetRepresentation(acContour);
            g_points_placer = vtkSmartPointer<vtkPolygonalSurfacePointPlacer>::New();
            g_points_placer->AddProp(sourceActor);
            acContour->SetPointPlacer(g_points_placer);
            vtkSmartPointer<vtkLinearContourLineInterpolator> interpolator = vtkSmartPointer<vtkLinearContourLineInterpolator>::New();
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
  if (!g_vopen_data.empty()) {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Open STL files") , "/home/", tr("STL files (*.stl)"));
    vtkSmartPointer<vtkSTLWriter> stlWriter = vtkSmartPointer<vtkSTLWriter>::New();
    stlWriter->SetFileName(fileName.toLatin1().data());
    stlWriter->SetInputData(g_vopen_data.back());
    stlWriter->Write();
    cout << "saved successfully..." << endl;
  }
}

/**
 * @brief MainWindow::slotExit
 */
void MainWindow::slotExit()
{
  qApp->exit();
}


/**
 * 删除 g_vclick_points 中最后的点，并删除其显示
 */
void MainWindow::deleteLastPoint()
{
    if (acContour)
    {
        acContour->DeleteLastNode();  // 删除最后一个点的显示
        g_vclick_points.pop_back();   // 从数据中删除该点
        if (g_vclick_points.size() == 0) {    // 若点集为空，则将平面去除，因为可能需要重新计算
          cout << "no points left" << endl;
          clearAllNodes(acContour, leftrenderer);
          assert(g_front_plane == NULL);
        }
        cout << "Delete the last node" << endl;
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
    if (g_vopen_data_actors.size() > 1)
    {
        assert(g_vopen_data_actors.size() == g_vopen_data.size());
        leftrenderer->RemoveActor(g_clipper_actor);
        if (g_front_plane_actor)
            removeThePlane();
        if (g_vopen_data_actors.size() != 1) {
          rmRecentActorPoints();

          vtkSmartPointer<vtkActor> actor = g_vopen_data_actors.back();
          leftrenderer->RemoveActor(actor);
          g_vopen_data_actors.pop_back();
          actor->Delete();

          vtkSmartPointer<vtkPolyData> polyData = g_vopen_data.back();
          g_vopen_data.pop_back();
          polyData->Delete();

          leftrenderer->AddActor(g_vopen_data_actors.back());
          clearAllNodes(acContour, leftrenderer);
          g_points_placer->AddProp(g_vopen_data_actors.back());
        }
        assert(g_front_plane == NULL);
    }
    reRenderAll();
}


/**
 * @brief SideBySideRenderWindowsQt::select First window(mid-window)
 */
void MainWindow::selectFirst()
{
    if (g_mid_actor)
    {
        leftrenderer->RemoveActor(g_vopen_data_actors[g_vopen_data_actors.size() - 1]);
        leftrenderer->RemoveActor(g_clipper_actor);
        rmRecentActorPoints();
        if (g_front_plane_actor)
            removeThePlane();
        midrender->RemoveActor(g_mid_actor);
        rightrender->RemoveActor(g_right_actor);
        g_vopen_data_actors.push_back(g_mid_actor);

        g_vopen_data.push_back(g_mid_data);
        clearAllNodes(acContour, leftrenderer);
        leftrenderer->AddActor(g_vopen_data_actors[g_vopen_data_actors.size() - 1]);
        g_points_placer->AddProp(g_vopen_data_actors[g_vopen_data_actors.size() - 1]);
        assert(g_front_plane == NULL);
    }
    reRenderAll();
}


/**
 * @brief SideBySideRenderWindowsQt::select Second
 */
void MainWindow::selectSecond()
{
    if (g_right_actor)
    {
        rmRecentActorPoints();
        leftrenderer->RemoveActor(g_vopen_data_actors[g_vopen_data_actors.size() - 1]);
        leftrenderer->RemoveActor(g_clipper_actor);
        if (g_front_plane_actor)
            removeThePlane();
        midrender->RemoveActor(g_mid_actor);
        rightrender->RemoveActor(g_right_actor);
        g_vopen_data_actors.push_back(g_right_actor);
        g_vopen_data.push_back(g_right_data);
        clearAllNodes(acContour, leftrenderer);
        leftrenderer->AddActor(g_vopen_data_actors[g_vopen_data_actors.size() - 1]);
        g_points_placer->AddProp(g_vopen_data_actors[g_vopen_data_actors.size() - 1]);
        assert(g_front_plane == NULL);
    }
    reRenderAll();
}


void MainWindow::cut()
{
    if ( CUTTING_MODE == 1 && g_vclick_points.size() > 2)
    {
        generateClipperData();
        generateClippedData();  //draw in mid and right

        midrender->SetActiveCamera(leftrenderer->GetActiveCamera());
        rightrender->SetActiveCamera(leftrenderer->GetActiveCamera());
        g_front_plane = NULL;
        if (g_front_plane_actor)
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
    rmRecentActorPoints();
    if (g_front_plane_actor)
        removeThePlane();

    //next version use mode to choose whether to edit or cut
    QString filename = QFileDialog :: getOpenFileName(this, NULL, NULL, "*.*");

    vtkSmartPointer<vtkPolyData> ear = getPolyData(filename);
    assert(ear != NULL);

    g_vimport_data.push_back(ear);

    vtkSmartPointer<vtkDataSetMapper> earMapper = vtkSmartPointer<vtkDataSetMapper>::New();
    earMapper->SetInputData(ear);

    assert(earMapper != NULL);

    vtkSmartPointer<vtkActor> earActor1 = vtkSmartPointer<vtkActor>::New();
    earActor1->SetMapper(earMapper);
    earActor1->GetProperty()->SetColor(1.0000, 0.3882, 0.2784);
    earActor1->GetProperty()->SetInterpolationToFlat();
    g_vimport_data_actors.push_back(earActor1);
    leftrenderer->AddActor(earActor1);

    reRenderAll();

}



void MainWindow::makeUnion()
{
    if (CUTTING_MODE == 0 && g_vimport_data.size() > 0) {
        cout << "processing union" << endl;

        vtkSmartPointer<vtkMatrix4x4> mat =
            vtkSmartPointer<vtkMatrix4x4>::New();

        vtkSmartPointer<vtkActor> & earActor1 = g_vimport_data_actors[g_vimport_data_actors.size() - 1];
        earActor1->GetMatrix(mat);
        vtkSmartPointer<vtkTransform> trans =
            vtkSmartPointer<vtkTransform>::New();
        trans->SetMatrix(mat);
        vtkSmartPointer<vtkPolyData>  ear1 =
            vtkSmartPointer<vtkPolyData>::New();
        ear1 = g_vimport_data[g_vimport_data.size() - 1];
        vtkSmartPointer<vtkTransformPolyDataFilter> transdata =
            vtkSmartPointer<vtkTransformPolyDataFilter>::New();
        transdata->SetInputData(ear1);
        transdata->SetTransform(trans);
        transdata->Update();
        ear1 = transdata->GetOutput();

        vtkSmartPointer<vtkMatrix4x4> mat1 =
            vtkSmartPointer<vtkMatrix4x4>::New();
        vtkSmartPointer<vtkActor> & sActor = g_vopen_data_actors[g_vopen_data_actors.size() - 1];
        sActor->GetMatrix(mat1);

        vtkSmartPointer<vtkTransform> trans1 =
            vtkSmartPointer<vtkTransform>::New();
        trans1->SetMatrix(mat1);
        vtkSmartPointer<vtkPolyData> source =
            vtkSmartPointer<vtkPolyData>::New();
        source = g_vopen_data[g_vopen_data.size() - 1];
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
        g_vopen_data.push_back(booleanOperation->GetOutput());
        g_vopen_data_actors.push_back(booleanOperationActor);
    }
    reRenderAll();
}


void MainWindow::addPaper()
{
    parallelplane(leftrenderer, g_front_plane_actor);
    leftrenderer->AddActor(g_front_plane_actor);
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
    vtkSmartPointer<vtkPolyData>  ear = g_vimport_data[g_vimport_data.size() - 1];

    int flag = 0;
    vtkSmartPointer<vtkMatrix4x4> remat1 = vtkSmartPointer<vtkMatrix4x4>::New();
    vtkSmartPointer<vtkActor> & earActor = g_vimport_data_actors[g_vimport_data_actors.size() - 1];
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

    g_vimport_data.pop_back();
    g_vimport_data.push_back(ear);

    vtkSmartPointer<vtkDataSetMapper> earMapper =
        vtkSmartPointer<vtkDataSetMapper>::New();
    earMapper->SetInputData(ear);

    vtkSmartPointer<vtkActor> earActor1 = vtkSmartPointer<vtkActor>::New();
    earActor1->SetMapper(earMapper);
    earActor1->GetProperty()->SetColor(1.0000, 0.3882, 0.2784);
    earActor1->GetProperty()->SetInterpolationToFlat();
    leftrenderer->RemoveActor(g_vimport_data_actors[g_vimport_data_actors.size() - 1]);   //last actor
    g_vimport_data_actors.pop_back();
    g_vimport_data_actors.push_back(earActor1);
    leftrenderer->AddActor(earActor1);

    reRenderAll();
}

void MainWindow::scaling_up()
{
    vtkSmartPointer<vtkPolyData>  ear = g_vimport_data[g_vimport_data.size() - 1];

    int flag = 0;
    vtkSmartPointer<vtkMatrix4x4> remat1 = vtkSmartPointer<vtkMatrix4x4>::New();
    vtkSmartPointer<vtkActor> & earActor = g_vimport_data_actors[g_vimport_data_actors.size() - 1];
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

    g_vimport_data.pop_back();
    g_vimport_data.push_back(ear);

    vtkSmartPointer<vtkDataSetMapper> earMapper =
        vtkSmartPointer<vtkDataSetMapper>::New();
    earMapper->SetInputData(ear);

    vtkSmartPointer<vtkActor> earActor1 = vtkSmartPointer<vtkActor>::New();
    earActor1->SetMapper(earMapper);
    earActor1->GetProperty()->SetColor(1.0000, 0.3882, 0.2784);
    earActor1->GetProperty()->SetInterpolationToFlat();
    leftrenderer->RemoveActor(g_vimport_data_actors[g_vimport_data_actors.size() - 1]);   //last actor
    g_vimport_data_actors.pop_back();
    g_vimport_data_actors.push_back(earActor1);
    leftrenderer->AddActor(earActor1);

    reRenderAll();

}








