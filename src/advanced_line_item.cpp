#include "advanced_line_item.h"
#include <QCursor>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QLineF>
#include <QTransform>
#include <QtMath>

static std::vector<RectangleProperties> linspace(double start, double end, unsigned int num, bool endpoint = false)
{
    std::vector<RectangleProperties> out;
    if (num == 0)
        return out;
    if (endpoint && num == 1) {
        out.push_back({ start });
        return out;
    }
    double step = (num > 1) ? (end - start) / (num - 1) : 0;
    if (!endpoint && num > 0) {
        step = (end - start) / (num + 1);
        start += step;
    }
    for (unsigned int i = 0; i < num; i++) {
        out.push_back({ start + i * step });
    }
    return out;
}

static qreal distance_to_line_segment(const QPointF& p, const QPointF& p1, const QPointF& p2)
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

AdvancedLineItem::AdvancedLineItem(std::string key, const QPointF& start, const QPointF& end, QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , m_key(key)
    , m_state(AdvancedLineItem::State{ start, end })
    , m_shared_rect_width(60.0)
    , m_shared_rect_height(40.0)
{
    setAcceptHoverEvents(true);
    // This flag lets the scene handle moving the entire object.
    // We will intercept the move in itemChange() to update our points.
    setFlag(ItemIsMovable);
    setFlag(ItemSendsGeometryChanges); // Needed for itemChange to be called

    m_rects = linspace(0.0, 1.0, m_num_rects);
}

QRectF AdvancedLineItem::boundingRect() const
{
    // This is crucial. It must return a rectangle that encloses the entire
    // drawable area of the item, including handles and line thickness.
    QRectF rect = QRectF(m_state.start_point, m_state.end_point).normalized();

    QLineF line(m_state.start_point, m_state.end_point);
    qreal angle = line.angle();

    // Expand the bounding rect to include the rotated rectangles
    for (const auto& r : m_rects) {
        QPointF center = line.pointAt(r.position_on_line);
        QTransform t;
        t.translate(center.x(), center.y());
        t.rotate(-angle);
        // mapRect gives the bounding box of the transformed rectangle
        rect = rect.united(t.mapRect(QRectF(-m_shared_rect_width / 2, -m_shared_rect_height / 2, m_shared_rect_width, m_shared_rect_height)));
    }

    // Add a margin for selection, handles, and line thickness
    const qreal margin = 15.0;
    return rect.adjusted(-margin, -margin, margin, margin);
}

void AdvancedLineItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->save();

    QLineF line(m_state.start_point, m_state.end_point);

    // Draw the line
    painter->setPen(QPen(Qt::blue, 2));
    painter->drawLine(line);

    // Draw endpoint handles
    painter->setBrush(Qt::red);
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(m_state.start_point, 5, 5);
    painter->drawEllipse(m_state.end_point, 5, 5);

    // Draw the rectangles
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

void AdvancedLineItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    m_drag_state = DragState();

    int rect_index = -1;
    Handle handle = get_handle_at_position(event->pos(), rect_index);

    if (handle != Handle::None) {
        m_drag_state.dragging = true;
        m_drag_state.active_handle = handle;
        m_drag_state.active_rect_index = rect_index;
        m_drag_state.drag_start_offset = event->pos() - m_state.start_point;
    }

    // If we click on the body but not an endpoint, we let the base class
    // handle it, which initiates the move (because of the ItemIsMovable flag).
    // For all other handles, we manage the drag ourselves.
    if (handle == Handle::Body) {
        QGraphicsObject::mousePressEvent(event);
    }
    else {
        event->accept();
    }
}

void AdvancedLineItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (!m_drag_state.dragging) {
        // If not dragging, pass to base class (e.g., for moving the item)
        QGraphicsObject::mouseMoveEvent(event);
        return;
    }

    // Tell the scene that we are about to change the geometry
    prepareGeometryChange();

    const QPointF pos = event->pos();

    switch (m_drag_state.active_handle) {
    case Handle::StartPoint:
        m_state.start_point = pos;
        break;
    case Handle::EndPoint:
        m_state.end_point = pos;
        break;
    case Handle::RectangleResizeLeft:
    case Handle::RectangleResizeRight:
    case Handle::RectangleResizeTop:
    case Handle::RectangleResizeBottom:
    case Handle::RectangleResizeTopLeft:
    case Handle::RectangleResizeTopRight:
    case Handle::RectangleResizeBottomLeft:
    case Handle::RectangleResizeBottomRight: {
        QLineF line(m_state.start_point, m_state.end_point);
        qreal angle = line.angle();
        QPointF center_point = line.pointAt(m_rects[m_drag_state.active_rect_index].position_on_line);

        QTransform transform;
        transform.translate(center_point.x(), center_point.y());
        transform.rotate(-angle);
        QPointF local_pos = transform.inverted().map(pos);

        // This logic is simplified; a full implementation would be more robust
        if (m_drag_state.active_handle == Handle::RectangleResizeRight || m_drag_state.active_handle == Handle::RectangleResizeTopRight || m_drag_state.active_handle == Handle::RectangleResizeBottomRight)
            m_shared_rect_width = qMax(10.0, local_pos.x() * 2);
        if (m_drag_state.active_handle == Handle::RectangleResizeLeft || m_drag_state.active_handle == Handle::RectangleResizeTopLeft || m_drag_state.active_handle == Handle::RectangleResizeBottomLeft)
            m_shared_rect_width = qMax(10.0, -local_pos.x() * 2);
        if (m_drag_state.active_handle == Handle::RectangleResizeBottom || m_drag_state.active_handle == Handle::RectangleResizeBottomRight || m_drag_state.active_handle == Handle::RectangleResizeBottomLeft)
            m_shared_rect_height = qMax(10.0, local_pos.y() * 2);
        if (m_drag_state.active_handle == Handle::RectangleResizeTop || m_drag_state.active_handle == Handle::RectangleResizeTopLeft || m_drag_state.active_handle == Handle::RectangleResizeTopRight)
            m_shared_rect_height = qMax(10.0, -local_pos.y() * 2);

        break;
    }
    default:
        // For Handle::Body, the base class handles movement.
        QGraphicsObject::mouseMoveEvent(event);
        break;
    }

    // Redraw the item
    update();
}

void AdvancedLineItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    m_drag_state.dragging = false;
    // Pass to base class to finalize movement, etc.
    QGraphicsObject::mouseReleaseEvent(event);
    update_cursor(event->pos());
}

void AdvancedLineItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    update_cursor(event->pos());
    QGraphicsObject::hoverMoveEvent(event);
}

void AdvancedLineItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    setCursor(Qt::ArrowCursor);
    QGraphicsObject::hoverLeaveEvent(event);
}

QVariant AdvancedLineItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    // This function is called by the scene when the item's state changes.
    // We are interested in when its position changes due to being moved.
    if (change == ItemPositionHasChanged && scene()) {
        // value is the new position. The delta is the change from the old position.
        QPointF delta = value.toPointF() - pos();
        // Move our internal points by the same amount.
        m_state.start_point += delta;
        m_state.end_point += delta;
    }
    return QGraphicsObject::itemChange(change, value);
}

