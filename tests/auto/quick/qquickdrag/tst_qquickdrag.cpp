/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QtTest/QSignalSpy>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlexpression.h>

template <typename T> static T evaluate(QObject *scope, const QString &expression)
{
    QQmlExpression expr(qmlContext(scope), scope, expression);
    QVariant result = expr.evaluate();
    if (expr.hasError())
        qWarning() << expr.error().toString();
    return result.value<T>();
}

template <> void evaluate<void>(QObject *scope, const QString &expression)
{
    QQmlExpression expr(qmlContext(scope), scope, expression);
    expr.evaluate();
    if (expr.hasError())
        qWarning() << expr.error().toString();
}

Q_DECLARE_METATYPE(Qt::DropActions)

class TestDropTarget : public QQuickItem
{
    Q_OBJECT
public:
    TestDropTarget(QQuickItem *parent = 0)
        : QQuickItem(parent)
        , enterEvents(0)
        , moveEvents(0)
        , leaveEvents(0)
        , dropEvents(0)
        , acceptAction(Qt::MoveAction)
        , defaultAction(Qt::IgnoreAction)
        , proposedAction(Qt::IgnoreAction)
        , accept(true)
    {
        setFlags(ItemAcceptsDrops);
    }

    void reset()
    {
        enterEvents = 0;
        moveEvents = 0;
        leaveEvents = 0;
        dropEvents = 0;
        defaultAction = Qt::IgnoreAction;
        proposedAction = Qt::IgnoreAction;
        supportedActions = Qt::IgnoreAction;
    }

    void dragEnterEvent(QDragEnterEvent *event)
    {
        ++enterEvents;
        position = event->pos();
        defaultAction = event->dropAction();
        proposedAction = event->proposedAction();
        supportedActions = event->possibleActions();
        event->setAccepted(accept);
    }

    void dragMoveEvent(QDragMoveEvent *event)
    {
        ++moveEvents;
        position = event->pos();
        defaultAction = event->dropAction();
        proposedAction = event->proposedAction();
        supportedActions = event->possibleActions();
        event->setAccepted(accept);
    }

    void dragLeaveEvent(QDragLeaveEvent *event)
    {
        ++leaveEvents;
        event->setAccepted(accept);
    }

    void dropEvent(QDropEvent *event)
    {
        ++dropEvents;
        position = event->pos();
        defaultAction = event->dropAction();
        proposedAction = event->proposedAction();
        supportedActions = event->possibleActions();
        event->setDropAction(acceptAction);
        event->setAccepted(accept);
    }

    int enterEvents;
    int moveEvents;
    int leaveEvents;
    int dropEvents;
    Qt::DropAction acceptAction;
    Qt::DropAction defaultAction;
    Qt::DropAction proposedAction;
    Qt::DropActions supportedActions;
    QPointF position;
    bool accept;
};

class tst_QQuickDrag: public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();

    void active();
    void drop();
    void move();
    void parentChange();
    void hotSpot();
    void supportedActions();
    void proposedAction();
    void keys();
    void source();
    void recursion_data();
    void recursion();

private:
    QQmlEngine engine;
};

void tst_QQuickDrag::initTestCase()
{

}

void tst_QQuickDrag::cleanupTestCase()
{

}

