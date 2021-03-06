/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
 *                 2014 wandererfan <WandererFan@gmail.com>                *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <cmath>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsItem>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsTextItem>
#include <QPainterPathStroker>
#include <QPainter>
#include <QString>
#include <QTextOption>
#include <sstream>
#include <algorithm>    // std::find
#endif

#include <qmath.h>

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include <Mod/TechDraw/App/DrawViewClip.h>

#include "QGIViewClip.h"

using namespace TechDrawGui;

QGIViewClip::QGIViewClip()
{
    setHandlesChildEvents(false);
    setCacheMode(QGraphicsItem::NoCache);
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);

    addToGroup(&m_cliparea);
    m_cliparea.setPos(0.,0.);
    m_cliparea.setRect(0.,0.,5.,5.);

    addToGroup(&m_frame);
    m_frame.setPos(0.,0.);
    m_frame.setRect(0.,0.,5.,5.);
}


QVariant QGIViewClip::itemChange(GraphicsItemChange change, const QVariant &value)
{
    return QGIView::itemChange(change, value);
}

void QGIViewClip::updateView(bool update)
{
    if(getViewObject() == 0 || !getViewObject()->isDerivedFrom(TechDraw::DrawViewClip::getClassTypeId()))
        return;

    TechDraw::DrawViewClip *viewClip = dynamic_cast<TechDraw::DrawViewClip *>(getViewObject());

    if (update ||
        viewClip->isTouched() ||
        viewClip->Height.isTouched() ||
        viewClip->Width.isTouched() ||
        viewClip->ShowFrame.isTouched()) {

        draw();
    }

    QGIView::updateView(update);
}

void QGIViewClip::draw()
{
    if (!isVisible()) {
        return;
    }

    drawClip();
    if (borderVisible) {
        drawBorder();
    }
}

void QGIViewClip::drawClip()
{
    if(getViewObject() == 0 || !getViewObject()->isDerivedFrom(TechDraw::DrawViewClip::getClassTypeId()))
        return;

    TechDraw::DrawViewClip *viewClip = dynamic_cast<TechDraw::DrawViewClip *>(getViewObject());

    prepareGeometryChange();
    double h = viewClip->Height.getValue();
    double w = viewClip->Width.getValue();
    QRectF r = QRectF(0,0,w,h);
    m_frame.setRect(r);
    m_frame.setPos(0.,0.);
    if (viewClip->ShowFrame.getValue()) {
        m_frame.show();
    } else {
        m_frame.hide();
    }

    m_cliparea.setRect(r.adjusted(-1,-1,1,1));                        //TODO: clip just outside frame or just inside??

    std::vector<std::string> childNames = viewClip->getChildViewNames();
    //for all child Views in Clip, add the graphics representation of the View to the Clip group
    for(std::vector<std::string>::iterator it = childNames.begin(); it != childNames.end(); it++) {
        QGIView* qgiv = getQGIVByName((*it));
        if (qgiv) {
            //TODO: why is qgiv never already in a group?
            if (qgiv->group() != &m_cliparea) {
                qgiv->hide();
                scene()->removeItem(qgiv);
                m_cliparea.addToGroup(qgiv);
                qgiv->isInnerView(true);
                double x = qgiv->getViewObject()->X.getValue();
                double y = qgiv->getViewObject()->Y.getValue();
                qgiv->setPosition(x,y);
                if (viewClip->ShowLabels.getValue()) {
                    qgiv->toggleBorder(true);
                } else {
                    qgiv->toggleBorder(false);
                }
                qgiv->show();
            }
        } else {
            Base::Console().Warning("Logic error? - drawClip() - qgiv for %s not found\n",(*it).c_str());   //gview for feature !exist
        }
    }

    //for all graphic views in qgigroup, remove from qgigroup the ones that aren't in ViewClip
    QList<QGraphicsItem*> qgItems = m_cliparea.childItems();
    QList<QGraphicsItem*>::iterator it = qgItems.begin();
    for (; it != qgItems.end(); it++) {
        QGIView* qv = dynamic_cast<QGIView*>((*it));
        if (qv) {
            std::string qvName = std::string(qv->getViewName());
            if (std::find(childNames.begin(),childNames.end(),qvName) == childNames.end()) {
                m_cliparea.removeFromGroup(qv);
                removeFromGroup(qv);
                qv->isInnerView(false);
                qv->toggleBorder(true);
            }
        }
    }
}


