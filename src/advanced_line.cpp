#include <QLineF>
#include <QTransform>
#include <QWidget>
#include <QtMath>

#include "advanced_line.h"

static bool is_near_point(const QPoint& point, const QPoint& target)
{
    const int threshold = 10;
    return (point - target).manhattanLength() < threshold;
}

static qreal distance_to_line_segment(const QPoint& p, const QPoint& p1, const QPoint& p2)
{
    QLineF line(p1, p2);
    qreal line_length_sq = line.length() * line.length();
    if (line_length_sq == 0.0)
        return QLineF(p, p1).length();

    qreal t = (QPointF::dotProduct(p - p1, p2 - p1)) / line_length_sq;
    t = qMax(0.0, qMin(1.0, t));

    QPointF projection = p1 + t * (p2 - p1);
    return QLineF(p, projection).length();
}

static std::vector<RectangleProperties> linspace(double start, double end, unsigned int num, bool endpoint = false)
{
    std::vector<RectangleProperties> out;
    if (num == 0)
        return out;
    if (endpoint && num == 1) {
        out.push_back(RectangleProperties{ start });
        return out;
    }

    double step = endpoint ? 1.0 / (num - 1) : 1.0 / (num + 1);
    for (unsigned int i = 0; i < num; i++) {
        double position = endpoint ? i * step : (i + 1) * step;
        out.push_back(RectangleProperties{ position });
    }

    return out;
}

AdvancedLine::AdvancedLine(const QPoint& start, const QPoint& end)
    : m_start_point(start)
    , m_end_point(end)
    , m_shared_rect_width(60.0)
    , m_shared_rect_height(40.0)
{

    m_rects = linspace(0.0, 1.0, m_num_rects);
}

void AdvancedLine::paint(QPainter* painter) const
{
    painter->save();

    QLineF line(m_start_point, m_end_point);

    // Draw the line
    painter->setPen(QPen(Qt::blue, 2));
    painter->drawLine(line);

    // Draw endpoint handles
    painter->setBrush(Qt::red);
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(m_start_point, 5, 5);
    painter->drawEllipse(m_end_point, 5, 5);

    // Draw the three rectangles
    QColor rect_color = QColor(Qt::green);
    rect_color.setAlphaF(0.5);
    painter->setBrush(rect_color);
    painter->setPen(QPen(Qt::darkGreen, 1));
    qreal angle = line.angle();

    for (const auto& rect : m_rects) {
        QPointF center_point = line.pointAt(rect.position_on_line);
        painter->save();
        painter->translate(center_point);
        painter->rotate(-angle);
        QRectF rectangle(-m_shared_rect_width / 2, -m_shared_rect_height / 2, m_shared_rect_width, m_shared_rect_height);
        painter->drawRect(rectangle);
        painter->restore();
    }

    painter->restore();
}

