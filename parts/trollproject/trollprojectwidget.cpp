/***************************************************************************
 *   Copyright (C) 2001 by Bernd Gehrmann                                  *
 *   bernd@kdevelop.org                                                    *
 *   Copyright (C) 2000-2001 by Trolltech AS.                              *
 *   info@trolltech.com                                                    *
 *   Copyright (C) 2002 by Jakob Simon-Gaarde                              *
 *   jakob@jsg.dk                                                          *
 *                                                                         *
 *   Part of this file is taken from Qt Designer.                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "trollprojectwidget.h"

#include <qfile.h>
#include <qfileinfo.h>
#include <qheader.h>
#include <qsplitter.h>
#include <qptrstack.h>
#include <qtextstream.h>
#include <qprocess.h>
#include <qtimer.h>
#include <qdir.h>
#include <qinputdialog.h>
#include <qfiledialog.h>
#include <kdebug.h>
#include <klistview.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kregexp.h>
#include <kurl.h>
#include <qmessagebox.h>

#include "kdevcore.h"
#include "kdevpartcontroller.h"
#include "kdevtoplevel.h"
#include "domutil.h"
#include "trollprojectpart.h"


/**
 * Class ProjectViewItem
 */

ProjectItem::ProjectItem(Type type, QListView *parent, const QString &text)
    : QListViewItem(parent, text), typ(type)
{}


ProjectItem::ProjectItem(Type type, ProjectItem *parent, const QString &text)
    : QListViewItem(parent, text), typ(type)
{}


/**
 * Class SubprojectItem
 */

SubprojectItem::SubprojectItem(QListView *parent, const QString &text, const QString &scopeString)
    : ProjectItem(Subproject, parent, text)
{
    this->scopeString=scopeString;
    configuration.m_template = QTMP_APPLICATION;
    init();
}


SubprojectItem::SubprojectItem(SubprojectItem *parent, const QString &text,const QString &scopeString)
    : ProjectItem(Subproject, parent, text)
{
    this->scopeString=scopeString;
    init();
}

SubprojectItem::~SubprojectItem()
{
}


void SubprojectItem::init()
{
  configuration.m_template = QTMP_APPLICATION;
  configuration.m_warnings = QWARN_ON;
  configuration.m_buildMode = QBM_RELEASE;
  configuration.m_requirements = QD_QT;
  groups.setAutoDelete(true);
  if (scopeString=="")
  {
    isScope = false;
    setPixmap(0, SmallIcon("folder"));
  }
  else
  {
    isScope = true;
    setPixmap(0, SmallIcon("qmake_scope.png"));
  }
}


/**
 * Class GroupItem
 */

GroupItem::GroupItem(QListView *lv, GroupType type, const QString &text, const QString &scopeString)
    : ProjectItem(Group, lv, text)
{
    this->scopeString = scopeString;
    groupType = type;
    files.setAutoDelete(true);
    setPixmap(0, SmallIcon("tar"));
}


/**
 * Class FileItem
 */

FileItem::FileItem(QListView *lv, const QString &text)
    : ProjectItem(File, lv, text)
{
    setPixmap(0, SmallIcon("document"));
}


