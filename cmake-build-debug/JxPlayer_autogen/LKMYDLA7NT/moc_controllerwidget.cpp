/****************************************************************************
** Meta object code from reading C++ file 'controllerwidget.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../qt/ui/controllerwidget.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'controllerwidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN16ControllerWidgetE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN16ControllerWidgetE = QtMocHelpers::stringData(
    "ControllerWidget",
    "OnImportButtonClicked",
    "",
    "OnPlayButtonClicked",
    "OnSpeedButtonClicked",
    "OnExportButtonClicked",
    "OnVideoFilterFlipVerticalButtonClicked",
    "OnVideoFilterGrayButtonClicked",
    "OnVideoFilterInvertButtonClicked",
    "OnVideoFilterStickerButtonClicked"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN16ControllerWidgetE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   62,    2, 0x08,    1 /* Private */,
       3,    0,   63,    2, 0x08,    2 /* Private */,
       4,    0,   64,    2, 0x08,    3 /* Private */,
       5,    0,   65,    2, 0x08,    4 /* Private */,
       6,    0,   66,    2, 0x08,    5 /* Private */,
       7,    0,   67,    2, 0x08,    6 /* Private */,
       8,    0,   68,    2, 0x08,    7 /* Private */,
       9,    0,   69,    2, 0x08,    8 /* Private */,

 // slots: parameters
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

Q_CONSTINIT const QMetaObject ControllerWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_ZN16ControllerWidgetE.offsetsAndSizes,
    qt_meta_data_ZN16ControllerWidgetE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN16ControllerWidgetE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<ControllerWidget, std::true_type>,
        // method 'OnImportButtonClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'OnPlayButtonClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'OnSpeedButtonClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'OnExportButtonClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'OnVideoFilterFlipVerticalButtonClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'OnVideoFilterGrayButtonClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'OnVideoFilterInvertButtonClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'OnVideoFilterStickerButtonClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void ControllerWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ControllerWidget *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->OnImportButtonClicked(); break;
        case 1: _t->OnPlayButtonClicked(); break;
        case 2: _t->OnSpeedButtonClicked(); break;
        case 3: _t->OnExportButtonClicked(); break;
        case 4: _t->OnVideoFilterFlipVerticalButtonClicked(); break;
        case 5: _t->OnVideoFilterGrayButtonClicked(); break;
        case 6: _t->OnVideoFilterInvertButtonClicked(); break;
        case 7: _t->OnVideoFilterStickerButtonClicked(); break;
        default: ;
        }
    }
    (void)_a;
}

const QMetaObject *ControllerWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ControllerWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN16ControllerWidgetE.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int ControllerWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 8;
    }
    return _id;
}
QT_WARNING_POP