void tst_QQuickDrag::active()
{
    QQuickCanvas canvas;
    TestDropTarget dropTarget(canvas.rootItem());
    dropTarget.setSize(QSizeF(100, 100));
    QQmlComponent component(&engine);
    component.setData(
            "import QtQuick 2.0\n"
            "Item {\n"
                "property bool dragActive: Drag.active\n"
                "property Item dragTarget: Drag.target\n"
                "x: 50; y: 50\n"
                "width: 10; height: 10\n"
            "}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickItem *item = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(item);
    item->setParentItem(&dropTarget);

    QCOMPARE(evaluate<bool>(item, "Drag.active"), false);
    QCOMPARE(evaluate<bool>(item, "dragActive"), false);

    evaluate<void>(item, "Drag.active = true");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&dropTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&dropTarget));
    QCOMPARE(dropTarget.enterEvents, 1); QCOMPARE(dropTarget.leaveEvents, 0);

    dropTarget.reset();
    evaluate<void>(item, "Drag.active = false");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), false);
    QCOMPARE(evaluate<bool>(item, "dragActive"), false);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(dropTarget.enterEvents, 0); QCOMPARE(dropTarget.leaveEvents, 1);

    dropTarget.reset();
    evaluate<void>(item, "Drag.cancel()");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), false);
    QCOMPARE(evaluate<bool>(item, "dragActive"), false);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(dropTarget.enterEvents, 0); QCOMPARE(dropTarget.leaveEvents, 0);

    dropTarget.reset();
    evaluate<void>(item, "Drag.start()");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&dropTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&dropTarget));
    QCOMPARE(dropTarget.enterEvents, 1); QCOMPARE(dropTarget.leaveEvents, 0);

    // Start while a drag is active, cancels the previous drag and starts a new one.
    dropTarget.reset();
    evaluate<void>(item, "Drag.start()");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&dropTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&dropTarget));
    QCOMPARE(dropTarget.enterEvents, 1); QCOMPARE(dropTarget.leaveEvents, 1);

    dropTarget.reset();
    evaluate<void>(item, "Drag.cancel()");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), false);
    QCOMPARE(evaluate<bool>(item, "dragActive"), false);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(dropTarget.enterEvents, 0); QCOMPARE(dropTarget.leaveEvents, 1);

    // Enter events aren't sent to items without the QQuickItem::ItemAcceptsDrops flag.
    dropTarget.setFlags(QQuickItem::Flags());

    dropTarget.reset();
    evaluate<void>(item, "Drag.active = true");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(dropTarget.enterEvents, 0); QCOMPARE(dropTarget.leaveEvents, 0);

    dropTarget.reset();
    evaluate<void>(item, "Drag.active = false");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), false);
    QCOMPARE(evaluate<bool>(item, "dragActive"), false);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(dropTarget.enterEvents, 0); QCOMPARE(dropTarget.leaveEvents, 0);

    dropTarget.setFlags(QQuickItem::ItemAcceptsDrops);

    dropTarget.reset();
    evaluate<void>(item, "Drag.active = true");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&dropTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&dropTarget));
    QCOMPARE(dropTarget.enterEvents, 1); QCOMPARE(dropTarget.leaveEvents, 0);

    dropTarget.setFlags(QQuickItem::Flags());

    dropTarget.reset();
    evaluate<void>(item, "Drag.active = false");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), false);
    QCOMPARE(evaluate<bool>(item, "dragActive"), false);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(dropTarget.enterEvents, 0); QCOMPARE(dropTarget.leaveEvents, 1);

    // Follow up events aren't sent to items if the enter event isn't accepted.
    dropTarget.setFlags(QQuickItem::ItemAcceptsDrops);
    dropTarget.accept = false;

    dropTarget.reset();
    evaluate<void>(item, "Drag.active = true");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(dropTarget.enterEvents, 1); QCOMPARE(dropTarget.leaveEvents, 0);

    dropTarget.reset();
    evaluate<void>(item, "Drag.active = false");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), false);
    QCOMPARE(evaluate<bool>(item, "dragActive"), false);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(dropTarget.enterEvents, 0); QCOMPARE(dropTarget.leaveEvents, 0);

    dropTarget.accept = true;

    dropTarget.reset();
    evaluate<void>(item, "Drag.active = true");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&dropTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&dropTarget));
    QCOMPARE(dropTarget.enterEvents, 1); QCOMPARE(dropTarget.leaveEvents, 0);

    dropTarget.accept = false;

    dropTarget.reset();
    evaluate<void>(item, "Drag.active = false");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), false);
    QCOMPARE(evaluate<bool>(item, "dragActive"), false);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(dropTarget.enterEvents, 0); QCOMPARE(dropTarget.leaveEvents, 1);

    // Events are sent to hidden or disabled items.
    dropTarget.accept = true;
    dropTarget.setVisible(false);
    dropTarget.reset();
    evaluate<void>(item, "Drag.active = true");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(dropTarget.enterEvents, 0); QCOMPARE(dropTarget.leaveEvents, 0);

    evaluate<void>(item, "Drag.active = false");
    dropTarget.setVisible(true);

    dropTarget.setOpacity(0.0);
    dropTarget.reset();
    evaluate<void>(item, "Drag.active = true");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(dropTarget.enterEvents, 0); QCOMPARE(dropTarget.leaveEvents, 0);

    evaluate<void>(item, "Drag.active = false");
    dropTarget.setOpacity(1.0);

    dropTarget.setEnabled(false);
    dropTarget.reset();
    evaluate<void>(item, "Drag.active = true");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(dropTarget.enterEvents, 0); QCOMPARE(dropTarget.leaveEvents, 0);

    evaluate<void>(item, "Drag.active = false");
    dropTarget.setEnabled(true);
    dropTarget.reset();

    // Queued move events are discarded if the drag is cancelled.
    evaluate<void>(item, "Drag.active = true");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(dropTarget.enterEvents, 1); QCOMPARE(dropTarget.leaveEvents, 0); QCOMPARE(dropTarget.moveEvents, 0);

    dropTarget.reset();
    item->setPos(QPointF(80, 80));
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(dropTarget.enterEvents, 0); QCOMPARE(dropTarget.leaveEvents, 0); QCOMPARE(dropTarget.moveEvents, 0);

    evaluate<void>(item, "Drag.active = false");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), false);
    QCOMPARE(evaluate<bool>(item, "dragActive"), false);
    QCOMPARE(dropTarget.enterEvents, 0); QCOMPARE(dropTarget.leaveEvents, 1); QCOMPARE(dropTarget.moveEvents, 0);

    dropTarget.reset();
    QCoreApplication::processEvents();
    QCOMPARE(dropTarget.enterEvents, 0); QCOMPARE(dropTarget.leaveEvents, 0); QCOMPARE(dropTarget.moveEvents, 0);
}

