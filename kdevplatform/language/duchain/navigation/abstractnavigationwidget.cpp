/*
   Copyright 2007 David Nolden <david.nolden.kdevelop@art-master.de>

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


#include "abstractnavigationwidget.h"

#include <QWheelEvent>
#include <QApplication>
#include <QVBoxLayout>
#include <QMetaObject>
#include <QScrollBar>
#include <QTextBrowser>

#include <KLocalizedString>

#include "../duchainlock.h"
#include <debug.h>

namespace {
const int maxNavigationWidgetWidth = 580;
const int maxNavigationWidgetHeight = 400;
}

namespace KDevelop {

class AbstractNavigationWidgetPrivate
{
public:
  AbstractNavigationWidgetPrivate(AbstractNavigationWidget* q) : q(q) {}

  void anchorClicked(const QUrl&);

  AbstractNavigationWidget* q;

  NavigationContextPointer m_startContext;

  QPointer<QTextBrowser> m_browser;
  QWidget* m_currentWidget = nullptr;
  QString m_currentText;
  mutable QSize m_idealTextSize;
  AbstractNavigationWidget::DisplayHints m_hints = AbstractNavigationWidget::NoHints;

  NavigationContextPointer m_context;
};

AbstractNavigationWidget::AbstractNavigationWidget()
  : d(new AbstractNavigationWidgetPrivate(this))
{
  setPalette( QApplication::palette() );
  setFocusPolicy(Qt::NoFocus);
  resize(100, 100);
}

QSize AbstractNavigationWidget::sizeHint() const
{
  if(d->m_browser) {
    updateIdealSize();
    QSize ret = QSize(qMin(d->m_idealTextSize.width(), maxNavigationWidgetWidth),
                      qMin(d->m_idealTextSize.height(), maxNavigationWidgetHeight));
    if(d->m_idealTextSize.height()>=maxNavigationWidgetHeight) {
      //make space for the scrollbar in case it's not fitting
      ret.rwidth() += 17; //m_browser->verticalScrollBar()->width() returns 100, for some reason
    }

    if(d->m_currentWidget) {
      ret.setHeight( ret.height() + d->m_currentWidget->sizeHint().height() );
      if(d->m_currentWidget->sizeHint().width() > ret.width())
        ret.setWidth(d->m_currentWidget->sizeHint().width());
      if(ret.width() < 500) //When we embed a widget, give it some space, even if it doesn't have a large size-hint
        ret.setWidth(500);

    }
    return ret;
  } else
    return QWidget::sizeHint();
}

void AbstractNavigationWidget::initBrowser(int height)
{
  Q_ASSERT(!d->m_browser);
  Q_UNUSED(height);
  d->m_browser = new QTextBrowser;

  // since we can embed arbitrary HTML we have to make sure it stays readable by forcing a black-white palette
  QPalette p;
  p.setColor(QPalette::AlternateBase, Qt::white);
  p.setColor(QPalette::Base, Qt::white);
  p.setColor(QPalette::Text, Qt::black);
  d->m_browser->setPalette( p );

  d->m_browser->setOpenLinks(false);
  d->m_browser->setOpenExternalLinks(false);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->addWidget(d->m_browser);
  layout->setMargin(0);
  setLayout(layout);

  connect(d->m_browser.data(), &QTextEdit::selectionChanged, this, [this]() {
    d->m_browser->copy();
  });
  connect(d->m_browser.data(), &QTextBrowser::anchorClicked, this, [&](const QUrl& url) { d->anchorClicked(url); });

  foreach(QWidget* w, findChildren<QWidget*>())
    w->setContextMenuPolicy(Qt::NoContextMenu);
}

AbstractNavigationWidget::~AbstractNavigationWidget() {
  if(d->m_currentWidget)
    layout()->removeWidget(d->m_currentWidget);
}

void AbstractNavigationWidget::setContext(NavigationContextPointer context, int initBrows)
{
  if(d->m_browser == nullptr)
    initBrowser(initBrows);

  if(!context) {
    qCDebug(LANGUAGE) << "no new context created";
    return;
  }
  if(context == d->m_context && (!context || context->alreadyComputed()))
    return;

  if (!d->m_startContext) {
    d->m_startContext = context;
  }

  bool wasInitial = (d->m_context == d->m_startContext);

  d->m_context = context;
  update();

  emit contextChanged(wasInitial, d->m_context == d->m_startContext);
  emit sizeHintChanged();
}

void AbstractNavigationWidget::updateIdealSize() const {
  if(d->m_context && !d->m_idealTextSize.isValid()) {
    QTextDocument doc;
    doc.setHtml(d->m_currentText);
    if(doc.idealWidth() > maxNavigationWidgetWidth) {
      doc.setTextWidth(maxNavigationWidgetWidth);
      d->m_idealTextSize.setWidth(maxNavigationWidgetWidth);
    }else{
      d->m_idealTextSize.setWidth(doc.idealWidth());
    }
    d->m_idealTextSize.setHeight(doc.size().height());
  }
}

void AbstractNavigationWidget::setDisplayHints(DisplayHints hints)
{
  d->m_hints = hints;
}

void AbstractNavigationWidget::update() {

  setUpdatesEnabled(false);
  Q_ASSERT( d->m_context );

  QString html;
  {
    DUChainReadLocker lock;
    html = d->m_context->html();
  }

  if(!html.isEmpty()) {
    int scrollPos = d->m_browser->verticalScrollBar()->value();

    if ( !(d->m_hints & EmbeddableWidget)) {
      // TODO: Only show that the first time, or the first few times this context is shown?
      html += QStringLiteral("<p><small>");
      if (d->m_context->linkCount() > 0) {
        html += i18n("(Hold <em>Alt</em> to show. Navigate via arrow keys, activate by pressing <em>Enter</em>)");
      } else {
        html += i18n("(Hold <em>Alt</em> to show this tooltip)");
      }
      html += QStringLiteral("</small></p>");
    }

    d->m_browser->setHtml( html );

    d->m_currentText = html;

    d->m_idealTextSize = QSize();

    QSize hint = sizeHint();
    if(hint.height() >= d->m_idealTextSize.height()) {
      d->m_browser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }else{
      d->m_browser->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }

    d->m_browser->verticalScrollBar()->setValue(scrollPos);
    d->m_browser->scrollToAnchor(QStringLiteral("currentPosition"));
    d->m_browser->show();
  }else{
    d->m_browser->hide();
  }

  if(d->m_currentWidget) {
    layout()->removeWidget(d->m_currentWidget);
    d->m_currentWidget->setParent(nullptr);
  }

  d->m_currentWidget = d->m_context->widget();

  d->m_browser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  d->m_browser->setMaximumHeight(10000);

  if(d->m_currentWidget) {
    const auto signalSignature = QMetaObject::normalizedSignature("navigateDeclaration(KDevelop::IndexedDeclaration)");
    if (d->m_currentWidget->metaObject()->indexOfSignal(signalSignature.constData()) != -1) {
      connect(d->m_currentWidget, SIGNAL(navigateDeclaration(KDevelop::IndexedDeclaration)),
              this, SLOT(navigateDeclaration(KDevelop::IndexedDeclaration)));
    }
    layout()->addWidget(d->m_currentWidget);
    if(d->m_context->isWidgetMaximized()) {
      //Leave unused room to the widget
      d->m_browser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
      d->m_browser->setMaximumHeight(d->m_idealTextSize.height());
    }
  }

  setUpdatesEnabled(true);
}

NavigationContextPointer AbstractNavigationWidget::context() {
  return d->m_context;
}

void AbstractNavigationWidget::navigateDeclaration(const IndexedDeclaration& decl)
{
  setContext(d->m_context->accept(decl));
}

void AbstractNavigationWidgetPrivate::anchorClicked(const QUrl& url)
{
  DUChainReadLocker lock;

  //We may get deleted while the call to acceptLink, so make sure we don't crash in that case
  QPointer<AbstractNavigationWidget> thisPtr(q);
  NavigationContextPointer nextContext = m_context->acceptLink(url.toString());

  if(thisPtr)
    q->setContext(nextContext);
}

void AbstractNavigationWidget::next() {
  Q_ASSERT( d->m_context );
  d->m_context->nextLink();
  update();
}

void AbstractNavigationWidget::previous() {
  Q_ASSERT( d->m_context );
  d->m_context->previousLink();
  update();
}

void AbstractNavigationWidget::accept() {
  Q_ASSERT( d->m_context );

  QPointer<AbstractNavigationWidget> thisPtr(this);
  NavigationContextPointer nextContext = d->m_context->accept();

  if(thisPtr)
    setContext( nextContext );
}

void AbstractNavigationWidget::back() {
  QPointer<AbstractNavigationWidget> thisPtr(this);
  NavigationContextPointer nextContext = d->m_context->back();

  if(thisPtr)
    setContext( nextContext );
}

void AbstractNavigationWidget::up() {
  d->m_context->up();
  update();
}

void AbstractNavigationWidget::down() {
  d->m_context->down();
  update();
}

void AbstractNavigationWidget::embeddedWidgetAccept() {
  accept();
}
void AbstractNavigationWidget::embeddedWidgetDown() {
  down();
}

void AbstractNavigationWidget::embeddedWidgetRight() {
  next();
}

void AbstractNavigationWidget::embeddedWidgetLeft() {
  previous();
}

void AbstractNavigationWidget::embeddedWidgetUp() {
  up();
}

void AbstractNavigationWidget::wheelEvent(QWheelEvent* event )
{
    QWidget::wheelEvent(event);
    event->accept();
}

}

#include "moc_abstractnavigationwidget.cpp"
