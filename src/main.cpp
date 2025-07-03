#include <QApplication>
#include <QMainWindow>

class MainWindow : public QMainWindow {
public:
    MainWindow(QWidget* parent = nullptr)
        : QMainWindow(parent)
    {
    }

    ~MainWindow() = default;

private:
};

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    MainWindow win;
    win.show();

    return app.exec();
}
