#include <QApplication>
#include <QLayout>
#include <QMainWindow>

#include "display_widget.h"

class MainWindow : public QMainWindow {
public:
    MainWindow(QWidget* parent = nullptr)
        : QMainWindow(parent)
    {

        QWidget* centralWidget = new QWidget(this);
        QVBoxLayout* layout = new QVBoxLayout(centralWidget);

        m_display_widget = new DisplayWidget(this);
        layout->addWidget(m_display_widget);

        setCentralWidget(centralWidget);
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