TrollProjectWidget::TrollProjectWidget(TrollProjectPart *part)
    : QVBox(0, "troll project widget")
{
    QSplitter *splitter = new QSplitter(Vertical, this);

    //////////////////
    // PROJECT VIEW //
    //////////////////

    overviewContainer = new QVBox(splitter,"Projects");
    overviewContainer->setMargin ( 2 );
    overviewContainer->setSpacing ( 2 );
    projectTools = new QHBox(overviewContainer,"Project buttons");
    projectTools->setMargin ( 2 );
    projectTools->setSpacing ( 2 );
    // build
    buildButton = new QToolButton ( projectTools, "Build button" );
    buildButton->setPixmap ( SmallIcon ( "qmake_build.png",22 ) );
    buildButton->setSizePolicy ( QSizePolicy ( ( QSizePolicy::SizeType ) 0, ( QSizePolicy::SizeType) 0, 0, 0, buildButton->sizePolicy().hasHeightForWidth() ) );
    buildButton->setEnabled ( true );
    // rebuild
    rebuildButton = new QToolButton ( projectTools, "Rebuild button" );
    rebuildButton->setPixmap ( SmallIcon ( "qmake_rebuild.png",22 ) );
    rebuildButton->setSizePolicy ( QSizePolicy ( ( QSizePolicy::SizeType ) 0, ( QSizePolicy::SizeType) 0, 0, 0, rebuildButton->sizePolicy().hasHeightForWidth() ) );
    rebuildButton->setEnabled ( true );
    // run
    runButton = new QToolButton ( projectTools, "Run button" );
    runButton->setPixmap ( SmallIcon ( "qmake_run.png",22 ) );
    runButton->setSizePolicy ( QSizePolicy ( ( QSizePolicy::SizeType ) 0, ( QSizePolicy::SizeType) 0, 0, 0, runButton->sizePolicy().hasHeightForWidth() ) );
    runButton->setEnabled ( true );
    // spacer
    QWidget *spacer = new QWidget(projectTools);
    projectTools->setStretchFactor(spacer, 1);
    // Project configuration
    projectconfButton = new QToolButton ( projectTools, "Project configuration button" );
    projectconfButton->setPixmap ( SmallIcon ( "configure.png",22 ) );
    projectconfButton->setText("Configure");
    projectconfButton->setSizePolicy ( QSizePolicy ( ( QSizePolicy::SizeType ) 0, ( QSizePolicy::SizeType) 0, 0, 0, projectconfButton->sizePolicy().hasHeightForWidth() ) );
    projectconfButton->setEnabled ( true );

    // Project button connections
    connect ( buildButton, SIGNAL ( clicked () ), this, SLOT ( slotBuildProject () ) );
    connect ( rebuildButton, SIGNAL ( clicked () ), this, SLOT ( slotRebuildProject () ) );
    connect ( runButton, SIGNAL ( clicked () ), this, SLOT ( slotRunProject () ) );
    connect ( projectconfButton, SIGNAL ( clicked () ), this, SLOT ( slotConfigureProject () ) );

    // Project tree
    overview = new KListView(overviewContainer, "project overview widget");
    overview->setResizeMode(QListView::LastColumn);
    overview->setSorting(-1);
    overview->header()->hide();
    overview->addColumn(QString::null);

    // Project tree connections
    connect( overview, SIGNAL(selectionChanged(QListViewItem*)),
             this, SLOT(slotOverviewSelectionChanged(QListViewItem*)) );
    connect( overview, SIGNAL(contextMenu(KListView*, QListViewItem*, const QPoint&)),
             this, SLOT(slotOverviewContextMenu(KListView*, QListViewItem*, const QPoint&)) );
    buildButton->setEnabled(false);
    rebuildButton->setEnabled(false);
    runButton->setEnabled(false);
    projectconfButton->setEnabled(false);


    /////////////////
    // DETAIL VIEW //
    /////////////////

    // Details tree
    detailContainer = new QVBox(splitter,"Details");
    detailContainer->setMargin ( 2 );
    detailContainer->setSpacing ( 2 );

    // Details Toolbar
    fileTools = new QHBox(detailContainer,"Detail buttons");
    fileTools->setMargin ( 2 );
    fileTools->setSpacing ( 2 );

    // Add existing files button
    addfilesButton = new QToolButton ( fileTools, "Add existing files" );
    addfilesButton->setPixmap ( SmallIcon ( "qmake_addexisting.png",22 ) );
    addfilesButton->setSizePolicy ( QSizePolicy ( ( QSizePolicy::SizeType ) 0, ( QSizePolicy::SizeType) 0, 0, 0, addfilesButton->sizePolicy().hasHeightForWidth() ) );
    addfilesButton->setEnabled ( true );

    // Add new file button
    newfileButton = new QToolButton ( fileTools, "Add new file" );
    newfileButton->setPixmap ( SmallIcon ( "qmake_addnew.png",22 ) );
    newfileButton->setSizePolicy ( QSizePolicy ( ( QSizePolicy::SizeType ) 0, ( QSizePolicy::SizeType) 0, 0, 0, newfileButton->sizePolicy().hasHeightForWidth() ) );
    newfileButton->setEnabled ( true );

    // remove file button
    removefileButton = new QToolButton ( fileTools, "remove file" );
    removefileButton->setPixmap ( SmallIcon ( "qmake_remove.png",22 ) );
    removefileButton->setSizePolicy ( QSizePolicy ( ( QSizePolicy::SizeType ) 0, ( QSizePolicy::SizeType) 0, 0, 0, removefileButton->sizePolicy().hasHeightForWidth() ) );
    removefileButton->setEnabled ( true );

    // spacer
    spacer = new QWidget(fileTools);
    projectTools->setStretchFactor(spacer, 1);

    // detail tree
    details = new KListView(detailContainer, "details widget");
    details->setRootIsDecorated(true);
    details->setResizeMode(QListView::LastColumn);
    details->setSorting(-1);
    details->header()->hide();
    details->addColumn(QString::null);

    // Detail button connections
    connect ( addfilesButton, SIGNAL ( clicked () ), this, SLOT ( slotAddFiles () ) );
    connect ( newfileButton, SIGNAL ( clicked () ), this, SLOT ( slotNewFile () ) );
    connect ( removefileButton, SIGNAL ( clicked () ), this, SLOT ( slotRemoveFile () ) );

    // Detail tree connections
    connect( details, SIGNAL(selectionChanged(QListViewItem*)),
             this, SLOT(slotDetailsSelectionChanged(QListViewItem*)) );
    connect( details, SIGNAL(executed(QListViewItem*)),
             this, SLOT(slotDetailsExecuted(QListViewItem*)) );
    connect( details, SIGNAL(contextMenu(KListView*, QListViewItem*, const QPoint&)),
             this, SLOT(slotDetailsContextMenu(KListView*, QListViewItem*, const QPoint&)) );

    m_part = part;
    m_shownSubproject = 0;
}