void tst_QQuickDrag::drop()
{
    QQuickCanvas canvas;
    TestDropTarget outerTarget(canvas.rootItem());
    outerTarget.setSize(QSizeF(100, 100));
    outerTarget.acceptAction = Qt::CopyAction;
    TestDropTarget innerTarget(&outerTarget);
    innerTarget.setSize(QSizeF(100, 100));
    innerTarget.acceptAction = Qt::MoveAction;
    QQmlComponent component(&engine);
    component.setData(
            "import QtQuick 2.0\n"
            "Item {\n"
                "property bool dragActive: Drag.active\n"
                "property Item dragTarget: Drag.target\n"
                "x: 50; y: 50\n"
                "width: 10; height: 10\n"
            "}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickItem *item = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(item);
    item->setParentItem(&outerTarget);

    innerTarget.reset(); outerTarget.reset();
    evaluate<void>(item, "Drag.active = true");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&innerTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&innerTarget));
    QCOMPARE(outerTarget.enterEvents, 1); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.dropEvents, 0);
    QCOMPARE(innerTarget.enterEvents, 1); QCOMPARE(innerTarget.leaveEvents, 0); QCOMPARE(innerTarget.dropEvents, 0);

    innerTarget.reset(); outerTarget.reset();
    QCOMPARE(evaluate<bool>(item, "Drag.drop() == Qt.MoveAction"), true);
    QCOMPARE(evaluate<bool>(item, "Drag.active"), false);
    QCOMPARE(evaluate<bool>(item, "dragActive"), false);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&innerTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&innerTarget));
    QCOMPARE(outerTarget.enterEvents, 0); QCOMPARE(outerTarget.leaveEvents, 1); QCOMPARE(outerTarget.dropEvents, 0);
    QCOMPARE(innerTarget.enterEvents, 0); QCOMPARE(innerTarget.leaveEvents, 0); QCOMPARE(innerTarget.dropEvents, 1);

    innerTarget.reset(); outerTarget.reset();
    evaluate<void>(item, "Drag.active = true");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&innerTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&innerTarget));
    QCOMPARE(outerTarget.enterEvents, 1); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.dropEvents, 0);
    QCOMPARE(innerTarget.enterEvents, 1); QCOMPARE(innerTarget.leaveEvents, 0); QCOMPARE(innerTarget.dropEvents, 0);

    // Inner target declines the drop so it is propagated to the outer target.
    innerTarget.accept = false;

    innerTarget.reset(); outerTarget.reset();
    QCOMPARE(evaluate<bool>(item, "Drag.drop() == Qt.CopyAction"), true);
    QCOMPARE(evaluate<bool>(item, "Drag.active"), false);
    QCOMPARE(evaluate<bool>(item, "dragActive"), false);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(outerTarget.enterEvents, 0); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.dropEvents, 1);
    QCOMPARE(innerTarget.enterEvents, 0); QCOMPARE(innerTarget.leaveEvents, 0); QCOMPARE(innerTarget.dropEvents, 1);


    // Inner target doesn't accept enter so drop goes directly to outer.
    innerTarget.accept = true;
    innerTarget.setFlags(QQuickItem::Flags());

    innerTarget.reset(); outerTarget.reset();
    evaluate<void>(item, "Drag.active = true");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(outerTarget.enterEvents, 1); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.dropEvents, 0);
    QCOMPARE(innerTarget.enterEvents, 0); QCOMPARE(innerTarget.leaveEvents, 0); QCOMPARE(innerTarget.dropEvents, 0);

    innerTarget.reset(); outerTarget.reset();
    QCOMPARE(evaluate<bool>(item, "Drag.drop() == Qt.CopyAction"), true);
    QCOMPARE(evaluate<bool>(item, "Drag.active"), false);
    QCOMPARE(evaluate<bool>(item, "dragActive"), false);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(outerTarget.enterEvents, 0); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.dropEvents, 1);
    QCOMPARE(innerTarget.enterEvents, 0); QCOMPARE(innerTarget.leaveEvents, 0); QCOMPARE(innerTarget.dropEvents, 0);

    // Neither target accepts drop so Qt::IgnoreAction is returned.
    innerTarget.reset(); outerTarget.reset();
    evaluate<void>(item, "Drag.active = true");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(outerTarget.enterEvents, 1); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.dropEvents, 0);
    QCOMPARE(innerTarget.enterEvents, 0); QCOMPARE(innerTarget.leaveEvents, 0); QCOMPARE(innerTarget.dropEvents, 0);

    outerTarget.accept = false;

    innerTarget.reset(); outerTarget.reset();
    QCOMPARE(evaluate<bool>(item, "Drag.drop() == Qt.IgnoreAction"), true);
    QCOMPARE(evaluate<bool>(item, "Drag.active"), false);
    QCOMPARE(evaluate<bool>(item, "dragActive"), false);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(outerTarget.enterEvents, 0); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.dropEvents, 1);
    QCOMPARE(innerTarget.enterEvents, 0); QCOMPARE(innerTarget.leaveEvents, 0); QCOMPARE(innerTarget.dropEvents, 0);

    // drop doesn't send an event and returns Qt.IgnoreAction if not active.
    innerTarget.accept = true;
    outerTarget.accept = true;
    innerTarget.reset(); outerTarget.reset();
    QCOMPARE(evaluate<bool>(item, "Drag.drop() == Qt.IgnoreAction"), true);
    QCOMPARE(evaluate<bool>(item, "Drag.active"), false);
    QCOMPARE(evaluate<bool>(item, "dragActive"), false);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(outerTarget.enterEvents, 0); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.dropEvents, 0);
    QCOMPARE(innerTarget.enterEvents, 0); QCOMPARE(innerTarget.leaveEvents, 0); QCOMPARE(innerTarget.dropEvents, 0);

    // Queued move event is delivered before a drop event.
    innerTarget.reset(); outerTarget.reset();
    evaluate<void>(item, "Drag.active = true");
    item->setPos(QPointF(80, 80));
    evaluate<void>(item, "Drag.drop()");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), false);
    QCOMPARE(evaluate<bool>(item, "dragActive"), false);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(outerTarget.enterEvents, 1); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.dropEvents, 1); QCOMPARE(outerTarget.moveEvents, 1);
    QCOMPARE(innerTarget.enterEvents, 0); QCOMPARE(innerTarget.leaveEvents, 0); QCOMPARE(innerTarget.dropEvents, 0); QCOMPARE(innerTarget.moveEvents, 0);

    innerTarget.reset(); outerTarget.reset();
    QCoreApplication::processEvents();
    QCOMPARE(outerTarget.enterEvents, 0); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.dropEvents, 0); QCOMPARE(outerTarget.moveEvents, 0);
    QCOMPARE(innerTarget.enterEvents, 0); QCOMPARE(innerTarget.leaveEvents, 0); QCOMPARE(innerTarget.dropEvents, 0); QCOMPARE(innerTarget.moveEvents, 0);
}

