#ifndef DISPLAY_WIDGET_H
#define DISPLAY_WIDGET_H

#include <QList>
#include <QMouseEvent>
#include <QWidget>

#include "advanced_line.h" // Include our new class

class DisplayWidget : public QWidget {
    Q_OBJECT

public:
    explicit DisplayWidget(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    QList<AdvancedLine> m_advanced_lines;
    AdvancedLine* m_active_line; // Pointer to the object currently being dragged
};

#endif /* DISPLAY_WIDGET_H */
