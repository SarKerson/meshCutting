/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.6.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QWidget>
#include "QVTKOpenGLWidget.h"

QT_BEGIN_NAMESPACE

class Ui_mainwindow
{
public:
    QAction *actionOpenFile;
    QAction *actionExit;
    QAction *actionPrint;
    QAction *actionHelp;
    QAction *actionSave;
    QWidget *centralwidget;
    QGridLayout *gridLayout;
    QVTKOpenGLWidget *qvtkWidgetLeft;
    QVTKOpenGLWidget *qvtkWidgetMid;
    QVTKOpenGLWidget *qvtkWidgetRight;

    void setupUi(QMainWindow *mainwindow)
    {
        if (mainwindow->objectName().isEmpty())
            mainwindow->setObjectName(QStringLiteral("mainwindow"));
        mainwindow->resize(580, 543);
        QIcon icon;
        icon.addFile(QStringLiteral(":/Icons/help.png"), QSize(), QIcon::Normal, QIcon::Off);
        mainwindow->setWindowIcon(icon);
        mainwindow->setIconSize(QSize(22, 22));
        actionOpenFile = new QAction(mainwindow);
        actionOpenFile->setObjectName(QStringLiteral("actionOpenFile"));
        actionOpenFile->setEnabled(true);
        QIcon icon1;
        icon1.addFile(QStringLiteral(":/Icons/fileopen.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionOpenFile->setIcon(icon1);
        actionExit = new QAction(mainwindow);
        actionExit->setObjectName(QStringLiteral("actionExit"));
        QIcon icon2;
        icon2.addFile(QStringLiteral("."), QSize(), QIcon::Normal, QIcon::Off);
        actionExit->setIcon(icon2);
        actionPrint = new QAction(mainwindow);
        actionPrint->setObjectName(QStringLiteral("actionPrint"));
        QIcon icon3;
        icon3.addFile(QStringLiteral(":/Icons/print.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionPrint->setIcon(icon3);
        actionHelp = new QAction(mainwindow);
        actionHelp->setObjectName(QStringLiteral("actionHelp"));
        actionHelp->setIcon(icon);
        actionSave = new QAction(mainwindow);
        actionSave->setObjectName(QStringLiteral("actionSave"));
        QIcon icon4;
        icon4.addFile(QStringLiteral(":/Icons/filesave.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionSave->setIcon(icon4);
        centralwidget = new QWidget(mainwindow);
        centralwidget->setObjectName(QStringLiteral("centralwidget"));
        gridLayout = new QGridLayout(centralwidget);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        qvtkWidgetLeft = new QVTKOpenGLWidget(centralwidget);
        qvtkWidgetLeft->setObjectName(QStringLiteral("qvtkWidgetLeft"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(qvtkWidgetLeft->sizePolicy().hasHeightForWidth());
        qvtkWidgetLeft->setSizePolicy(sizePolicy);

        gridLayout->addWidget(qvtkWidgetLeft, 0, 0, 2, 1);

        qvtkWidgetMid = new QVTKOpenGLWidget(centralwidget);
        qvtkWidgetMid->setObjectName(QStringLiteral("qvtkWidgetMid"));
        sizePolicy.setHeightForWidth(qvtkWidgetMid->sizePolicy().hasHeightForWidth());
        qvtkWidgetMid->setSizePolicy(sizePolicy);

        gridLayout->addWidget(qvtkWidgetMid, 0, 1, 1, 1);

        qvtkWidgetRight = new QVTKOpenGLWidget(centralwidget);
        qvtkWidgetRight->setObjectName(QStringLiteral("qvtkWidgetRight"));
        sizePolicy.setHeightForWidth(qvtkWidgetRight->sizePolicy().hasHeightForWidth());
        qvtkWidgetRight->setSizePolicy(sizePolicy);

        gridLayout->addWidget(qvtkWidgetRight, 1, 1, 1, 1);

        mainwindow->setCentralWidget(centralwidget);

        retranslateUi(mainwindow);

        QMetaObject::connectSlotsByName(mainwindow);
    } // setupUi

    void retranslateUi(QMainWindow *mainwindow)
    {
        mainwindow->setWindowTitle(QApplication::translate("mainwindow", "SimpleView", 0));
        actionOpenFile->setText(QApplication::translate("mainwindow", "Open File...", 0));
        actionExit->setText(QApplication::translate("mainwindow", "Exit", 0));
        actionPrint->setText(QApplication::translate("mainwindow", "Print", 0));
        actionHelp->setText(QApplication::translate("mainwindow", "Help", 0));
        actionSave->setText(QApplication::translate("mainwindow", "Save", 0));
    } // retranslateUi

};

namespace Ui {
    class mainwindow: public Ui_mainwindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
