#include <QPainter>

#include "display_widget.h"

DisplayWidget::DisplayWidget(QWidget* parent)
    : QWidget(parent)
    , m_active_line(nullptr)
{
    setMinimumSize(500, 500);
    setMouseTracking(true);

    // Create two independent line objects to demonstrate the functionality
    m_advanced_lines.append(AdvancedLine(QPoint(50, 50), QPoint(250, 100)));
    m_advanced_lines.append(AdvancedLine(QPoint(100, 400), QPoint(400, 300)));
}

void DisplayWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Tell each line object to draw itself
    for (const auto& line_object : m_advanced_lines) {
        line_object.paint(&painter);
    }
}

void DisplayWidget::mousePressEvent(QMouseEvent* event)
{
    m_active_line = nullptr;
    // Iterate backwards so the top-most object is checked first
    for (int i = m_advanced_lines.size() - 1; i >= 0; --i) {
        if (m_advanced_lines[i].handle_mouse_press(event->pos())) {
            m_active_line = &m_advanced_lines[i];
            update();
            return; // Stop after finding the first object that handles the press
        }
    }
}

void DisplayWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_active_line) {
        m_active_line->handle_mouse_move(event->pos());
        update();
    }

    Qt::CursorShape current_cursor = Qt::ArrowCursor;

    if (m_active_line) {
        // If dragging, get the cursor from the active line
        current_cursor = m_active_line->get_cursor_for_position(event->pos());
    }
    else {
        // If not dragging (hovering), check all lines to see if we are over one
        // Iterate backwards so the top-most object is checked first
        for (int i = m_advanced_lines.size() - 1; i >= 0; --i) {
            Qt::CursorShape shape = m_advanced_lines[i].get_cursor_for_position(event->pos());
            if (shape != Qt::ArrowCursor) {
                current_cursor = shape;
                break; // Found an object to interact with, stop checking
            }
        }
    }

    setCursor(current_cursor);
}

void DisplayWidget::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    if (m_active_line) {
        m_active_line->handle_mouse_release();
        m_active_line = nullptr;
        // After release, we might be hovering over something else, so we need to
        // re-evaluate the cursor for the new position.
        mouseMoveEvent(event);
        update();
    }
}
