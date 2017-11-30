#ifndef SideBySideRenderWindowsQt_H
#define SideBySideRenderWindowsQt_H

#include <vtkSmartPointer.h>
#include <vtkEventQtSlotConnect.h>

#include <QMainWindow>
#include <QMenu>
#include <QAction>

#include "ui_SideBySideRenderWindowsQt.h"

//class QAction;
//class QMenu;
class vtkOrientedGlyphContourRepresentation;
class KeyboardEvent;
class PointsPut;

class SideBySideRenderWindowsQt : public QMainWindow, private Ui::SideBySideRenderWindowsQt
{
  Q_OBJECT
public:

//protected:
//#ifndef QT_NO_CONTEXTMENU
//    void contextMenuEvent(QContextMenuEvent *event) override;
//#endif // QT_NO_CONTEXTMENU


  // Constructor/Destructor
  SideBySideRenderWindowsQt(); 
  ~SideBySideRenderWindowsQt() {};

public:
  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor;
  vtkSmartPointer<vtkOrientedGlyphContourRepresentation> acContour;
//  vtkSmartPointer<PointsPut> poput;
//  vtkSmartPointer<KeyboardEvent> key;
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindowLeft;
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindowMid;
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindowRight;



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

  QAction * key_a_act;              //camera mode
  QAction * key_s_act;              //object mode
  QAction * key_u_act;              //union

  QAction * import_act;             //import file

  QAction * key_n_act;

};

#endif