AdvancedLineItem::Handle AdvancedLineItem::get_handle_at_position(const QPointF& pos, int& rect_index_out) const
{
    rect_index_out = -1;
    const int handle_size = 10;

    // Check endpoints first (highest priority)
    if (QRectF(m_state.start_point - QPointF(handle_size / 2.0, handle_size / 2.0), QSizeF(handle_size, handle_size)).contains(pos))
        return Handle::StartPoint;
    if (QRectF(m_state.end_point - QPointF(handle_size / 2.0, handle_size / 2.0), QSizeF(handle_size, handle_size)).contains(pos))
        return Handle::EndPoint;

    // Check rectangles
    QLineF line(m_state.start_point, m_state.end_point);
    qreal angle = line.angle();
    const int resize_border_width = 8;

    for (int i = 0; i < m_num_rects; ++i) {
        QPointF center = line.pointAt(m_rects[i].position_on_line);
        QTransform t;
        t.translate(center.x(), center.y());
        t.rotate(-angle);
        QPointF local_pos = t.inverted().map(pos);

        QRectF rect_body(-m_shared_rect_width / 2, -m_shared_rect_height / 2, m_shared_rect_width, m_shared_rect_height);
        QRectF outer_rect = rect_body.adjusted(-resize_border_width, -resize_border_width, resize_border_width, resize_border_width);

        if (outer_rect.contains(local_pos)) {
            rect_index_out = i;
            qreal half_w = m_shared_rect_width / 2;
            qreal half_h = m_shared_rect_height / 2;

            // --- CORNER CHECKS (HIGHEST PRIORITY) ---
            if (QRectF(half_w - resize_border_width, -half_h - resize_border_width, resize_border_width * 2, resize_border_width * 2).contains(local_pos))
                return Handle::RectangleResizeTopRight;
            if (QRectF(-half_w - resize_border_width, -half_h - resize_border_width, resize_border_width * 2, resize_border_width * 2).contains(local_pos))
                return Handle::RectangleResizeTopLeft;
            if (QRectF(half_w - resize_border_width, half_h - resize_border_width, resize_border_width * 2, resize_border_width * 2).contains(local_pos))
                return Handle::RectangleResizeBottomRight;
            if (QRectF(-half_w - resize_border_width, half_h - resize_border_width, resize_border_width * 2, resize_border_width * 2).contains(local_pos))
                return Handle::RectangleResizeBottomLeft;

            // --- BORDER CHECKS ---
            if (local_pos.y() < rect_body.top() + resize_border_width)
                return Handle::RectangleResizeTop;
            if (local_pos.y() > rect_body.bottom() - resize_border_width)
                return Handle::RectangleResizeBottom;
            if (local_pos.x() < rect_body.left() + resize_border_width)
                return Handle::RectangleResizeLeft;
            if (local_pos.x() > rect_body.right() - resize_border_width)
                return Handle::RectangleResizeRight;
        }
    }

    // Check line body last
    if (distance_to_line_segment(pos, m_state.start_point, m_state.end_point) < 5.0) {
        return Handle::Body;
    }

    return Handle::None;
}

static Qt::CursorShape map_angle_to_cursor(qreal angle)
{
    angle = fmod(angle, 180.0);
    if (angle < 0) {
        angle += 180.0;
    }

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

void AdvancedLineItem::update_cursor(const QPointF& pos)
{
    int rect_index;
    Handle handle = get_handle_at_position(pos, rect_index);
    Qt::CursorShape cursor = Qt::ArrowCursor;

    // For resizable handles, calculate the appropriate rotated cursor
    QLineF line(m_state.start_point, m_state.end_point);
    qreal line_angle = line.angle(); // Angle of the line itself

    switch (handle) {
    case Handle::StartPoint:
    case Handle::EndPoint:
        cursor = Qt::CrossCursor;
        break;
    case Handle::Body:
        cursor = Qt::SizeAllCursor;
        break;
    case Handle::RectangleResizeTop:
    case Handle::RectangleResizeBottom:
        cursor = map_angle_to_cursor(line_angle + 90.0);
        break;
    case Handle::RectangleResizeLeft:
    case Handle::RectangleResizeRight:
        cursor = map_angle_to_cursor(line_angle + 0.0);
        break;
    case Handle::RectangleResizeTopLeft:
    case Handle::RectangleResizeBottomRight:
        cursor = map_angle_to_cursor(line_angle + 135.0);

        break;
    case Handle::RectangleResizeTopRight:
    case Handle::RectangleResizeBottomLeft:
        cursor = map_angle_to_cursor(line_angle + 45.0);
        break;
    default:
        cursor = Qt::ArrowCursor;
        break;
    }
    setCursor(cursor);
}

void AdvancedLineItem::set_num_rects(unsigned int num_rects)
{
    prepareGeometryChange();
    m_num_rects = num_rects;
    m_rects = linspace(0.0, 1.0, m_num_rects);
    update();
}

std::string AdvancedLineItem::key()
{
    return m_key;
}

AdvancedLineItem::State& AdvancedLineItem::state()
{
    return m_state;
}
