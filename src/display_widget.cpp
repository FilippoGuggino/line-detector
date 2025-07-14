#include "display_widget.h"

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QPixmap>

DisplayWidget::DisplayWidget(QWidget* parent)
    : QGraphicsView(parent)
{
    setMinimumSize(500, 500);

    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(0, 0, 500, 500);
    setScene(m_scene);

    setRenderHint(QPainter::Antialiasing);
    setMouseTracking(true); // Needed for hover events if no item is being dragged
}

void DisplayWidget::set_pixmap(QPixmap pixmap)
{
    m_scene->setSceneRect(0, 0, pixmap.height(), pixmap.width());
    m_pixmap = scene()->addPixmap(pixmap);
    m_pixmap->setZValue(-1); // Ensure it's drawn behind everything else
}
