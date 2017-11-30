/*without the 3 codes below, the project would be wrong!*/
#include "vtkAutoInit.h"
VTK_MODULE_INIT(vtkRenderingOpenGL2); // VTK was built with vtkRenderingOpenGL2
VTK_MODULE_INIT(vtkInteractionStyle);


/*start project*/

#include <QApplication>
#include <QSurfaceFormat>

#include "QVTKOpenGLWidget.h"
#include "SideBySideRenderWindowsQt.h"

int main( int argc, char** argv )
{
  // needed to ensure appropriate OpenGL context is created for VTK rendering.
  QSurfaceFormat::setDefaultFormat(QVTKOpenGLWidget::defaultFormat());

  // QT Stuff
  QApplication app( argc, argv );
  
  SideBySideRenderWindowsQt sideBySideRenderWindowsQt;
  sideBySideRenderWindowsQt.show();

//  sideBySideRenderWindowsQt.renderWindowInteractor->Initialize();
//  sideBySideRenderWindowsQt.renderWindowInteractor->Start();

  
  return app.exec();
}
