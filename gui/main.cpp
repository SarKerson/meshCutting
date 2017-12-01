/*without the 3 codes below, the project would be wrong!*/

#include "vtkAutoInit.h"
VTK_MODULE_INIT(vtkRenderingOpenGL2); // VTK was built with vtkRenderingOpenGL2
VTK_MODULE_INIT(vtkInteractionStyle);


#include "mainwindow.h"
#include <QApplication>

#include <QSurfaceFormat>

#include "QVTKOpenGLWidget.h"

int main(int argc, char *argv[])
{
    QSurfaceFormat::setDefaultFormat(QVTKOpenGLWidget::defaultFormat());
    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    return a.exec();
}