TrollProjectWidget::~TrollProjectWidget()
{}


void TrollProjectWidget::openProject(const QString &dirName)
{
    SubprojectItem *item = new SubprojectItem(overview, "/","");
    item->subdir = dirName.right(dirName.length()-dirName.findRev('/'));
    item->path = dirName;
    item->m_RootBuffer = &(item->m_FileBuffer);
    parse(item);
    item->setOpen(true);
    overview->setSelected(item, true);
}


void TrollProjectWidget::closeProject()
{
    overview->clear();
    details->clear();
}


QStringList TrollProjectWidget::allSubprojects()
{
    int prefixlen = projectDirectory().length()+1;
    QStringList res;

    QListViewItemIterator it(overview);
    for (; it.current(); ++it) {
        if (it.current() == overview->firstChild())
            continue;
        QString path = static_cast<SubprojectItem*>(it.current())->path;
        res.append(path.mid(prefixlen));
    }

    return res;
}


QStringList TrollProjectWidget::allFiles()
{
    QStack<QListViewItem> s;
    QStringList res;

    for ( QListViewItem *item = overview->firstChild(); item;
          item = item->nextSibling()? item->nextSibling() : s.pop() ) {
        if (item->firstChild())
            s.push(item->firstChild());

        SubprojectItem *spitem = static_cast<SubprojectItem*>(item);
        QString path = spitem->path;
        QListIterator<GroupItem> tit(spitem->groups);
        for (; tit.current(); ++tit) {
            GroupItem::GroupType type = (*tit)->groupType;
            if (type == GroupItem::Sources || type == GroupItem::Headers) {
                QListIterator<FileItem> fit(tit.current()->files);
                for (; fit.current(); ++fit)
                    res.append(path + "/" + (*fit)->name);
            }
        }
    }

    return res;
}

QString TrollProjectWidget::projectDirectory()
{
    if (!overview->firstChild())
        return QString::null; //confused

    return static_cast<SubprojectItem*>(overview->firstChild())->path;
}


QString TrollProjectWidget::subprojectDirectory()
{
    if (!m_shownSubproject)
        return QString::null;

    return m_shownSubproject->path;
}



void TrollProjectWidget::slotOverviewSelectionChanged(QListViewItem *item)
{
    if (!item)
        return;
    cleanDetailView(m_shownSubproject);
    m_shownSubproject = static_cast<SubprojectItem*>(item);
    buildProjectDetailTree(m_shownSubproject,details);
    if (m_shownSubproject->isScope)
    {
      buildButton->setEnabled(false);
      rebuildButton->setEnabled(false);
      runButton->setEnabled(false);
      projectconfButton->setEnabled(false);
    }
    else
    {
      buildButton->setEnabled(true);
      rebuildButton->setEnabled(true);
      runButton->setEnabled(true);
      projectconfButton->setEnabled(true);
    }
}

void TrollProjectWidget::cleanDetailView(SubprojectItem *item)
{
  if (item)
  {
    // Remove all GroupItems and all of their children from the view
//    QListIterator<SubprojectItem> it(item->scopes);
//    for (; it.current(); ++it)
//    {
//      cleanDetailView(*it);
//      details->takeItem(*it);
//    }
    QListIterator<GroupItem> it1(item->groups);
    for (; it1.current(); ++it1) {
      // After AddTargetDialog, it can happen that an
      // item is not yet in the list view, so better check...
      if (it1.current()->parent())
      while ((*it1)->firstChild())
        (*it1)->takeItem((*it1)->firstChild());
      details->takeItem(*it1);
    }
  }

}

