#ifndef TDISPATCHER_H
#define TDISPATCHER_H

#include <QMetaType>
#include <QMetaMethod>
#include <QMetaObject>
#include <QStringList>
#include <TGlobal>
#include "tsystemglobal.h"


template <class T>
class TDispatcher
{
public:
    TDispatcher(const QString &metaTypeName);
    ~TDispatcher();

    bool invoke(const QString &method, const QStringList &args = QStringList());
    T *object();
    QString typeName() const { return metaType; }

private:
    Q_DISABLE_COPY(TDispatcher)

    QString metaType;
    int typeId;
    T *ptr;
};


template <class T>
inline TDispatcher<T>::TDispatcher(const QString &metaTypeName)
    : metaType(metaTypeName),
      typeId(0),
      ptr(0)
{ }

template <class T>
inline TDispatcher<T>::~TDispatcher()
{
    if (ptr) {
        QMetaType::destroy(typeId, ptr);
    }
}

template <class T>
inline bool TDispatcher<T>::invoke(const QString &method, const QStringList &args)
{
    T_TRACEFUNC("");

    object();
    if (!ptr) {
        tSystemDebug("Failed to invoke, no such class: %s", qPrintable(metaType));
        return false;
    }

    int argcnt = 0;
    int idx = -1;
    for (int i = args.count(); i >= 0; --i) {
        // Find method
        QByteArray mtd = method.toLatin1() + '(';
        for (int j = 0; j < i; ++j) {
            if (j > 0) mtd += ',';
            mtd += "QString";
        }
        mtd += ')';
        mtd = QMetaObject::normalizedSignature(mtd);
        idx = ptr->metaObject()->indexOfSlot(mtd.constData());
        if (idx >= 0) {
            argcnt = i;
            tSystemDebug("Found method: %s", mtd.constData());
            break;
        }
    }

    bool res = false;
    if (idx < 0) {
        tSystemDebug("No such method: %s", qPrintable(method));
        return false;
    } else {
        QMetaMethod mm = ptr->metaObject()->method(idx);    
        tSystemDebug("Invoke method: %s", qPrintable(metaType + "#" + method));
        switch (argcnt) {
        case 0:
            res = mm.invoke(ptr, Qt::AutoConnection);
            break;
        case 1:
            res = mm.invoke(ptr, Qt::AutoConnection, Q_ARG(QString, args[0]));
            break;
        case 2:
            res = mm.invoke(ptr, Qt::AutoConnection, Q_ARG(QString, args[0]), Q_ARG(QString, args[1]));
            break;
        case 3:
            res = mm.invoke(ptr, Qt::AutoConnection,
                            Q_ARG(QString, args[0]), Q_ARG(QString, args[1]), Q_ARG(QString, args[2]));
            break;
        case 4:
            res = mm.invoke(ptr, Qt::AutoConnection,
                            Q_ARG(QString, args[0]), Q_ARG(QString, args[1]), Q_ARG(QString, args[2]),
                            Q_ARG(QString, args[3]));
            break;
        case 5:
            res = mm.invoke(ptr, Qt::AutoConnection,
                            Q_ARG(QString, args[0]), Q_ARG(QString, args[1]), Q_ARG(QString, args[2]),
                            Q_ARG(QString, args[3]), Q_ARG(QString, args[4]));
            break;
        case 6:
            res = mm.invoke(ptr, Qt::AutoConnection,
                            Q_ARG(QString, args[0]), Q_ARG(QString, args[1]), Q_ARG(QString, args[2]),
                            Q_ARG(QString, args[3]), Q_ARG(QString, args[4]), Q_ARG(QString, args[5]));
            break;
        case 7:
            res = mm.invoke(ptr, Qt::AutoConnection,
                            Q_ARG(QString, args[0]), Q_ARG(QString, args[1]), Q_ARG(QString, args[2]),
                            Q_ARG(QString, args[3]), Q_ARG(QString, args[4]), Q_ARG(QString, args[5]),
                            Q_ARG(QString, args[6]));
            break;
        case 8:
            res = mm.invoke(ptr, Qt::AutoConnection,
                            Q_ARG(QString, args[0]), Q_ARG(QString, args[1]), Q_ARG(QString, args[2]),
                            Q_ARG(QString, args[3]), Q_ARG(QString, args[4]), Q_ARG(QString, args[5]),
                            Q_ARG(QString, args[6]), Q_ARG(QString, args[7]));
            break;
        case 9:
            res = mm.invoke(ptr, Qt::AutoConnection,
                            Q_ARG(QString, args[0]), Q_ARG(QString, args[1]), Q_ARG(QString, args[2]),
                            Q_ARG(QString, args[3]), Q_ARG(QString, args[4]), Q_ARG(QString, args[5]),
                            Q_ARG(QString, args[6]), Q_ARG(QString, args[7]), Q_ARG(QString, args[8]));
            break;
        default:
            res = mm.invoke(ptr, Qt::AutoConnection,
                            Q_ARG(QString, args[0]), Q_ARG(QString, args[1]), Q_ARG(QString, args[2]),
                            Q_ARG(QString, args[3]), Q_ARG(QString, args[4]), Q_ARG(QString, args[5]),
                            Q_ARG(QString, args[6]), Q_ARG(QString, args[7]), Q_ARG(QString, args[8]),
                            Q_ARG(QString, args[9]));
            break;
        }
    }
    return res;
}

template <class T>
inline T *TDispatcher<T>::object()
{
    T_TRACEFUNC("");

    if (!ptr) {
        if (typeId <= 0 && !metaType.isEmpty()) {
            typeId = QMetaType::type(metaType.toLatin1().constData());
            if (typeId > 0) {
#if QT_VERSION >= 0x050000
                ptr = static_cast<T *>(QMetaType::create(typeId));
#else
                ptr = static_cast<T *>(QMetaType::construct(typeId));
#endif
                Q_CHECK_PTR(ptr);
                tSystemDebug("Constructs object, class: %s  typeId: %d", qPrintable(metaType), typeId);
            } else {
                tSystemDebug("No such object class : %s", qPrintable(metaType));
            }
        }
    }
    return ptr;
}

#endif // TDISPATCHER_H
