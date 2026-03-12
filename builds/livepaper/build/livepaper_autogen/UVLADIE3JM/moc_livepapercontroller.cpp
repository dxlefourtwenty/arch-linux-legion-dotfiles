/****************************************************************************
** Meta object code from reading C++ file 'livepapercontroller.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/livepapercontroller.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'livepapercontroller.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.2. It"
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
struct qt_meta_tag_ZN19LivepaperControllerE_t {};
} // unnamed namespace

template <> constexpr inline auto LivepaperController::qt_create_metaobjectdata<qt_meta_tag_ZN19LivepaperControllerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "LivepaperController",
        "currentIndexChanged",
        "",
        "countChanged",
        "themeChanged",
        "closeRequested",
        "galleryIndexChanged",
        "applySelected",
        "tickGalleryScroll",
        "setCurrentIndex",
        "index",
        "moveLeft",
        "moveRight",
        "exitApp",
        "requestClose",
        "pathAt",
        "thumbPathAt",
        "labelAt",
        "model",
        "count",
        "currentIndex",
        "currentLabel",
        "galleryIndex",
        "bg",
        "QColor",
        "idle",
        "hover",
        "border",
        "overlay",
        "text",
        "borderRadius"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'currentIndexChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'countChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'themeChanged'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'closeRequested'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'galleryIndexChanged'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'applySelected'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'tickGalleryScroll'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Method 'setCurrentIndex'
        QtMocHelpers::MethodData<void(int)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 10 },
        }}),
        // Method 'moveLeft'
        QtMocHelpers::MethodData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'moveRight'
        QtMocHelpers::MethodData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'exitApp'
        QtMocHelpers::MethodData<void()>(13, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'requestClose'
        QtMocHelpers::MethodData<void()>(14, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'pathAt'
        QtMocHelpers::MethodData<QString(int) const>(15, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::Int, 10 },
        }}),
        // Method 'thumbPathAt'
        QtMocHelpers::MethodData<QString(int) const>(16, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::Int, 10 },
        }}),
        // Method 'labelAt'
        QtMocHelpers::MethodData<QString(int) const>(17, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::Int, 10 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'model'
        QtMocHelpers::PropertyData<QObject*>(18, QMetaType::QObjectStar, QMC::DefaultPropertyFlags | QMC::Constant),
        // property 'count'
        QtMocHelpers::PropertyData<int>(19, QMetaType::Int, QMC::DefaultPropertyFlags, 1),
        // property 'currentIndex'
        QtMocHelpers::PropertyData<int>(20, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'currentLabel'
        QtMocHelpers::PropertyData<QString>(21, QMetaType::QString, QMC::DefaultPropertyFlags, 0),
        // property 'galleryIndex'
        QtMocHelpers::PropertyData<qreal>(22, QMetaType::QReal, QMC::DefaultPropertyFlags, 4),
        // property 'bg'
        QtMocHelpers::PropertyData<QColor>(23, 0x80000000 | 24, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 2),
        // property 'idle'
        QtMocHelpers::PropertyData<QColor>(25, 0x80000000 | 24, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 2),
        // property 'hover'
        QtMocHelpers::PropertyData<QColor>(26, 0x80000000 | 24, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 2),
        // property 'border'
        QtMocHelpers::PropertyData<QColor>(27, 0x80000000 | 24, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 2),
        // property 'overlay'
        QtMocHelpers::PropertyData<QColor>(28, 0x80000000 | 24, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 2),
        // property 'text'
        QtMocHelpers::PropertyData<QColor>(29, 0x80000000 | 24, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 2),
        // property 'borderRadius'
        QtMocHelpers::PropertyData<int>(30, QMetaType::Int, QMC::DefaultPropertyFlags, 2),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<LivepaperController, qt_meta_tag_ZN19LivepaperControllerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject LivepaperController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN19LivepaperControllerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN19LivepaperControllerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN19LivepaperControllerE_t>.metaTypes,
    nullptr
} };

void LivepaperController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<LivepaperController *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->currentIndexChanged(); break;
        case 1: _t->countChanged(); break;
        case 2: _t->themeChanged(); break;
        case 3: _t->closeRequested(); break;
        case 4: _t->galleryIndexChanged(); break;
        case 5: _t->applySelected(); break;
        case 6: _t->tickGalleryScroll(); break;
        case 7: _t->setCurrentIndex((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 8: _t->moveLeft(); break;
        case 9: _t->moveRight(); break;
        case 10: _t->exitApp(); break;
        case 11: _t->requestClose(); break;
        case 12: { QString _r = _t->pathAt((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 13: { QString _r = _t->thumbPathAt((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 14: { QString _r = _t->labelAt((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (LivepaperController::*)()>(_a, &LivepaperController::currentIndexChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (LivepaperController::*)()>(_a, &LivepaperController::countChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (LivepaperController::*)()>(_a, &LivepaperController::themeChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (LivepaperController::*)()>(_a, &LivepaperController::closeRequested, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (LivepaperController::*)()>(_a, &LivepaperController::galleryIndexChanged, 4))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<QObject**>(_v) = _t->model(); break;
        case 1: *reinterpret_cast<int*>(_v) = _t->count(); break;
        case 2: *reinterpret_cast<int*>(_v) = _t->currentIndex(); break;
        case 3: *reinterpret_cast<QString*>(_v) = _t->currentLabel(); break;
        case 4: *reinterpret_cast<qreal*>(_v) = _t->galleryIndex(); break;
        case 5: *reinterpret_cast<QColor*>(_v) = _t->bg(); break;
        case 6: *reinterpret_cast<QColor*>(_v) = _t->idle(); break;
        case 7: *reinterpret_cast<QColor*>(_v) = _t->hover(); break;
        case 8: *reinterpret_cast<QColor*>(_v) = _t->border(); break;
        case 9: *reinterpret_cast<QColor*>(_v) = _t->overlay(); break;
        case 10: *reinterpret_cast<QColor*>(_v) = _t->text(); break;
        case 11: *reinterpret_cast<int*>(_v) = _t->borderRadius(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 2: _t->setCurrentIndex(*reinterpret_cast<int*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *LivepaperController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *LivepaperController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN19LivepaperControllerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int LivepaperController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 15;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    }
    return _id;
}

// SIGNAL 0
void LivepaperController::currentIndexChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void LivepaperController::countChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void LivepaperController::themeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void LivepaperController::closeRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void LivepaperController::galleryIndexChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}
QT_WARNING_POP