// This function centralizes all hit-testing logic. It checks for handles on
// endpoints, the line, and rectangle borders/corners.
AdvancedLine::Handle AdvancedLine::get_handle_at_position(const QPoint& pos, int& rect_index_out) const
{
    rect_index_out = -1;

    // Check endpoints first (highest priority)
    if (is_near_point(pos, m_start_point) || is_near_point(pos, m_end_point)) {
        return Handle::Body;
    }

    if (distance_to_line_segment(pos, m_start_point, m_end_point) < 5.0) {
        return Handle::Body;
    }

    QLineF line(m_start_point, m_end_point);
    qreal angle = line.angle();
    const int resize_handle_size = 8; // Area around borders to detect hover/click

    // Check rectangles
    for (int i = 0; i < m_num_rects; ++i) {
        QPointF center_point = line.pointAt(m_rects[i].position_on_line);

        // Transform mouse position into the rectangle's local coordinate system
        QTransform transform;
        transform.translate(center_point.x(), center_point.y());
        transform.rotate(-angle);
        QPointF local_pos = transform.inverted().map(QPointF(pos));

        // Create a slightly larger rect for initial broad-phase check
        QRectF hover_rect = QRectF(-m_shared_rect_width / 2, -m_shared_rect_height / 2, m_shared_rect_width, m_shared_rect_height)
                                .adjusted(-resize_handle_size, -resize_handle_size, resize_handle_size, resize_handle_size);

        if (hover_rect.contains(local_pos)) {
            rect_index_out = i;
            qreal half_w = m_shared_rect_width / 2;
            qreal half_h = m_shared_rect_height / 2;

            // --- CORNER CHECKS (HIGHEST PRIORITY) ---
            if (QRectF(half_w - resize_handle_size, -half_h - resize_handle_size, resize_handle_size * 2, resize_handle_size * 2).contains(local_pos))
                return Handle::TopRight;
            if (QRectF(-half_w - resize_handle_size, -half_h - resize_handle_size, resize_handle_size * 2, resize_handle_size * 2).contains(local_pos))
                return Handle::TopLeft;
            if (QRectF(half_w - resize_handle_size, half_h - resize_handle_size, resize_handle_size * 2, resize_handle_size * 2).contains(local_pos))
                return Handle::BottomRight;
            if (QRectF(-half_w - resize_handle_size, half_h - resize_handle_size, resize_handle_size * 2, resize_handle_size * 2).contains(local_pos))
                return Handle::BottomLeft;

            // --- BORDER CHECKS ---
            if (qAbs(local_pos.x() - half_w) < resize_handle_size)
                return Handle::Right;
            if (qAbs(local_pos.x() + half_w) < resize_handle_size)
                return Handle::Left;
            if (qAbs(local_pos.y() - half_h) < resize_handle_size)
                return Handle::Bottom;
            if (qAbs(local_pos.y() + half_h) < resize_handle_size)
                return Handle::Top;
        }
    }

    return Handle::None;
}

bool AdvancedLine::handle_mouse_press(const QPoint& pos)
{
    m_drag_state = DragState(); // Reset state

    int rect_index = -1;
    Handle handle = get_handle_at_position(pos, rect_index);

    if (handle == Handle::None) {
        return false; // This object was not clicked
    }

    m_drag_state.dragging = true;

    if (rect_index != -1) {
        // A rectangle part was clicked
        m_drag_state.rect_index = rect_index;
        m_drag_state.handle = handle;
    }
    else {
        // A line part was clicked
        if (is_near_point(pos, m_start_point)) {
            m_drag_state.line_point = &m_start_point;
        }
        else if (is_near_point(pos, m_end_point)) {
            m_drag_state.line_point = &m_end_point;
        }
        else {
            // The line body was clicked
            m_drag_state.is_dragging_line = true;
            m_drag_state.drag_start_offset = pos - m_start_point;
        }
    }

    return true;
}

