/***************************************************************************
 *   Copyright 1999-2001 Bernd Gehrmann and the KDevelop Team              *
 *   bernd@kdevelop.org                                                    *
 *   Copyright 2007 Dukju Ahn <dukjuahn@gmail.com>                         *
 *   Copyright 2010 Silvère Lestang <silvere.lestang@gmail.com>            *
 *   Copyright 2010 Julien Desgats <julien.desgats@gmail.com>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef GREPOUTPUTMODEL_H
#define GREPOUTPUTMODEL_H

#include <QStandardItemModel>
#include <QList>

#include <language/codegen/documentchangeset.h>

class QModelIndex;
class QRegExp;

namespace KDevelop {
    class IStatus;
}

class GrepOutputItem : public QStandardItem
{
public:
    typedef QList<GrepOutputItem> List;

    GrepOutputItem(KDevelop::DocumentChangePointer change, const QString &text);
    GrepOutputItem(const QString &filename, const QString &text);
    ~GrepOutputItem();

    QString filename() const ;
    int lineNumber() const ;
    KDevelop::DocumentChangePointer change() const ;
    bool isText() const ;
    /// Recursively apply check state to children
    void propagateState() ;
    /// Check children to determine current state
    void refreshState() ;

    virtual QVariant data ( int role = Qt::UserRole + 1 ) const;

private:
    KDevelop::DocumentChangePointer m_change;
   
    void showCollapsed();
    void showExpanded();
};

Q_DECLARE_METATYPE(GrepOutputItem::List);

class GrepOutputModel : public QStandardItemModel
{
    Q_OBJECT

public:
    explicit GrepOutputModel( QObject *parent = 0 );
    ~GrepOutputModel();

    void setRegExp(const QRegExp& re);
    void setReplacementTemplate(const QString &tmpl);
    /// applies replacement on given text
    QString replacementFor(const QString &text);
    void clear();  // resets file & match counts
 
    QModelIndex previousItemIndex(const QModelIndex &currentIdx) const;
    QModelIndex nextItemIndex(const QModelIndex &currentIdx) const;
    
public Q_SLOTS:
    void appendOutputs( const QString &filename, const GrepOutputItem::List &lines );
    void activate( const QModelIndex &idx );
    void doReplacements();
    void setReplacement(const QString &repl);

Q_SIGNALS:
    void showErrorMessage(const QString & message, int timeout = 0);
    
private:    
    QRegExp m_regExp;
    QString m_replacement;
    QString m_replacementTemplate;
    QString m_finalReplacement;
    bool m_finalUpToDate;  /// says if m_finalReplacement is up to date or must be regenerated
    GrepOutputItem *m_rootItem;
    int m_fileCount;
    int m_matchCount;

private slots:
    void updateCheckState(QStandardItem*);
};

#endif
