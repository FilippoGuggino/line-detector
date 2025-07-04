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
    for (const auto& lineObject : m_advanced_lines) {
        lineObject.paint(&painter);
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
        m_active_line->update_cursor(event->pos(), this);
        update();
    }
    else {
        // No object is being dragged, so just set the default cursor
        setCursor(Qt::ArrowCursor);
    }
}

void DisplayWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_active_line) {
        m_active_line->handle_mouse_release();
        m_active_line = nullptr;
        setCursor(Qt::ArrowCursor);
        update();
    }
}