void TrollProjectWidget::buildProjectDetailTree(SubprojectItem *item,KListView *listviewControl)
{
  // Insert all GroupItems and all of their children into the view
  if (listviewControl)
  {
//    QListIterator<SubprojectItem> it1(item->scopes);
//    for (; it1.current(); ++it1)
//    {
//      listviewControl->insertItem(*it1);
//      buildProjectDetailTree(*it1,NULL);
//    }
    QListIterator<GroupItem> it2(item->groups);
    for (; it2.current(); ++it2)
    {
        listviewControl->insertItem(*it2);
        QListIterator<FileItem> it3((*it2)->files);
        for (; it3.current(); ++it3)
            (*it2)->insertItem(*it3);
        (*it2)->setOpen(true);
    }
  }
  else
  {
//    QListIterator<SubprojectItem> it1(item->scopes);
//    for (; it1.current(); ++it1)
//    {
//      item->insertItem(*it1);
//      buildProjectDetailTree(*it1,NULL);
//    }
    QListIterator<GroupItem> it2(item->groups);
    for (; it2.current(); ++it2)
    {
        item->insertItem(*it2);
        QListIterator<FileItem> it3((*it2)->files);
        for (; it3.current(); ++it3)
            (*it2)->insertItem(*it3);
        (*it2)->setOpen(true);
    }
  }
}

void TrollProjectWidget::slotDetailsExecuted(QListViewItem *item)
{
    if (!item)
        return;

    // We assume here that ALL items in both list views
    // are ProjectItem's
    ProjectItem *pvitem = static_cast<ProjectItem*>(item);
    if (pvitem->type() != ProjectItem::File)
        return;

    QString dirName = m_shownSubproject->path;
    FileItem *fitem = static_cast<FileItem*>(pvitem);
    m_part->partController()->editDocument(KURL(dirName + "/" + QString(fitem->name)));
    m_part->topLevel()->lowerView(this);
}


void TrollProjectWidget::slotConfigureProject()
{
  ProjectConfigurationDlg *dlg = new ProjectConfigurationDlg(&(m_shownSubproject->configuration));
  dlg->exec();
  updateProjectConfiguration(m_shownSubproject);
}

void TrollProjectWidget::slotRunProject()
{
  // no subproject selected
  if (!m_shownSubproject)
    return;
  // can't build from scope
  if (m_shownSubproject->isScope)
    return;
  // Only run application projects
  if (m_shownSubproject->configuration.m_template!=QTMP_APPLICATION)
    return;
  QString relpath = m_shownSubproject->path.mid(projectDirectory().length());
  m_part->execute(projectDirectory()+relpath+"/"+m_shownSubproject->subdir);

}

void TrollProjectWidget::slotBuildProject()
{
  // no subproject selected
  if (!m_shownSubproject)
    return;
  // can't build from scope
  if (m_shownSubproject->isScope)
    return;
  QString relpath = m_shownSubproject->path.mid(projectDirectory().length());
  m_part->startMakeCommand(projectDirectory() + relpath, QString::fromLatin1(""));
  m_part->topLevel()->lowerView(this);
}

void TrollProjectWidget::slotRebuildProject()
{
  // no subproject selected
  if (!m_shownSubproject)
    return;
  // can't build from scope
  if (m_shownSubproject->isScope)
    return;
  QString relpath = m_shownSubproject->path.mid(projectDirectory().length());
  m_part->startMakeCommand(projectDirectory() + relpath, QString::fromLatin1(""));
  m_part->topLevel()->lowerView(this);
}