void AdvancedLine::handle_mouse_move(const QPoint& pos)
{
    if (!m_drag_state.dragging)
        return;

    if (m_drag_state.line_point) {
        *m_drag_state.line_point = pos;
    }
    else if (m_drag_state.is_dragging_line) {
        QPoint new_start_point = pos - m_drag_state.drag_start_offset;
        QPoint delta = new_start_point - m_start_point;
        m_start_point = new_start_point;
        m_end_point += delta;
    }
    else if (m_drag_state.rect_index != -1) {
        QLineF line(m_start_point, m_end_point);
        qreal angle = line.angle();
        QPointF center_point = line.pointAt(m_rects[m_drag_state.rect_index].position_on_line);

        QTransform transform;
        transform.translate(center_point.x(), center_point.y());
        transform.rotate(-angle);
        QPointF local_pos = transform.inverted().map(QPointF(pos));

        switch (m_drag_state.handle) {
        case Handle::Left:
            m_shared_rect_width = qMax(10.0, -local_pos.x() * 2);
            break;
        case Handle::Right:
            m_shared_rect_width = qMax(10.0, local_pos.x() * 2);
            break;
        case Handle::Top:
            m_shared_rect_height = qMax(10.0, -local_pos.y() * 2);
            break;
        case Handle::Bottom:
            m_shared_rect_height = qMax(10.0, local_pos.y() * 2);
            break;
        case Handle::TopLeft:
            m_shared_rect_width = qMax(10.0, -local_pos.x() * 2);
            m_shared_rect_height = qMax(10.0, -local_pos.y() * 2);
            break;
        case Handle::TopRight:
            m_shared_rect_width = qMax(10.0, local_pos.x() * 2);
            m_shared_rect_height = qMax(10.0, -local_pos.y() * 2);
            break;
        case Handle::BottomLeft:
            m_shared_rect_width = qMax(10.0, -local_pos.x() * 2);
            m_shared_rect_height = qMax(10.0, local_pos.y() * 2);
            break;
        case Handle::BottomRight:
            m_shared_rect_width = qMax(10.0, local_pos.x() * 2);
            m_shared_rect_height = qMax(10.0, local_pos.y() * 2);
            break;
        default:
            break;
        }
    }
}

void AdvancedLine::handle_mouse_release()
{
    m_drag_state = DragState(); // Reset drag state
}

// private helper function to select the best cursor for a given angle.
// Angles are 0-360, with 0 pointing right (3 o'clock).
static Qt::CursorShape map_angle_to_cursor(qreal angle)
{
    angle = fmod(angle, 180.0);
    if (angle < 0) {
        angle += 180.0;
    }
    std::cout << " angle after: " << angle << std::endl;

    // Check which 45-degree slice the angle falls into
    if ((angle >= 0.0 && angle < 22.5) || (angle >= 157.5 && angle < 180.0)) {
        return Qt::SizeHorCursor; // Horizontal
    }
    else if (angle >= 22.5 && angle < 67.5) {
        return Qt::SizeBDiagCursor; // Diagonal '\'
    }
    else if (angle >= 67.5 && angle < 112.5) {
        return Qt::SizeVerCursor; // Vertical
    }
    else { // angle >= 112.5 && angle < 157.5
        return Qt::SizeFDiagCursor; // Diagonal '/'
    }
}

Qt::CursorShape AdvancedLine::get_cursor_for_position(const QPoint& pos) const
{
    int rect_index = -1;
    Handle handle = get_handle_at_position(pos, rect_index);

    // Handle non-resizable parts first
    if (handle == Handle::None) {
        return Qt::ArrowCursor;
    }
    if (handle == Handle::Body) {
        return Qt::SizeAllCursor;
    }

    // For resizable handles, calculate the appropriate rotated cursor
    QLineF line(m_start_point, m_end_point);
    qreal line_angle = line.angle(); // Angle of the line itself

    qreal handle_base_angle = 0.0;
    switch (handle) {
    case Handle::Top:
    case Handle::Bottom:
        handle_base_angle = 90.0; // Perpendicular to the line
        break;
    case Handle::Left:
    case Handle::Right:
        handle_base_angle = 0.0; // Parallel to the line
        break;
    case Handle::TopLeft:
    case Handle::BottomRight:
        handle_base_angle = 135.0; // Diagonal '\' relative to the line
        break;
    case Handle::TopRight:
    case Handle::BottomLeft:
        handle_base_angle = 45.0; // Diagonal '/' relative to the line
        break;
    default:
        return Qt::ArrowCursor;
    }

    // The final cursor angle is the sum of the line's angle and the handle's base angle.
    qreal final_cursor_angle = line_angle + handle_base_angle;

    return map_angle_to_cursor(final_cursor_angle);
}

void AdvancedLine::set_num_rects(unsigned int num_rects)
{
    m_num_rects = num_rects;
    m_rects = linspace(0.0, 1.0, m_num_rects);
}