void tst_QQuickDrag::move()
{
    QQuickCanvas canvas;
    TestDropTarget outerTarget(canvas.rootItem());
    outerTarget.setSize(QSizeF(100, 100));
    TestDropTarget leftTarget(&outerTarget);
    leftTarget.setPos(QPointF(0, 35));
    leftTarget.setSize(QSizeF(30, 30));
    TestDropTarget rightTarget(&outerTarget);
    rightTarget.setPos(QPointF(70, 35));
    rightTarget.setSize(QSizeF(30, 30));
    QQmlComponent component(&engine);
    component.setData(
            "import QtQuick 2.0\n"
            "Item {\n"
                "property bool dragActive: Drag.active\n"
                "property Item dragTarget: Drag.target\n"
                "x: 50; y: 50\n"
                "width: 10; height: 10\n"
            "}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickItem *item = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(item);
    item->setParentItem(&outerTarget);

    evaluate<void>(item, "Drag.active = true");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(outerTarget.enterEvents, 1); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.moveEvents, 0);
    QCOMPARE(leftTarget .enterEvents, 0); QCOMPARE(leftTarget .leaveEvents, 0); QCOMPARE(leftTarget .moveEvents, 0);
    QCOMPARE(rightTarget.enterEvents, 0); QCOMPARE(rightTarget.leaveEvents, 0); QCOMPARE(rightTarget.moveEvents, 0);
    QCOMPARE(outerTarget.position.x(), qreal(50)); QCOMPARE(outerTarget.position.y(), qreal(50));

    // Move within the outer target.
    outerTarget.reset(); leftTarget.reset(); rightTarget.reset();
    item->setPos(QPointF(60, 50));
    // Move event is delivered in the event loop.
    QCOMPARE(outerTarget.enterEvents, 0); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.moveEvents, 0);
    QCOMPARE(leftTarget .enterEvents, 0); QCOMPARE(leftTarget .leaveEvents, 0); QCOMPARE(leftTarget .moveEvents, 0);
    QCOMPARE(rightTarget.enterEvents, 0); QCOMPARE(rightTarget.leaveEvents, 0); QCOMPARE(rightTarget.moveEvents, 0);
    QCoreApplication::processEvents();
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(outerTarget.enterEvents, 0); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.moveEvents, 1);
    QCOMPARE(leftTarget .enterEvents, 0); QCOMPARE(leftTarget .leaveEvents, 0); QCOMPARE(leftTarget .moveEvents, 0);
    QCOMPARE(rightTarget.enterEvents, 0); QCOMPARE(rightTarget.leaveEvents, 0); QCOMPARE(rightTarget.moveEvents, 0);
    QCOMPARE(outerTarget.position.x(), qreal(60)); QCOMPARE(outerTarget.position.y(), qreal(50));

    // Move into the right target.
    outerTarget.reset(); leftTarget.reset(); rightTarget.reset();
    // Setting X and Y individually should still only generate on move.
    item->setX(75);
    item->setY(50);
    QCOMPARE(outerTarget.enterEvents, 0); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.moveEvents, 0);
    QCOMPARE(leftTarget .enterEvents, 0); QCOMPARE(leftTarget .leaveEvents, 0); QCOMPARE(leftTarget .moveEvents, 0);
    QCOMPARE(rightTarget.enterEvents, 0); QCOMPARE(rightTarget.leaveEvents, 0); QCOMPARE(rightTarget.moveEvents, 0);
    QCoreApplication::processEvents();
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&rightTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&rightTarget));
    QCOMPARE(outerTarget.enterEvents, 0); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.moveEvents, 1);
    QCOMPARE(leftTarget .enterEvents, 0); QCOMPARE(leftTarget .leaveEvents, 0); QCOMPARE(leftTarget .moveEvents, 0);
    QCOMPARE(rightTarget.enterEvents, 1); QCOMPARE(rightTarget.leaveEvents, 0); QCOMPARE(rightTarget.moveEvents, 0);
    QCOMPARE(outerTarget.position.x(), qreal(75)); QCOMPARE(outerTarget.position.y(), qreal(50));
    QCOMPARE(rightTarget.position.x(), qreal(5)); QCOMPARE(rightTarget.position.y(), qreal(15));

    // Move into the left target.
    outerTarget.reset(); leftTarget.reset(); rightTarget.reset();
    item->setPos(QPointF(25, 50));
    QCoreApplication::processEvents();
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&leftTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&leftTarget));
    QCOMPARE(outerTarget.enterEvents, 0); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.moveEvents, 1);
    QCOMPARE(leftTarget .enterEvents, 1); QCOMPARE(leftTarget .leaveEvents, 0); QCOMPARE(leftTarget .moveEvents, 0);
    QCOMPARE(rightTarget.enterEvents, 0); QCOMPARE(rightTarget.leaveEvents, 1); QCOMPARE(rightTarget.moveEvents, 0);
    QCOMPARE(outerTarget.position.x(), qreal(25)); QCOMPARE(outerTarget.position.y(), qreal(50));
    QCOMPARE(leftTarget.position.x(), qreal(25)); QCOMPARE(leftTarget.position.y(), qreal(15));

    // Move within the left target.
    outerTarget.reset(); leftTarget.reset(); rightTarget.reset();
    item->setPos(QPointF(25, 40));
    QCoreApplication::processEvents();
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&leftTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&leftTarget));
    QCOMPARE(outerTarget.enterEvents, 0); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.moveEvents, 1);
    QCOMPARE(leftTarget .enterEvents, 0); QCOMPARE(leftTarget .leaveEvents, 0); QCOMPARE(leftTarget .moveEvents, 1);
    QCOMPARE(rightTarget.enterEvents, 0); QCOMPARE(rightTarget.leaveEvents, 0); QCOMPARE(rightTarget.moveEvents, 0);
    QCOMPARE(outerTarget.position.x(), qreal(25)); QCOMPARE(outerTarget.position.y(), qreal(40));
    QCOMPARE(leftTarget.position.x(), qreal(25)); QCOMPARE(leftTarget.position.y(), qreal(5));

    // Move out of all targets.
    outerTarget.reset(); leftTarget.reset(); rightTarget.reset();
    item->setPos(QPointF(110, 50));
    QCoreApplication::processEvents();
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(outerTarget.enterEvents, 0); QCOMPARE(outerTarget.leaveEvents, 1); QCOMPARE(outerTarget.moveEvents, 0);
    QCOMPARE(leftTarget .enterEvents, 0); QCOMPARE(leftTarget .leaveEvents, 1); QCOMPARE(leftTarget .moveEvents, 0);
    QCOMPARE(rightTarget.enterEvents, 0); QCOMPARE(rightTarget.leaveEvents, 0); QCOMPARE(rightTarget.moveEvents, 0);

    // Stop the right target accepting drag events and move into it.
    rightTarget.accept = false;

    outerTarget.reset(); leftTarget.reset(); rightTarget.reset();
    item->setPos(QPointF(80, 50));
    QCoreApplication::processEvents();
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(outerTarget.enterEvents, 1); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.moveEvents, 0);
    QCOMPARE(leftTarget .enterEvents, 0); QCOMPARE(leftTarget .leaveEvents, 0); QCOMPARE(leftTarget .moveEvents, 0);
    QCOMPARE(rightTarget.enterEvents, 1); QCOMPARE(rightTarget.leaveEvents, 0); QCOMPARE(rightTarget.moveEvents, 0);
    QCOMPARE(outerTarget.position.x(), qreal(80)); QCOMPARE(outerTarget.position.y(), qreal(50));

    // Stop the outer target accepting drag events after it has accepted an enter event.
    outerTarget.accept = false;

    outerTarget.reset(); leftTarget.reset(); rightTarget.reset();
    item->setPos(QPointF(60, 50));
    QCoreApplication::processEvents();
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(outerTarget.enterEvents, 0); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.moveEvents, 1);
    QCOMPARE(leftTarget .enterEvents, 0); QCOMPARE(leftTarget .leaveEvents, 0); QCOMPARE(leftTarget .moveEvents, 0);
    QCOMPARE(rightTarget.enterEvents, 0); QCOMPARE(rightTarget.leaveEvents, 0); QCOMPARE(rightTarget.moveEvents, 0);
    QCOMPARE(outerTarget.position.x(), qreal(60)); QCOMPARE(outerTarget.position.y(), qreal(50));

    // Clear the QQuickItem::ItemAcceptsDrops flag from the outer target after it accepted an enter event.
    outerTarget.setFlags(QQuickItem::Flags());

    outerTarget.reset(); leftTarget.reset(); rightTarget.reset();
    item->setPos(QPointF(40, 50));
    QCoreApplication::processEvents();
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(outerTarget.enterEvents, 0); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.moveEvents, 1);
    QCOMPARE(leftTarget .enterEvents, 0); QCOMPARE(leftTarget .leaveEvents, 0); QCOMPARE(leftTarget .moveEvents, 0);
    QCOMPARE(rightTarget.enterEvents, 0); QCOMPARE(rightTarget.leaveEvents, 0); QCOMPARE(rightTarget.moveEvents, 0);
    QCOMPARE(outerTarget.position.x(), qreal(40)); QCOMPARE(outerTarget.position.y(), qreal(50));

    // Clear the QQuickItem::ItemAcceptsDrops flag from the left target before it accepts an enter event.
    leftTarget.setFlags(QQuickItem::Flags());

    outerTarget.reset(); leftTarget.reset(); rightTarget.reset();
    item->setPos(QPointF(25, 50));
    QCoreApplication::processEvents();
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(outerTarget.enterEvents, 0); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.moveEvents, 1);
    QCOMPARE(leftTarget .enterEvents, 0); QCOMPARE(leftTarget .leaveEvents, 0); QCOMPARE(leftTarget .moveEvents, 0);
    QCOMPARE(rightTarget.enterEvents, 0); QCOMPARE(rightTarget.leaveEvents, 0); QCOMPARE(rightTarget.moveEvents, 0);
    QCOMPARE(outerTarget.position.x(), qreal(25)); QCOMPARE(outerTarget.position.y(), qreal(50));
}

