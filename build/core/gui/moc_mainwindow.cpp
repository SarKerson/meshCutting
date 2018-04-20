/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.6.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../core/gui/mainwindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.6.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_MainWindow_t {
    QByteArrayData data[23];
    char stringdata0[223];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MainWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
QT_MOC_LITERAL(0, 0, 10), // "MainWindow"
QT_MOC_LITERAL(1, 11, 8), // "slotExit"
QT_MOC_LITERAL(2, 20, 0), // ""
QT_MOC_LITERAL(3, 21, 8), // "keyboard"
QT_MOC_LITERAL(4, 30, 5), // "mouse"
QT_MOC_LITERAL(5, 36, 8), // "openFile"
QT_MOC_LITERAL(6, 45, 8), // "saveFile"
QT_MOC_LITERAL(7, 54, 15), // "deleteLastPoint"
QT_MOC_LITERAL(8, 70, 14), // "clearAllPoints"
QT_MOC_LITERAL(9, 85, 4), // "back"
QT_MOC_LITERAL(10, 90, 11), // "selectFirst"
QT_MOC_LITERAL(11, 102, 12), // "selectSecond"
QT_MOC_LITERAL(12, 115, 3), // "cut"
QT_MOC_LITERAL(13, 119, 10), // "cameraMode"
QT_MOC_LITERAL(14, 130, 10), // "objectMode"
QT_MOC_LITERAL(15, 141, 9), // "makeUnion"
QT_MOC_LITERAL(16, 151, 10), // "importFile"
QT_MOC_LITERAL(17, 162, 8), // "addPaper"
QT_MOC_LITERAL(18, 171, 10), // "scaling_up"
QT_MOC_LITERAL(19, 182, 12), // "scaling_down"
QT_MOC_LITERAL(20, 195, 5), // "reSet"
QT_MOC_LITERAL(21, 201, 9), // "afterProc"
QT_MOC_LITERAL(22, 211, 11) // "calculation"

    },
    "MainWindow\0slotExit\0\0keyboard\0mouse\0"
    "openFile\0saveFile\0deleteLastPoint\0"
    "clearAllPoints\0back\0selectFirst\0"
    "selectSecond\0cut\0cameraMode\0objectMode\0"
    "makeUnion\0importFile\0addPaper\0scaling_up\0"
    "scaling_down\0reSet\0afterProc\0calculation"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MainWindow[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
      21,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,  119,    2, 0x0a /* Public */,
       3,    0,  120,    2, 0x0a /* Public */,
       4,    0,  121,    2, 0x0a /* Public */,
       5,    0,  122,    2, 0x0a /* Public */,
       6,    0,  123,    2, 0x0a /* Public */,
       7,    0,  124,    2, 0x0a /* Public */,
       8,    0,  125,    2, 0x0a /* Public */,
       9,    0,  126,    2, 0x0a /* Public */,
      10,    0,  127,    2, 0x0a /* Public */,
      11,    0,  128,    2, 0x0a /* Public */,
      12,    0,  129,    2, 0x0a /* Public */,
      13,    0,  130,    2, 0x0a /* Public */,
      14,    0,  131,    2, 0x0a /* Public */,
      15,    0,  132,    2, 0x0a /* Public */,
      16,    0,  133,    2, 0x0a /* Public */,
      17,    0,  134,    2, 0x0a /* Public */,
      18,    0,  135,    2, 0x0a /* Public */,
      19,    0,  136,    2, 0x0a /* Public */,
      20,    0,  137,    2, 0x0a /* Public */,
      21,    0,  138,    2, 0x0a /* Public */,
      22,    0,  139,    2, 0x0a /* Public */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        MainWindow *_t = static_cast<MainWindow *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->slotExit(); break;
        case 1: _t->keyboard(); break;
        case 2: _t->mouse(); break;
        case 3: _t->openFile(); break;
        case 4: _t->saveFile(); break;
        case 5: _t->deleteLastPoint(); break;
        case 6: _t->clearAllPoints(); break;
        case 7: _t->back(); break;
        case 8: _t->selectFirst(); break;
        case 9: _t->selectSecond(); break;
        case 10: _t->cut(); break;
        case 11: _t->cameraMode(); break;
        case 12: _t->objectMode(); break;
        case 13: _t->makeUnion(); break;
        case 14: _t->importFile(); break;
        case 15: _t->addPaper(); break;
        case 16: _t->scaling_up(); break;
        case 17: _t->scaling_down(); break;
        case 18: _t->reSet(); break;
        case 19: _t->afterProc(); break;
        case 20: _t->calculation(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObject MainWindow::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_MainWindow.data,
      qt_meta_data_MainWindow,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
        return static_cast<void*>(const_cast< MainWindow*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 21)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 21;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 21)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 21;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
