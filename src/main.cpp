#include <QApplication>
#include <QLayout>
#include <QMainWindow>

#include "advanced_item_registry.h"
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

        AdvancedLineItem* line1 = new AdvancedLineItem("line1", QPointF(50, 50), QPointF(250, 100));
        m_item_registry.add_item(line1);

        AdvancedLineItem* line2 = new AdvancedLineItem("line2", QPointF(100, 400), QPointF(400, 300));
        m_item_registry.add_item(line2);

        auto item_opt = m_item_registry.get_item_entry<AdvancedLineItem>("line1");
        if (item_opt) {
            layout->addWidget(item_opt->widget);
        }

        m_display_widget->scene()->addItem(line1);
        m_display_widget->scene()->addItem(line2);

        setWindowTitle("Line Detector");
    }

    ~MainWindow() = default;

private:
    DisplayWidget* m_display_widget;

    AdvancedItemRegistry m_item_registry;
};

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    MainWindow win;
    win.show();

    return app.exec();
}