void tst_QQuickDrag::parentChange()
{
    QQuickCanvas canvas1;
    TestDropTarget dropTarget1(canvas1.rootItem());
    dropTarget1.setSize(QSizeF(100, 100));

    QQuickCanvas canvas2;
    TestDropTarget dropTarget2(canvas2.rootItem());
    dropTarget2.setSize(QSizeF(100, 100));

    QQmlComponent component(&engine);
    component.setData(
            "import QtQuick 2.0\n"
            "Item {\n"
                "property real hotSpotX: Drag.hotSpot.x\n"
                "property real hotSpotY: Drag.hotSpot.y\n"
                "x: 50; y: 50\n"
                "width: 10; height: 10\n"
                "Drag.active: true\n"
            "}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickItem *item = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(item);

    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);

    // Verify setting a parent item for an item with an active drag sends an enter event.
    item->setParentItem(canvas1.rootItem());
    QCOMPARE(dropTarget1.enterEvents, 0);
    QCoreApplication::processEvents();
    QCOMPARE(dropTarget1.enterEvents, 1);

    // Changing the parent within the same canvas should send a move event.
    item->setParentItem(&dropTarget1);
    QCOMPARE(dropTarget1.enterEvents, 1);
    QCOMPARE(dropTarget1.moveEvents, 0);
    QCoreApplication::processEvents();
    QCOMPARE(dropTarget1.enterEvents, 1);
    QCOMPARE(dropTarget1.moveEvents, 1);

    // Changing the parent to an item in another canvas sends a leave event in the old canvas
    // and an enter on the new canvas.
    item->setParentItem(canvas2.rootItem());
    QCOMPARE(dropTarget1.enterEvents, 1);
    QCOMPARE(dropTarget1.moveEvents, 1);
    QCOMPARE(dropTarget1.leaveEvents, 0);
    QCOMPARE(dropTarget2.enterEvents, 0);
    QCoreApplication::processEvents();
    QCOMPARE(dropTarget1.enterEvents, 1);
    QCOMPARE(dropTarget1.moveEvents, 1);
    QCOMPARE(dropTarget1.leaveEvents, 1);
    QCOMPARE(dropTarget2.enterEvents, 1);

    // Removing then parent item sends a leave event.
    item->setParentItem(0);
    QCOMPARE(dropTarget1.enterEvents, 1);
    QCOMPARE(dropTarget1.moveEvents, 1);
    QCOMPARE(dropTarget1.leaveEvents, 1);
    QCOMPARE(dropTarget2.enterEvents, 1);
    QCOMPARE(dropTarget2.leaveEvents, 0);
    QCoreApplication::processEvents();
    QCOMPARE(dropTarget1.enterEvents, 1);
    QCOMPARE(dropTarget1.moveEvents, 1);
    QCOMPARE(dropTarget1.leaveEvents, 1);
    QCOMPARE(dropTarget2.enterEvents, 1);
    QCOMPARE(dropTarget2.leaveEvents, 1);

    // Go around again and verify no events if active is false.
    evaluate<void>(item, "Drag.active = false");
    item->setParentItem(canvas1.rootItem());
    QCoreApplication::processEvents();

    item->setParentItem(&dropTarget1);
    QCoreApplication::processEvents();

    item->setParentItem(canvas2.rootItem());
    QCoreApplication::processEvents();

    item->setParentItem(0);
    QCoreApplication::processEvents();
    QCOMPARE(dropTarget1.enterEvents, 1);
    QCOMPARE(dropTarget1.moveEvents, 1);
    QCOMPARE(dropTarget1.leaveEvents, 1);
    QCOMPARE(dropTarget2.enterEvents, 1);
    QCOMPARE(dropTarget2.leaveEvents, 1);
}

