#ifndef ADVANCED_LINE_H
#define ADVANCED_LINE_H

#include <QMouseEvent>
#include <QObject>
#include <QPainter>
#include <QPoint>

// A simple struct for rectangle properties along the line
struct RectangleProperties {
    qreal position_on_line = 0.5;
};

class AdvancedLine {
public:
    AdvancedLine(const QPoint& start, const QPoint& end);

    // Public interface for the DisplayWidget to use
    void paint(QPainter* painter) const;
    bool handle_mouse_press(const QPoint& pos);
    void handle_mouse_move(const QPoint& pos);
    void handle_mouse_release();
    void update_cursor(const QPoint& pos, QWidget* parent);

private:
    // Enum and struct for managing internal drag state
    enum class Handle { None,
        Left,
        Right,
        Top,
        Bottom };

    struct DragState {
        bool dragging = false;
        bool is_dragging_line = false;
        int rect_index = -1;
        Handle handle = Handle::None;
        QPoint* line_point = nullptr;
        QPoint drag_start_offset;
    };

    // Data for this specific line object
    QPoint m_start_point;
    QPoint m_end_point;
    unsigned int m_num_rects = 4;
    std::vector<RectangleProperties> m_rects;
    qreal m_shared_rect_width;
    qreal m_shared_rect_height;
    DragState m_drag_state;
};

#endif /* ADVANCED_LINE_H */
