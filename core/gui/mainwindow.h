#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <vtkSmartPointer.h>
#include <vtkEventQtSlotConnect.h>
#include <QMainWindow>
#include <QMenu>
#include <QAction>
#include <QComboBox>
#include "ui_mainwindow.h"

class vtkOrientedGlyphContourRepresentation;
class KeyboardEvent;
class PointsPut;

class MainWindow : public QMainWindow, private Ui::mainwindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow() {}
public:

  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor;   // 获取interactor, 见.cpp中，this->renderWindowInteractor = renderWindowLeft->GetInteractor();，获取左render窗口的交互器
  vtkSmartPointer<vtkOrientedGlyphContourRepresentation> acContour;    // 用于轮廓交互（绘制点并形成多边形），初始化为NULL，在openFile()成功的时候赋值
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindowLeft;      // 左边的render窗口
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindowMid;       // 右上角render窗口
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindowRight;     // 右下角render窗口
public:
  void reRenderAll();

public slots:

  virtual void slotExit();
  void keyboard();
  void mouse();
  void openFile();
  void saveFile();

  void deleteLastPoint();
  void clearAllPoints();
  void back();
  void selectFirst();
  void selectSecond();
  void cut();

  void cameraMode();
  void objectMode();
  void makeUnion();
  void importFile();
  void addPaper();

  void scaling_up();
  void scaling_down();

  void reSet();
  void afterProc();
  void calculation();

  void key_m();
  void key_a();
  void key_v();

  void key_s();

  void slotSize(QString str);


private:

  vtkSmartPointer<vtkEventQtSlotConnect> m_Connections;

  QMenu * fileMenu;

  QAction * openfile_act;
  QAction * savefile_act;
  QAction * delete_last_act;            //delete last point
  QAction * clear_all_act;             //clear all points
  QAction * back_act;              //back
  QAction * select_first_act;              //select the first module
  QAction * select_second_act;              //select the second mudule
  QAction * union_act;              //union
  QAction * import_act;             //import file
  QAction * cut_act;
  QAction * add_paper_act;
  QAction * reset_act;
  QAction * after_process_act;
  QAction * cal_act;
  QAction * chose_points_ear_act;  // choose points on the ear
  QAction * chose_points_data_act;  // choose points on the clipper
  QAction * ear_data_trans_act;  // action
  QAction * ear_rotation_act;
  QComboBox * pSizeBox;

};

#endif // MAINWINDOW_H
