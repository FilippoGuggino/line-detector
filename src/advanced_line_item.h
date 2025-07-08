#ifndef ADVANCED_LINE_H
#define ADVANCED_LINE_H

#include <vector>

#include <QGraphicsObject>
#include <QLayout>
#include <QPainter>
#include <QPushButton>
#include <QWidget>

struct RectangleProperties {
    double position_on_line; // 0.0 to 1.0
};

class AdvancedLineItem : public QGraphicsObject {
    Q_OBJECT

public:
    enum { Type = UserType + 1 };

    int type() const override
    {
        return Type;
    }

    struct State {
        QPointF start_point;
        QPointF end_point;
    };

    class ControlPanel : public QWidget {
    public:
        ControlPanel(AdvancedLineItem::State& state)
            : m_state(state)
        {
        }

    private:
        AdvancedLineItem::State& m_state;
    };

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

    AdvancedLineItem(std::string key, const QPointF& start, const QPointF& end, QGraphicsItem* parent = nullptr);

    // Required QGraphicsItem overrides
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    void set_num_rects(unsigned int num_rects);
    std::string key();
    AdvancedLineItem::State& state();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    Handle get_handle_at_position(const QPointF& pos, int& rect_index_out) const;
    void update_cursor(const QPointF& pos);

    std::string m_key;
    State m_state;

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