void tst_QQuickDrag::hotSpot()
{
    QQuickCanvas canvas;
    TestDropTarget dropTarget(canvas.rootItem());
    dropTarget.setSize(QSizeF(100, 100));
    QQmlComponent component(&engine);
    component.setData(
            "import QtQuick 2.0\n"
            "Item {\n"
                "property real hotSpotX: Drag.hotSpot.x\n"
                "property real hotSpotY: Drag.hotSpot.y\n"
                "x: 50; y: 50\n"
                "width: 10; height: 10\n"
            "}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickItem *item = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(item);
    item->setParentItem(&dropTarget);

    QCOMPARE(evaluate<qreal>(item, "Drag.hotSpot.x"), qreal(0));
    QCOMPARE(evaluate<qreal>(item, "Drag.hotSpot.y"), qreal(0));
    QCOMPARE(evaluate<qreal>(item, "hotSpotX"), qreal(0));
    QCOMPARE(evaluate<qreal>(item, "hotSpotY"), qreal(0));

    evaluate<void>(item, "{ Drag.start(); Drag.cancel() }");
    QCOMPARE(dropTarget.position.x(), qreal(50));
    QCOMPARE(dropTarget.position.y(), qreal(50));

    evaluate<void>(item, "{ Drag.hotSpot.x = 5, Drag.hotSpot.y = 5 }");
    QCOMPARE(evaluate<qreal>(item, "Drag.hotSpot.x"), qreal(5));
    QCOMPARE(evaluate<qreal>(item, "Drag.hotSpot.y"), qreal(5));
    QCOMPARE(evaluate<qreal>(item, "hotSpotX"), qreal(5));
    QCOMPARE(evaluate<qreal>(item, "hotSpotY"), qreal(5));

    evaluate<void>(item, "Drag.start()");
    QCOMPARE(dropTarget.position.x(), qreal(55));
    QCOMPARE(dropTarget.position.y(), qreal(55));

    item->setPos(QPointF(30, 20));
    QCoreApplication::processEvents();
    QCOMPARE(dropTarget.position.x(), qreal(35));
    QCOMPARE(dropTarget.position.y(), qreal(25));

    evaluate<void>(item, "{ Drag.hotSpot.x = 10; Drag.hotSpot.y = 10 }");
    QCOMPARE(evaluate<qreal>(item, "Drag.hotSpot.x"), qreal(10));
    QCOMPARE(evaluate<qreal>(item, "Drag.hotSpot.y"), qreal(10));
    QCOMPARE(evaluate<qreal>(item, "hotSpotX"), qreal(10));
    QCOMPARE(evaluate<qreal>(item, "hotSpotY"), qreal(10));

    // Setting the hotSpot will deliver a move event in the event loop.
    QCOMPARE(dropTarget.position.x(), qreal(35));
    QCOMPARE(dropTarget.position.y(), qreal(25));
    QCoreApplication::processEvents();
    QCOMPARE(dropTarget.position.x(), qreal(40));
    QCOMPARE(dropTarget.position.y(), qreal(30));

    item->setPos(QPointF(10, 20));
    QCoreApplication::processEvents();
    QCOMPARE(dropTarget.position.x(), qreal(20));
    QCOMPARE(dropTarget.position.y(), qreal(30));

    evaluate<void>(item, "{ Drag.hotSpot.x = 10; Drag.hotSpot.y = 10 }");
}

void tst_QQuickDrag::supportedActions()
{
    QQuickCanvas canvas;
    TestDropTarget dropTarget(canvas.rootItem());
    dropTarget.setSize(QSizeF(100, 100));
    QQmlComponent component(&engine);
    component.setData(
            "import QtQuick 2.0\n"
            "Item {\n"
                "property int supportedActions: Drag.supportedActions\n"
                "x: 50; y: 50\n"
                "width: 10; height: 10\n"
            "}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickItem *item = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(item);
    item->setParentItem(&dropTarget);

    QCOMPARE(evaluate<bool>(item, "Drag.supportedActions == Qt.CopyAction | Qt.MoveAction | Qt.LinkAction"), true);
    QCOMPARE(evaluate<bool>(item, "supportedActions == Qt.CopyAction | Qt.MoveAction | Qt.LinkAction"), true);
    evaluate<void>(item, "{ Drag.start(); Drag.cancel() }");
    QCOMPARE(dropTarget.supportedActions, Qt::CopyAction | Qt::MoveAction | Qt::LinkAction);

    dropTarget.reset();
    evaluate<void>(item, "Drag.supportedActions = Qt.CopyAction | Qt.MoveAction");
    QCOMPARE(evaluate<bool>(item, "Drag.supportedActions == Qt.CopyAction | Qt.MoveAction"), true);
    QCOMPARE(evaluate<bool>(item, "supportedActions == Qt.CopyAction | Qt.MoveAction"), true);
    evaluate<void>(item, "Drag.start()");
    QCOMPARE(dropTarget.supportedActions, Qt::CopyAction | Qt::MoveAction);
    QCOMPARE(dropTarget.leaveEvents, 0);
    QCOMPARE(dropTarget.enterEvents, 1);

    // Changing the supported actions will restart the drag, after a delay to avoid any
    // recursion.
    evaluate<void>(item, "Drag.supportedActions = Qt.MoveAction");
    QCOMPARE(evaluate<bool>(item, "Drag.supportedActions == Qt.MoveAction"), true);
    QCOMPARE(evaluate<bool>(item, "supportedActions == Qt.MoveAction"), true);
    item->setPos(QPointF(60, 60));
    QCOMPARE(dropTarget.supportedActions, Qt::CopyAction | Qt::MoveAction);
    QCOMPARE(dropTarget.leaveEvents, 0);
    QCOMPARE(dropTarget.enterEvents, 1);
    QCoreApplication::processEvents();
    QCOMPARE(dropTarget.supportedActions, Qt::MoveAction);
    QCOMPARE(dropTarget.leaveEvents, 1);
    QCOMPARE(dropTarget.enterEvents, 2);

    // Calling start with proposed actions will override the current actions for the next sequence.
    evaluate<void>(item, "Drag.start(Qt.CopyAction)");
    QCOMPARE(evaluate<bool>(item, "Drag.supportedActions == Qt.MoveAction"), true);
    QCOMPARE(evaluate<bool>(item, "supportedActions == Qt.MoveAction"), true);
    QCOMPARE(dropTarget.supportedActions, Qt::CopyAction);

    evaluate<void>(item, "Drag.start()");
    QCOMPARE(evaluate<bool>(item, "Drag.supportedActions == Qt.MoveAction"), true);
    QCOMPARE(evaluate<bool>(item, "supportedActions == Qt.MoveAction"), true);
    QCOMPARE(dropTarget.supportedActions, Qt::MoveAction);
}

void tst_QQuickDrag::proposedAction()
{
    QQuickCanvas canvas;
    TestDropTarget dropTarget(canvas.rootItem());
    dropTarget.setSize(QSizeF(100, 100));
    QQmlComponent component(&engine);
    component.setData(
            "import QtQuick 2.0\n"
            "Item {\n"
                "property int proposedAction: Drag.proposedAction\n"
                "x: 50; y: 50\n"
                "width: 10; height: 10\n"
            "}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickItem *item = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(item);
    item->setParentItem(&dropTarget);

    QCOMPARE(evaluate<bool>(item, "Drag.proposedAction == Qt.MoveAction"), true);
    QCOMPARE(evaluate<bool>(item, "proposedAction == Qt.MoveAction"), true);
    evaluate<void>(item, "{ Drag.start(); Drag.cancel() }");
    QCOMPARE(dropTarget.defaultAction, Qt::MoveAction);
    QCOMPARE(dropTarget.proposedAction, Qt::MoveAction);

    evaluate<void>(item, "Drag.proposedAction = Qt.CopyAction");
    QCOMPARE(evaluate<bool>(item, "Drag.proposedAction == Qt.CopyAction"), true);
    QCOMPARE(evaluate<bool>(item, "proposedAction == Qt.CopyAction"), true);
    evaluate<void>(item, "Drag.start()");
    QCOMPARE(dropTarget.defaultAction, Qt::CopyAction);
    QCOMPARE(dropTarget.proposedAction, Qt::CopyAction);

    // The proposed action can change during a drag.
    evaluate<void>(item, "Drag.proposedAction = Qt.MoveAction");
    QCOMPARE(evaluate<bool>(item, "Drag.proposedAction == Qt.MoveAction"), true);
    QCOMPARE(evaluate<bool>(item, "proposedAction == Qt.MoveAction"), true);
    QCoreApplication::processEvents();
    QCOMPARE(dropTarget.defaultAction, Qt::MoveAction);
    QCOMPARE(dropTarget.proposedAction, Qt::MoveAction);

    evaluate<void>(item, "Drag.proposedAction = Qt.LinkAction");
    QCOMPARE(evaluate<bool>(item, "Drag.proposedAction == Qt.LinkAction"), true);
    QCOMPARE(evaluate<bool>(item, "proposedAction == Qt.LinkAction"), true);
    evaluate<void>(item, "Drag.drop()");
    QCOMPARE(dropTarget.defaultAction, Qt::LinkAction);
    QCOMPARE(dropTarget.proposedAction, Qt::LinkAction);
}

void tst_QQuickDrag::keys()
{
    QQmlComponent component(&engine);
    component.setData(
            "import QtQuick 2.0\n"
            "Item {\n"
                "property variant keys: Drag.keys\n"
                "x: 50; y: 50\n"
                "width: 10; height: 10\n"
            "}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickItem *item = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(item);

    QCOMPARE(evaluate<QStringList>(item, "Drag.keys"), QStringList());
    QCOMPARE(evaluate<QStringList>(item, "keys"), QStringList());
    QCOMPARE(item->property("keys").toStringList(), QStringList());

    evaluate<void>(item, "Drag.keys = [\"red\", \"blue\"]");
    QCOMPARE(evaluate<QStringList>(item, "Drag.keys"), QStringList() << "red" << "blue");
    QCOMPARE(evaluate<QStringList>(item, "keys"), QStringList() << "red" << "blue");
    QCOMPARE(item->property("keys").toStringList(), QStringList() << "red" << "blue");

    // Test changing the keys restarts a drag.
    QQuickCanvas canvas;
    item->setParentItem(canvas.rootItem());
    TestDropTarget dropTarget(canvas.rootItem());
    dropTarget.setSize(QSizeF(100, 100));

    evaluate<void>(item, "Drag.start()");
    QCOMPARE(dropTarget.leaveEvents, 0);
    QCOMPARE(dropTarget.enterEvents, 1);

    evaluate<void>(item, "Drag.keys = [\"green\"]");
    QCOMPARE(dropTarget.leaveEvents, 0);
    QCOMPARE(dropTarget.enterEvents, 1);
    QCoreApplication::processEvents();
    QCOMPARE(dropTarget.leaveEvents, 1);
    QCOMPARE(dropTarget.enterEvents, 2);
}

void tst_QQuickDrag::source()
{

    QQmlComponent component(&engine);
    component.setData(
            "import QtQuick 2.0\n"
            "Item {\n"
                "property Item source: Drag.source\n"
                "x: 50; y: 50\n"
                "width: 10; height: 10\n"
                "Item { id: proxySource; objectName: \"proxySource\" }\n"
            "}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickItem *item = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(item);

    QCOMPARE(evaluate<QObject *>(item, "Drag.source"), static_cast<QObject *>(item));
    QCOMPARE(evaluate<QObject *>(item, "source"), static_cast<QObject *>(item));

    QQuickItem *proxySource = item->findChild<QQuickItem *>("proxySource");
    QVERIFY(proxySource);

    evaluate<void>(item, "Drag.source = proxySource");
    QCOMPARE(evaluate<QObject *>(item, "Drag.source"), static_cast<QObject *>(proxySource));
    QCOMPARE(evaluate<QObject *>(item, "source"), static_cast<QObject *>(proxySource));

    evaluate<void>(item, "Drag.source = undefined");
    QCOMPARE(evaluate<QObject *>(item, "Drag.source"), static_cast<QObject *>(item));
    QCOMPARE(evaluate<QObject *>(item, "source"), static_cast<QObject *>(item));

    // Test changing the source restarts a drag.
    QQuickCanvas canvas;
    item->setParentItem(canvas.rootItem());
    TestDropTarget dropTarget(canvas.rootItem());
    dropTarget.setSize(QSizeF(100, 100));

    evaluate<void>(item, "Drag.start()");
    QCOMPARE(dropTarget.leaveEvents, 0);
    QCOMPARE(dropTarget.enterEvents, 1);

    evaluate<void>(item, "Drag.source = proxySource");
    QCOMPARE(dropTarget.leaveEvents, 0);
    QCOMPARE(dropTarget.enterEvents, 1);
    QCoreApplication::processEvents();
    QCOMPARE(dropTarget.leaveEvents, 1);
    QCOMPARE(dropTarget.enterEvents, 2);
}

class RecursingDropTarget : public TestDropTarget
{
public:
    RecursingDropTarget(const QString &script, int type, QQuickItem *parent)
        : TestDropTarget(parent), script(script), type(type), item(0) {}

    void setItem(QQuickItem *i) { item = i; }

protected:
    void dragEnterEvent(QDragEnterEvent *event)
    {
        TestDropTarget::dragEnterEvent(event);
        if (type == QEvent::DragEnter && enterEvents < 2)
            evaluate<void>(item, script);
    }

    void dragMoveEvent(QDragMoveEvent *event)
    {
        TestDropTarget::dragMoveEvent(event);
        if (type == QEvent::DragMove && moveEvents < 2)
            evaluate<void>(item, script);
    }

    void dragLeaveEvent(QDragLeaveEvent *event)
    {
        TestDropTarget::dragLeaveEvent(event);
        if (type == QEvent::DragLeave && leaveEvents < 2)
            evaluate<void>(item, script);
    }

    void dropEvent(QDropEvent *event)
    {
        TestDropTarget::dropEvent(event);
        if (type == QEvent::Drop && dropEvents < 2)
            evaluate<void>(item, script);
    }

private:
    QString script;
    int type;
    QQuickItem *item;

};

void tst_QQuickDrag::recursion_data()
{
    QTest::addColumn<QString>("script");
    QTest::addColumn<int>("type");
    QTest::addColumn<QByteArray>("warning");

    QTest::newRow("Drag.start() in Enter")
            << QString("Drag.start()")
            << int(QEvent::DragEnter)
            << QByteArray("<Unknown File>: QML QQuickDragAttached: start() cannot be called from within a drag event handler");
    QTest::newRow("Drag.cancel() in Enter")
            << QString("Drag.cancel()")
            << int(QEvent::DragEnter)
            << QByteArray("<Unknown File>: QML QQuickDragAttached: cancel() cannot be called from within a drag event handler");
    QTest::newRow("Drag.drop() in Enter")
            << QString("Drag.drop()")
            << int(QEvent::DragEnter)
            << QByteArray("<Unknown File>: QML QQuickDragAttached: drop() cannot be called from within a drag event handler");
    QTest::newRow("Drag.active = true in Enter")
            << QString("Drag.active = true")
            << int(QEvent::DragEnter)
            << QByteArray();
    QTest::newRow("Drag.active = false in Enter")
            << QString("Drag.active = false")
            << int(QEvent::DragEnter)
            << QByteArray("<Unknown File>: QML QQuickDragAttached: active cannot be changed from within a drag event handler");
    QTest::newRow("move in Enter")
            << QString("x = 23")
            << int(QEvent::DragEnter)
            << QByteArray();

    QTest::newRow("Drag.start() in Move")
            << QString("Drag.start()")
            << int(QEvent::DragMove)
            << QByteArray("<Unknown File>: QML QQuickDragAttached: start() cannot be called from within a drag event handler");
    QTest::newRow("Drag.cancel() in Move")
            << QString("Drag.cancel()")
            << int(QEvent::DragMove)
            << QByteArray("<Unknown File>: QML QQuickDragAttached: cancel() cannot be called from within a drag event handler");
    QTest::newRow("Drag.drop() in Move")
            << QString("Drag.drop()")
            << int(QEvent::DragMove)
            << QByteArray("<Unknown File>: QML QQuickDragAttached: drop() cannot be called from within a drag event handler");
    QTest::newRow("Drag.active = true in Move")
            << QString("Drag.active = true")
            << int(QEvent::DragMove)
            << QByteArray();
    QTest::newRow("Drag.active = false in Move")
            << QString("Drag.active = false")
            << int(QEvent::DragMove)
            << QByteArray("<Unknown File>: QML QQuickDragAttached: active cannot be changed from within a drag event handler");
    QTest::newRow("move in Move")
            << QString("x = 23")
            << int(QEvent::DragMove)
            << QByteArray();

    QTest::newRow("Drag.start() in Leave")
            << QString("Drag.start()")
            << int(QEvent::DragLeave)
            << QByteArray("<Unknown File>: QML QQuickDragAttached: start() cannot be called from within a drag event handler");
    QTest::newRow("Drag.cancel() in Leave")
            << QString("Drag.cancel()")
            << int(QEvent::DragLeave)
            << QByteArray("<Unknown File>: QML QQuickDragAttached: cancel() cannot be called from within a drag event handler");
    QTest::newRow("Drag.drop() in Leave")
            << QString("Drag.drop()")
            << int(QEvent::DragLeave)
            << QByteArray("<Unknown File>: QML QQuickDragAttached: drop() cannot be called from within a drag event handler");
    QTest::newRow("Drag.active = true in Leave")
            << QString("Drag.active = true")
            << int(QEvent::DragLeave)
            << QByteArray("<Unknown File>: QML QQuickDragAttached: active cannot be changed from within a drag event handler");
    QTest::newRow("Drag.active = false in Leave")
            << QString("Drag.active = false")
            << int(QEvent::DragLeave)
            << QByteArray();
    QTest::newRow("move in Leave")
            << QString("x = 23")
            << int(QEvent::DragLeave)
            << QByteArray();

    QTest::newRow("Drag.start() in Drop")
            << QString("Drag.start()")
            << int(QEvent::Drop)
            << QByteArray("<Unknown File>: QML QQuickDragAttached: start() cannot be called from within a drag event handler");
    QTest::newRow("Drag.cancel() in Drop")
            << QString("Drag.cancel()")
            << int(QEvent::Drop)
            << QByteArray("<Unknown File>: QML QQuickDragAttached: cancel() cannot be called from within a drag event handler");
    QTest::newRow("Drag.drop() in Drop")
            << QString("Drag.drop()")
            << int(QEvent::Drop)
            << QByteArray("<Unknown File>: QML QQuickDragAttached: drop() cannot be called from within a drag event handler");
    QTest::newRow("Drag.active = true in Drop")
            << QString("Drag.active = true")
            << int(QEvent::Drop)
            << QByteArray("<Unknown File>: QML QQuickDragAttached: active cannot be changed from within a drag event handler");
    QTest::newRow("Drag.active = false in Drop")
            << QString("Drag.active = false")
            << int(QEvent::Drop)
            << QByteArray();
    QTest::newRow("move in Drop")
            << QString("x = 23")
            << int(QEvent::Drop)
            << QByteArray();
}

void tst_QQuickDrag::recursion()
{
    QFETCH(QString, script);
    QFETCH(int, type);
    QFETCH(QByteArray, warning);

    if (!warning.isEmpty())
        QTest::ignoreMessage(QtWarningMsg, warning.constData());

    QQuickCanvas canvas;
    RecursingDropTarget dropTarget(script, type, canvas.rootItem());
    dropTarget.setSize(QSizeF(100, 100));
    QQmlComponent component(&engine);
    component.setData(
            "import QtQuick 2.0\n"
            "Item {\n"
                "x: 50; y: 50\n"
                "width: 10; height: 10\n"
            "}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickItem *item = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(item);
    item->setParentItem(canvas.rootItem());

    dropTarget.setItem(item);

    evaluate<void>(item, "Drag.start()");
    QCOMPARE(dropTarget.enterEvents, 1);
    QCOMPARE(dropTarget.moveEvents, 0);
    QCOMPARE(dropTarget.dropEvents, 0);
    QCOMPARE(dropTarget.leaveEvents, 0);

    evaluate<void>(item, "y = 15");
    QCoreApplication::processEvents();
    QCOMPARE(dropTarget.enterEvents, 1);
    QCOMPARE(dropTarget.moveEvents, 1);
    QCOMPARE(dropTarget.dropEvents, 0);
    QCOMPARE(dropTarget.leaveEvents, 0);

    if (type == QEvent::Drop) {
        QCOMPARE(evaluate<bool>(item, "Drag.drop() == Qt.MoveAction"), true);
        QCOMPARE(dropTarget.enterEvents, 1);
        QCOMPARE(dropTarget.moveEvents, 1);
        QCOMPARE(dropTarget.dropEvents, 1);
        QCOMPARE(dropTarget.leaveEvents, 0);
    } else {
        evaluate<void>(item, "Drag.cancel()");
        QCOMPARE(dropTarget.enterEvents, 1);
        QCOMPARE(dropTarget.moveEvents, 1);
        QCOMPARE(dropTarget.dropEvents, 0);
        QCOMPARE(dropTarget.leaveEvents, 1);
    }
}


QTEST_MAIN(tst_QQuickDrag)

#include "tst_qquickdrag.moc"
