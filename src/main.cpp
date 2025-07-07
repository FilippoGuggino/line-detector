#include <QApplication>
#include <QLayout>
#include <QMainWindow>

#include "advanced_line_item.h"
#include "display_widget.h"

class MainWindow : public QMainWindow {
public:
    MainWindow(QWidget* parent = nullptr)
        : QMainWindow(parent)
    {
        QWidget* centralWidget = new QWidget(this);
        QHBoxLayout* layout = new QHBoxLayout();
        centralWidget->setLayout(layout);
        setCentralWidget(centralWidget);

        m_display_widget = new DisplayWidget(this);
        layout->addWidget(m_display_widget);

        AdvancedLineItem* line1 = new AdvancedLineItem(QPointF(50, 50), QPointF(250, 100));
        AdvancedLineItem* line2 = new AdvancedLineItem(QPointF(100, 400), QPointF(400, 300));

        m_display_widget->scene()->addItem(line1);
        m_display_widget->scene()->addItem(line2);

        setWindowTitle("Line Detector");
    }

    ~MainWindow() = default;

private:
    DisplayWidget* m_display_widget;
};

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    MainWindow win;
    win.show();

    return app.exec();
}
