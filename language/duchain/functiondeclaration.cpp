/* This  is part of KDevelop
    Copyright 2002-2005 Roberto Raggi <roberto@kdevelop.org>
    Copyright 2006 Adam Treat <treat@kde.org>
    Copyright 2006-2007 Hamish Rodda <rodda@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "functiondeclaration.h"

#include "ducontext.h"

namespace KDevelop
{

class FunctionDeclarationPrivate
{
};

FunctionDeclaration::FunctionDeclaration(const FunctionDeclaration& rhs) : Declaration(rhs), AbstractFunctionDeclaration(rhs), d(new FunctionDeclarationPrivate) {
}

FunctionDeclaration::FunctionDeclaration(KTextEditor::Range * range, Scope scope, DUContext* context)
  : Declaration(range, scope, context), d(new FunctionDeclarationPrivate)
{
}

FunctionDeclaration::~FunctionDeclaration()
{
}

Declaration* FunctionDeclaration::clone() const {
  return new FunctionDeclaration(*this);
}

}
// kate: space-indent on; indent-width 2; tab-width: 4; replace-tabs on; auto-insert-doxygen on
