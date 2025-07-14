#ifndef ADVANCED_LINE_H
#define ADVANCED_LINE_H

#include <vector>

#include <QGraphicsObject>
#include <QPainter>

#include <opencv2/opencv.hpp>

struct RectangleProperties {
    double position_on_line; // 0.0 to 1.0
};

class AdvancedLineItem : public QGraphicsObject {
    Q_OBJECT

public:
    // Enum for identifying which part of the item is being interacted with
    enum class Handle {
        None,
        Body,
        StartPoint,
        EndPoint,
        RectangleMove, // For moving the rectangles along the line (not implemented, but good placeholder)
        RectangleResizeTop,
        RectangleResizeBottom,
        RectangleResizeLeft,
        RectangleResizeRight,
        RectangleResizeTopLeft,
        RectangleResizeTopRight,
        RectangleResizeBottomLeft,
        RectangleResizeBottomRight
    };

    AdvancedLineItem(const QPointF& start, const QPointF& end, QGraphicsItem* parent = nullptr);

    // Required QGraphicsItem overrides
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    void set_num_rects(unsigned int num_rects);
    std::vector<cv::RotatedRect> get_rect_regions() const;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

private:
    Handle get_handle_at_position(const QPointF& pos, int& rect_index_out) const;
    void update_cursor(const QPointF& pos);

    QPointF m_start_point;
    QPointF m_end_point;

    unsigned int m_num_rects = 3;
    std::vector<RectangleProperties> m_rects;
    qreal m_shared_rect_width;
    qreal m_shared_rect_height;

    // State for dragging operations
    struct DragState {
        bool dragging = false;
        Handle active_handle = Handle::None;
        int active_rect_index = -1;
        QPointF drag_start_offset; // For moving the whole line
    } m_drag_state;
};

#endif /* ADVANCED_LINE_H */