void TrollProjectWidget::slotOverviewContextMenu(KListView *, QListViewItem *item, const QPoint &p)
{
    if (!item)
        return;

    SubprojectItem *spitem = static_cast<SubprojectItem*>(item);

    if (spitem->isScope)
      return;
    KPopupMenu popup(i18n("Subproject %1").arg(item->text(0)), this);
    int idAddSubproject = popup.insertItem(SmallIcon("folder_new"),i18n("Add Subproject..."));
    int idBuild = popup.insertItem(SmallIcon("launch"),i18n("Build"));
    int idQmake = popup.insertItem(SmallIcon("launch"),i18n("Run qmake"));
    int idProjectConfiguration = popup.insertItem(SmallIcon("configure.png"),i18n("Subproject settings"));
    int r = popup.exec(p);

    QString relpath = spitem->path.mid(projectDirectory().length());
    if (r == idAddSubproject)
    {

      bool ok = FALSE;
      QString subdirname = QInputDialog::getText(
                        tr( "Add Subdir" ),
                        tr( "Please enter a name for the new subdir." ),
                        QLineEdit::Normal, QString::null, &ok, this );
      if ( ok && !subdirname.isEmpty() )
      {
        QDir dir(projectDirectory()+relpath);
        if (!dir.mkdir(subdirname))
        {
          KMessageBox::error(this,i18n("Failed to create subdirectory. "
                                       "Do you have write permission "
                                       "in the projectfolder?" ));
          return;
        }
        spitem->subdirs.append(subdirname);
        updateProjectFile(spitem);
        SubprojectItem *newitem = new SubprojectItem(spitem, subdirname,"");
        newitem->subdir = subdirname;
        newitem->path = spitem->path + "/" + subdirname;

      }
      else
        return;
    }
    else if (r == idBuild)
    {
        m_part->startMakeCommand(projectDirectory() + relpath, QString::fromLatin1(""));
        m_part->topLevel()->lowerView(this);
    }
    else if (r == idQmake)
    {
        m_part->startQMakeCommand(projectDirectory() + relpath);
        m_part->topLevel()->lowerView(this);
    }
    else if (r == idProjectConfiguration)
    {
      ProjectConfigurationDlg *dlg = new ProjectConfigurationDlg(&(spitem->configuration));
      dlg->exec();
      updateProjectConfiguration(spitem);
    }
}

void TrollProjectWidget::updateProjectConfiguration(SubprojectItem *item)
//=======================================================================
{
  FileBuffer *Buffer = &(item->m_FileBuffer);
  QString relpath = item->path.mid(projectDirectory().length());
  Buffer->removeValues("TEMPLATE");
  if (item->configuration.m_template == QTMP_APPLICATION)
    Buffer->setValues("TEMPLATE",QString("app"));
  if (item->configuration.m_template == QTMP_LIBRARY)
    Buffer->setValues("TEMPLATE",QString("lib"));
  if (item->configuration.m_template == QTMP_SUBDIRS)
    Buffer->setValues("TEMPLATE",QString("subdirs"));
  Buffer->removeValues("CONFIG");
  QStringList configList;
  if (item->configuration.m_buildMode == QBM_RELEASE)
    configList.append("release");
  else if (item->configuration.m_buildMode == QBM_DEBUG)
    configList.append("debug");
  if (item->configuration.m_warnings == QWARN_ON)
    configList.append("warn_on");
  else if (item->configuration.m_warnings == QWARN_OFF)
    configList.append("warn_off");
  if (item->configuration.m_requirements & QD_QT)
    configList.append("qt");
  if (item->configuration.m_requirements & QD_OPENGL)
    configList.append("opengl");
  if (item->configuration.m_requirements & QD_THREAD)
    configList.append("thread");
  if (item->configuration.m_requirements & QD_X11)
    configList.append("x11");
  Buffer->setValues("CONFIG",configList,5,true);
  Buffer->saveBuffer(projectDirectory()+relpath+"/"+m_shownSubproject->subdir+".pro");
}


void TrollProjectWidget::updateProjectFile(QListViewItem *item)
{
  SubprojectItem *spitem = static_cast<SubprojectItem*>(item);
  QString relpath = m_shownSubproject->path.mid(projectDirectory().length());
  FileBuffer *subBuffer=m_shownSubproject->m_RootBuffer->getSubBuffer(spitem->scopeString);
  subBuffer->removeValues("SUBDIRS");
  subBuffer->setValues("SUBDIRS",spitem->subdirs,4,true);
  subBuffer->removeValues("SOURCES");
  subBuffer->setValues("SOURCES",spitem->sources,4,true);
  subBuffer->removeValues("HEADERS");
  subBuffer->setValues("HEADERS",spitem->headers,4,true);
  subBuffer->removeValues("FORMS");
  subBuffer->setValues("FORMS",spitem->forms,4,true);
  m_shownSubproject->m_RootBuffer->saveBuffer(projectDirectory()+relpath+"/"+m_shownSubproject->subdir+".pro");
}

void TrollProjectWidget::addFileToCurrentSubProject(GroupItem *titem,const QString &filename)
{
  FileItem *fitem = createFileItem(filename);
  titem->files.append(fitem);
  switch (titem->groupType)
  {
    case GroupItem::Sources:
      titem->owner->sources.append(filename);
      break;
    case GroupItem::Headers:
      titem->owner->headers.append(filename);
      break;
    case GroupItem::Forms:
      titem->owner->forms.append(filename);
      break;
  }
}

