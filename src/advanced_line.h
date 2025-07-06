#ifndef ADVANCED_LINE_H
#define ADVANCED_LINE_H

#include <QPainter>
#include <QPoint>
#include <vector>

struct RectangleProperties {
    qreal position_on_line = 0.5;
};

class AdvancedLine {
public:
    AdvancedLine(const QPoint& start, const QPoint& end);

    void paint(QPainter* painter) const;
    bool handle_mouse_press(const QPoint& pos);
    void handle_mouse_move(const QPoint& pos);
    void handle_mouse_release();
    Qt::CursorShape get_cursor_for_position(const QPoint& pos) const;

    void set_num_rects(unsigned int num_rects);

private:
    enum class Handle {
        None,
        Left,
        Right,
        Top,
        Bottom,
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight,
        Body
    };

    struct DragState {
        bool dragging = false;
        bool is_dragging_line = false;
        int rect_index = -1;
        Handle handle = Handle::None;
        QPoint* line_point = nullptr;
        QPoint drag_start_offset;
    };

    Handle get_handle_at_position(const QPoint& pos, int& rect_index) const;

    QPoint m_start_point;
    QPoint m_end_point;
    unsigned int m_num_rects = 4;
    std::vector<RectangleProperties> m_rects;
    qreal m_shared_rect_width;
    qreal m_shared_rect_height;
    DragState m_drag_state;
};

#endif /* ADVANCED_LINE_H */
