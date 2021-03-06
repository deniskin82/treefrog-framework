/* Copyright (c) 2010-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TSqlORMapper>
#include <TCriteria>
#include "tsessionsqlobjectstore.h"
#include "tsessionobject.h"

/*!
  \class TSessionSqlObjectStore
  \brief The TSessionSqlObjectStore class stores HTTP sessions into database
         system using object-relational mapping tool.
  \sa TSessionObject
*/

/* create table session ( id varchar(50) primary key, data blob, updated_at datetime ); */

bool TSessionSqlObjectStore::store(TSession &session)
{
    TSqlORMapper<TSessionObject> mapper;
    TCriteria cri(TSessionObject::Id, TSql::Equal, session.id());
    TSessionObject so = mapper.findFirst(cri);

    QDataStream ds(&so.data, QIODevice::WriteOnly);
    ds << *static_cast<const QVariantHash *>(&session);

    if (so.isEmpty()) {
        so.id = session.id();
        return so.create();
    }
    return so.update();
}


TSession TSessionSqlObjectStore::find(const QByteArray &id, const QDateTime &modified)
{
    TSqlORMapper<TSessionObject> mapper;
    TCriteria cri;
    cri.add(TSessionObject::Id, TSql::Equal, id);
    cri.add(TSessionObject::UpdatedAt, TSql::GreaterEqual, modified);

    TSessionObject sess = mapper.findFirst(cri);
    if (sess.isEmpty())
        return TSession();
    
    TSession result(id);
    QDataStream ds(&sess.data, QIODevice::ReadOnly);
    ds >> *static_cast<QVariantHash *>(&result);
    return result;  
}


bool TSessionSqlObjectStore::remove(const QDateTime &garbageExpiration)
{
    TSqlORMapper<TSessionObject> mapper;
    TCriteria cri(TSessionObject::UpdatedAt, TSql::LessThan, garbageExpiration);
    int cnt = mapper.removeAll(cri);
    return (cnt >= 0);
}


bool TSessionSqlObjectStore::remove(const QByteArray &id)
{
    TSqlORMapper<TSessionObject> mapper;
    int cnt = mapper.removeAll(TCriteria(TSessionObject::Id, id));
    return (cnt > 0);
}