void TrollProjectWidget::addFileToCurrentSubProject(GroupItem::GroupType gtype,const QString &filename)
{
  if (!m_shownSubproject)
    return;
  FileItem *fitem = createFileItem(filename);
  GroupItem *gitem = 0;

  QListIterator<GroupItem> it(m_shownSubproject->groups);
  for (; it.current(); ++it)
  {
    if ((*it)->groupType == gtype)
    {
      gitem = *it;
      break;
    }
  }
  if (!gitem)
    return;
  gitem->files.append(fitem);
  switch (gtype)
  {
    case GroupItem::Sources:
      m_shownSubproject->sources.append(filename);
      break;
    case GroupItem::Headers:
      m_shownSubproject->headers.append(filename);
      break;
    case GroupItem::Forms:
      m_shownSubproject->forms.append(filename);
      break;
  }
}

/**
* Method adds a file to the current project by grouped
* by file extension
*/
void TrollProjectWidget::addFile(const QString &fileName)
{
  if (!m_shownSubproject)
    return;
  QStringList splitFile = QStringList::split('.',fileName);
  QString ext = splitFile[splitFile.count()-1];
  splitFile = QStringList::split('/',fileName);
  QString fileWithNoSlash = splitFile[splitFile.count()-1];
  ext = ext.simplifyWhiteSpace();
  if (QString("cpp c").find(ext)>-1)
    addFileToCurrentSubProject(GroupItem::Sources,fileWithNoSlash);
  else if (QString("hpp h").find(ext)>-1)
    addFileToCurrentSubProject(GroupItem::Headers,fileWithNoSlash);
  else if (QString("ui").find(ext)>-1)
    addFileToCurrentSubProject(GroupItem::Forms,fileWithNoSlash);
  else
    addFileToCurrentSubProject(GroupItem::NoType,fileWithNoSlash);
  updateProjectFile(m_shownSubproject);
  slotOverviewSelectionChanged(m_shownSubproject);
}


void TrollProjectWidget::slotAddFiles()
{
  QString relpath = m_shownSubproject->path.mid(projectDirectory().length());
  QString  sourceFiles = "Source files (*.cpp *.c *.hpp *.h *.ui)";
  QString  allFiles = "All files (*)";
  QFileDialog *dialog = new QFileDialog(projectDirectory()+relpath,
                                        sourceFiles,
                                        this,
                                        "Insert existing files",
                                        TRUE);
  dialog->addFilter(allFiles);
  dialog->setMode(QFileDialog::ExistingFiles);
  dialog->exec();
  QStringList files = dialog->selectedFiles();
  for (unsigned int i=0;i<files.count();i++)
  {
    // Copy selected files to current subproject folder
    QProcess *proc = new QProcess( this );
    proc->addArgument( "cp" );
    proc->addArgument( "-f" );
    proc->addArgument( files[i] );
    proc->addArgument( projectDirectory()+relpath );
    proc->start();
    QString filename = files[i].right(files[i].length()-files[i].findRev('/')-1);
    // and add them to the filelist
    addFile(filename);
  }
}

void TrollProjectWidget::slotNewFile()
{
  bool ok = FALSE;
  QString relpath = m_shownSubproject->path.mid(projectDirectory().length());
  QString filename = QInputDialog::getText(
                     tr( "Insert New File"),
                     tr( "Please enter a name for the new file." ),
                     QLineEdit::Normal, QString::null, &ok, this );
  if ( ok && !filename.isEmpty() )
  {
    QFile newfile(projectDirectory()+relpath+'/'+filename);
    if (!newfile.open(IO_WriteOnly))
    {
      KMessageBox::error(this,i18n("Failed to create new file. "
                                   "Do you have write permission "
                                   "in the projectfolder?" ));
      return;
    }
    newfile.close();
    addFile(projectDirectory()+relpath+'/'+filename);
  }
}

void TrollProjectWidget::slotRemoveFile()
{
  QListViewItem *selectedItem = details->currentItem();
  if (!selectedItem)
    return;
  ProjectItem *pvitem = static_cast<ProjectItem*>(selectedItem);
  // Check that it is a file (just in case)
  if (pvitem->type() != ProjectItem::File)
    return;
  FileItem *fitem = static_cast<FileItem*>(pvitem);
  removeFile(m_shownSubproject, fitem);
}


