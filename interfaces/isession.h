/* This file is part of KDevelop
Copyright 2008 Anreas Pakulat <apaku@gmx.de>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public License
along with this library; see the file COPYING.LIB.  If not, write to
the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
Boston, MA 02110-1301, USA.
*/

#ifndef ISESSION_H
#define ISESSION_H

#include "interfacesexport.h"
#include <QtCore/QObject>

#include <ksharedconfig.h>

class QString;
class KUrl;

namespace KDevelop
{

class IPlugin;

class KDEVPLATFORMINTERFACES_EXPORT ISession : public QObject
{
    Q_OBJECT
public:
    ISession( QObject* parent = 0 );
    virtual ~ISession();

    virtual QString name() = 0;
    virtual KUrl pluginDataArea( const IPlugin* ) = 0;
    virtual KSharedConfig::Ptr config() = 0;
};

}

#endif

