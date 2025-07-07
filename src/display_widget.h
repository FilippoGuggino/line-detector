#ifndef DISPLAY_WIDGET_H
#define DISPLAY_WIDGET_H

#include <QGraphicsView>

class QGraphicsScene;
class QGraphicsPixmapItem;

class DisplayWidget : public QGraphicsView {
    Q_OBJECT

public:
    explicit DisplayWidget(QWidget* parent = nullptr);

private:
    QGraphicsScene* m_scene;
    QGraphicsPixmapItem* m_background_item;
};

#endif /* DISPLAY_WIDGET_H */