void TrollProjectWidget::slotDetailsSelectionChanged(QListViewItem *item)
{
    if (!item)
        return;
    ProjectItem *pvitem = static_cast<ProjectItem*>(item);
    if (pvitem->type() == ProjectItem::Group)
    {
        removefileButton->setEnabled(false);
    }
    else if (pvitem->type() == ProjectItem::File)
    {
        removefileButton->setEnabled(true);
    }
}

void TrollProjectWidget::slotDetailsContextMenu(KListView *, QListViewItem *item, const QPoint &p)
{
    if (!item)
        return;

    ProjectItem *pvitem = static_cast<ProjectItem*>(item);
    if (pvitem->type() == ProjectItem::Group) {
        GroupItem *titem = static_cast<GroupItem*>(pvitem);
        QString title,ext;
        switch (titem->groupType) {
        case GroupItem::Sources:
            title = i18n("Sources");
            ext = "*.cpp *.c";
            break;
        case GroupItem::Headers:
            title = i18n("Headers");
            ext = "*.h *.hpp";
            break;
        case GroupItem::Forms:
            title = i18n("Forms");
            ext = "*.ui";
            break;
        default: ;
        }

        KPopupMenu popup(title, this);
        int idInsExistingFile = popup.insertItem(SmallIconSet("fileopen"),i18n("Insert existing files..."));
        int idInsNewFile = popup.insertItem(SmallIconSet("filenew"),i18n("Insert new file..."));
        int idFileProperties = popup.insertItem(SmallIconSet("filenew"),i18n("Properties..."));
        int r = popup.exec(p);
        QString relpath = m_shownSubproject->path.mid(projectDirectory().length());
        if (r == idInsExistingFile)
        {
          QFileDialog *dialog = new QFileDialog(projectDirectory()+relpath,
                                                title + " ("+ext+")",
                                                this,
                                                "Insert existing "+ title,
                                                TRUE);
          dialog->setMode(QFileDialog::ExistingFiles);
          dialog->exec();
          QStringList files = dialog->selectedFiles();
          for (unsigned int i=0;i<files.count();i++)
          {
            // Copy selected files to current subproject folder
            QProcess *proc = new QProcess( this );
            proc->addArgument( "cp" );
            proc->addArgument( "-f" );
            proc->addArgument( files[i] );
            proc->addArgument( projectDirectory()+relpath );
            proc->start();
            QString filename = files[i].right(files[i].length()-files[i].findRev('/')-1);
            // and add them to the filelist
            addFileToCurrentSubProject(titem,filename);
          }
          // Update project file
          updateProjectFile(titem->owner);
          // Update subprojectview
          slotOverviewSelectionChanged(m_shownSubproject);
        }
        if (r == idInsNewFile)
        {
          bool ok = FALSE;
          QString filename = QInputDialog::getText(
                            tr( "Insert New File"),
                            tr( "Please enter a name for the new file." ),
                            QLineEdit::Normal, QString::null, &ok, this );
          if ( ok && !filename.isEmpty() )
          {
            QFile newfile(projectDirectory()+relpath+'/'+filename);
            if (!newfile.open(IO_WriteOnly))
            {
              KMessageBox::error(this,i18n("Failed to create new file. "
                                           "Do you have write permission "
                                           "in the projectfolder?" ));
              return;
            }
            newfile.close();
            addFileToCurrentSubProject(titem,filename);
            updateProjectFile(titem->owner);
            slotOverviewSelectionChanged(m_shownSubproject);
          }
        }

    } else if (pvitem->type() == ProjectItem::File) {

        removefileButton->setEnabled(true);
        FileItem *fitem = static_cast<FileItem*>(pvitem);

        KPopupMenu popup(i18n("File: %1").arg(fitem->name), this);
        int idRemoveFile = popup.insertItem(i18n("Remove File..."));
        int idFileProperties = popup.insertItem(i18n("Properties..."));

        FileContext context(m_shownSubproject->path + "/" + fitem->name, false);
        m_part->core()->fillContextMenu(&popup, &context);

        int r = popup.exec(p);
        if (r == idRemoveFile)
            removeFile(m_shownSubproject, fitem);
        // Fileproperties
        else if (r == idFileProperties)
        {
          FilePropertyDlg *propdlg = new FilePropertyDlg(m_shownSubproject,fitem);
          propdlg->exec();
        }

    }
}


