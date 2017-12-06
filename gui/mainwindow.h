#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <vtkSmartPointer.h>
#include <vtkEventQtSlotConnect.h>
#include <QMainWindow>
#include <QMenu>
#include <QAction>
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
  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor;
  vtkSmartPointer<vtkOrientedGlyphContourRepresentation> acContour;
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindowLeft;
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindowMid;
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindowRight;
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


private:

  vtkSmartPointer<vtkEventQtSlotConnect> m_Connections;

  QMenu * fileMenu;

  QAction * openfile_act;
  QAction * savefile_act;
  QAction * key_k_act;            //delete last point
  QAction * key_c_act;             //clear all points
  QAction * key_0_act;              //back
  QAction * key_1_act;              //select the first module
  QAction * key_2_act;              //select the second mudule

  QAction * key_a_act;              //camera mode   : 1
  QAction * key_s_act;              //object mode   : 2
  QAction * key_u_act;              //union

  QAction * import_act;             //import file

  QAction * key_n_act;

  QAction * addpaper_act;//

//  QAction * scale_up_act;
//  QAction * scale_down_act;


};

#endif // MAINWINDOW_H
