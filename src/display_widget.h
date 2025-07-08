#ifndef DISPLAY_WIDGET_H
#define DISPLAY_WIDGET_H

#include <QGraphicsView>

class QGraphicsScene;
class QGraphicsPixmapItem;

class DisplayWidget : public QGraphicsView {
    Q_OBJECT

public:
    explicit DisplayWidget(QWidget* parent = nullptr);
    void set_pixmap(QPixmap pixmap);

private:
    QGraphicsScene* m_scene;
    QGraphicsPixmapItem* m_pixmap;
};

#endif /* DISPLAY_WIDGET_H */