void TrollProjectWidget::removeFile(SubprojectItem *spitem, FileItem *fitem)
{
    GroupItem *gitem = static_cast<GroupItem*>(fitem->parent());

    emitRemovedFile(spitem->path + "/" + fitem->text(0));
    switch (gitem->groupType)
    {
      case GroupItem::Sources:
        spitem->sources.remove(fitem->text(0));
        break;
      case GroupItem::Headers:
        spitem->headers.remove(fitem->text(0));
        break;
      case GroupItem::Forms:
        spitem->forms.remove(fitem->text(0));
        break;
      default: ;
    }
    gitem->files.remove(fitem);
    updateProjectFile(m_shownSubproject);
}


GroupItem *TrollProjectWidget::createGroupItem(GroupItem::GroupType groupType, const QString &name,const QString &scopeString)
{
    // Workaround because for QListView not being able to create
    // items without actually inserting them
    GroupItem *titem = new GroupItem(overview, groupType, name,scopeString);
    overview->takeItem(titem);

    return titem;
}


FileItem *TrollProjectWidget::createFileItem(const QString &name)
{
    FileItem *fitem = new FileItem(overview, name);
    overview->takeItem(fitem);
    fitem->name = name;

    return fitem;
}

void TrollProjectWidget::emitAddedFile(const QString &fileName)
{
    emit m_part->addedFileToProject(fileName);
}


void TrollProjectWidget::emitRemovedFile(const QString &fileName)
{
    emit m_part->removedFileFromProject(fileName);
}


void TrollProjectWidget::parseScope(SubprojectItem *item, QString scopeString, FileBuffer *buffer)
{
    if (scopeString!="")
    {
      QStringList scopeNames = QStringList::split(':',scopeString);
      SubprojectItem *sitem;
      sitem = new SubprojectItem(item, scopeNames[scopeNames.count()-1],scopeString);
      sitem->path = item->path;
      sitem->m_RootBuffer = buffer;
      sitem->subdir = item->subdir;
      item->scopes.append(sitem);
      item=sitem;
    }

    QStringList minusListDummy;
    FileBuffer *subBuffer = buffer->getSubBuffer(scopeString);
    subBuffer->getValues("FORMS",item->forms,minusListDummy);
    subBuffer->getValues("SOURCES",item->sources,minusListDummy);
    subBuffer->getValues("HEADERS",item->headers,minusListDummy);

    // Create list view items
    GroupItem *titem = createGroupItem(GroupItem::Forms, "FORMS",scopeString);
    item->groups.append(titem);
    titem->owner = item;
    if (!item->forms.isEmpty()) {
        QStringList l = item->forms;
        QStringList::Iterator it;
        for (it = l.begin(); it != l.end(); ++it) {
            FileItem *fitem = createFileItem(*it);
            titem->files.append(fitem);
        }
    }
    titem = createGroupItem(GroupItem::Sources, "SOURCES",scopeString);
    item->groups.append(titem);
    titem->owner = item;
    if (!item->sources.isEmpty()) {
        QStringList l = item->sources;
        QStringList::Iterator it;
        for (it = l.begin(); it != l.end(); ++it) {
            FileItem *fitem = createFileItem(*it);
            titem->files.append(fitem);
        }
    }
    titem = createGroupItem(GroupItem::Headers, "HEADERS",scopeString);
    titem->owner = item;
    item->groups.append(titem);
    if (!item->headers.isEmpty()) {
        QStringList l = item->headers;
        QStringList::Iterator it;
        for (it = l.begin(); it != l.end(); ++it) {
            FileItem *fitem = createFileItem(*it);
            titem->files.append(fitem);
        }
    }
    QStringList childScopes = subBuffer->getChildScopeNames();
    for (unsigned int i=0; i<childScopes.count();i++)
      parseScope(item,scopeString+(scopeString!="" ? ":" : "")+childScopes[i],buffer);
}

void TrollProjectWidget::parse(SubprojectItem *item)
{
    QFileInfo fi(item->path);
    QString proname = item->path + "/" + fi.baseName() + ".pro";
    kdDebug(9024) << "Parsing " << proname << endl;


    item->m_FileBuffer.bufferFile(proname);
    item->m_FileBuffer.handleScopes();

    parseScope(item,"",&(item->m_FileBuffer));
    QStringList plusListDummy;
    QStringList lst;
    item->m_FileBuffer.getValues("SUBDIRS",lst,plusListDummy);
    item->subdirs = lst;
    QStringList::Iterator it;
    for (it = lst.begin(); it != lst.end(); ++it)
    {
      SubprojectItem *newitem = new SubprojectItem(item, (*it),"");
      newitem->subdir = *it;
      newitem->m_RootBuffer = &(newitem->m_FileBuffer);
      newitem->path = item->path + "/" + (*it);
      parse(newitem);
    }

}

#include "trollprojectwidget.moc"
