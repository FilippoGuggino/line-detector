#include <QLineF>
#include <QTransform>
#include <QWidget>
#include <QtMath>

#include "advanced_line.h"

static bool isNearPoint(const QPoint& point, const QPoint& target)
{
    const int threshold = 10;
    return (point - target).manhattanLength() < threshold;
}

static qreal distanceToLineSegment(const QPoint& p, const QPoint& p1, const QPoint& p2)
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
    if (endpoint) {
        out.push_back(RectangleProperties{ start });
    }

    double step = endpoint == false ? 1.0 / (num + 1) : 1.0 / (num - 1);
    for (int i = 0; i < num; i++) {
        out.push_back(RectangleProperties{ (i + 1) * step });
    }

    if (endpoint) {
        out.push_back(RectangleProperties{ end });
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

bool AdvancedLine::handle_mouse_press(const QPoint& pos)
{
    // Reset internal state before checking for a new press
    m_drag_state = DragState();

    if (isNearPoint(pos, m_start_point)) {
        m_drag_state.dragging = true;
        m_drag_state.line_point = &m_start_point;
        return true;
    }
    if (isNearPoint(pos, m_end_point)) {
        m_drag_state.dragging = true;
        m_drag_state.line_point = &m_end_point;
        return true;
    }

    QLineF line(m_start_point, m_end_point);
    qreal angle = line.angle();
    const int resizeHandleSize = 10;
    for (int i = 0; i < m_num_rects; ++i) {
        QPointF center_point = line.pointAt(m_rects[i].position_on_line);
        QTransform transform;
        transform.translate(center_point.x(), center_point.y());
        transform.rotate(-angle);
        QPointF local_pos = transform.inverted().map(QPointF(pos));

        if (qAbs(local_pos.x()) < m_shared_rect_width / 2 + resizeHandleSize && qAbs(local_pos.y()) < m_shared_rect_height / 2 + resizeHandleSize) {
            if (qAbs(local_pos.x() - m_shared_rect_width / 2) < resizeHandleSize)
                m_drag_state.handle = Handle::Right;
            else if (qAbs(local_pos.x() + m_shared_rect_width / 2) < resizeHandleSize)
                m_drag_state.handle = Handle::Left;
            else if (qAbs(local_pos.y() - m_shared_rect_height / 2) < resizeHandleSize)
                m_drag_state.handle = Handle::Bottom;
            else if (qAbs(local_pos.y() + m_shared_rect_height / 2) < resizeHandleSize)
                m_drag_state.handle = Handle::Top;

            if (m_drag_state.handle != Handle::None) {
                m_drag_state.dragging = true;
                m_drag_state.rect_index = i;
                return true;
            }
        }
    }

    if (distanceToLineSegment(pos, m_start_point, m_end_point) < 5.0) {
        m_drag_state.dragging = true;
        m_drag_state.is_dragging_line = true;
        m_drag_state.drag_start_offset = pos - m_start_point;
        return true;
    }

    return false; // This object was not clicked
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
        default:
            break;
        }
    }
}

void AdvancedLine::handle_mouse_release()
{
    m_drag_state = DragState(); // Reset drag state
}

void AdvancedLine::update_cursor(const QPoint& pos, QWidget* parent)
{
    if (!parent)
        return;

    if (m_drag_state.dragging) {
        if (m_drag_state.line_point || m_drag_state.is_dragging_line) {
            parent->setCursor(Qt::SizeAllCursor);
        }
        else if (m_drag_state.handle == Handle::Left || m_drag_state.handle == Handle::Right) {
            parent->setCursor(Qt::SizeHorCursor);
        }
        else {
            parent->setCursor(Qt::SizeVerCursor);
        }
    }
    else {
        // Bonus: check for hover cursor (not implemented to keep it simple, but this is where it would go)
        parent->setCursor(Qt::ArrowCursor);
    }
}

void AdvancedLine::set_num_rects(unsigned int num_rects)
{
    m_num_rects = num_rects;
    m_rects = linspace(0.0, 1.0, m_num_rects);
}
