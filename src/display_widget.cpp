#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QPixmap>

#include "advanced_line_item.h"
#include "display_widget.h"

#include <iostream>

DisplayWidget::DisplayWidget(QWidget* parent)
    : QGraphicsView(parent)
{
    setMinimumSize(500, 500);

    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(0, 0, 500, 500);
    setScene(m_scene);

    QPixmap pixmap("test-images/banana-bg.jpg");
    pixmap = pixmap.scaled(scene()->width(), scene()->height());
    if (pixmap.isNull()) {
        pixmap = QPixmap(256, 256);
        pixmap.fill(Qt::lightGray);
    }
    m_background_item = m_scene->addPixmap(pixmap);
    // m_background_item->setPos(QPointF(120, 120));
    m_background_item->setZValue(-1); // Ensure it's drawn behind everything else

    setRenderHint(QPainter::Antialiasing);
    setMouseTracking(true); // Needed for hover events if no item is being dragged
}

void DisplayWidget::mousePressEvent(QMouseEvent* event)
{
    QPointF scene_pos = mapToScene(event->pos());

    QGraphicsItem* clicked_item = scene()->itemAt(scene_pos, transform());

    if (clicked_item) {
        AdvancedLineItem* advanced_line = qgraphicsitem_cast<AdvancedLineItem*>(clicked_item);
        if (advanced_line) {
            std::cout << "advanced line: " << advanced_line->key() << std::endl;
        }
    }

    QGraphicsView::mousePressEvent(event);
}
