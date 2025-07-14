#include <QApplication>
#include <QLayout>
#include <QMainWindow>
#include <opencv2/opencv.hpp>

#include "advanced_line_item.h"
#include "display_widget.h"

static QImage to_qimage(const cv::Mat& m)
{
    return QImage((uint8_t*)m.data, m.cols, m.rows, m.step, QImage::Format_BGR888);
}

static cv::Mat rotate_image(const cv::Mat& src, double angle_deg)
{
    cv::Point2f center(src.cols / 2.0f, src.rows / 2.0f);

    cv::Rect2f bbox = cv::RotatedRect(center, src.size(), angle_deg).boundingRect();

    cv::Mat R = cv::getRotationMatrix2D(center, angle_deg, 1.0);
    R.at<double>(0, 2) += bbox.width / 2.0 - center.x;
    R.at<double>(1, 2) += bbox.height / 2.0 - center.y;

    cv::Mat dst;
    cv::warpAffine(src, dst, R, bbox.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT);

    return dst;
}

static cv::Rect get_padded_bbox(const std::vector<cv::Point2f>& points, cv::Size size, int kPadding = 0)
{
    cv::Rect bbox = cv::boundingRect(points);
    bbox.x = std::clamp<int>(bbox.x - kPadding, 0, size.width - 1);
    bbox.y = std::clamp<int>(bbox.y - kPadding, 0, size.height - 1);
    bbox.width = std::clamp<int>(bbox.width + kPadding, 1, size.width - bbox.x);
    bbox.height = std::clamp<int>(bbox.height + kPadding, 1, size.height - bbox.y);

    return bbox;
}

static cv::Mat get_translation_matrix(double tx, double ty)
{
    cv::Mat translation = (cv::Mat_<double>(3, 3) << 1.0, 0.0, tx,
        0.0, 1.0, ty,
        0.0, 0.0, 1.0);
    return translation;
}

static cv::Mat get_rotation_matrix(double angle_deg)
{
    double c = std::cos(angle_deg * M_PI / 180.0);
    double s = std::sin(angle_deg * M_PI / 180.0);

    cv::Mat R = (cv::Mat_<double>(3, 3) << c, -s, 0,
        s, c, 0,
        0, 0, 1);

    return R;
}

static std::vector<cv::Mat> get_tiles(cv::Mat bgr_img, const std::vector<cv::RotatedRect>& rects)
{
    // all angles should be the same
    double angle = rects[0].angle;

    std::vector<cv::Point2f> corners;
    // only first and last rects are necessary to compute the bounding box
    for (auto& rect : { *rects.begin(), *rects.rbegin() }) {
        std::array<cv::Point2f, 4> points;
        rect.points(points.data());
        corners.insert(std::end(corners), std::begin(points), std::end(points));
    }

    cv::Rect bbox = get_padded_bbox(corners, cv::Size(bgr_img.cols, bgr_img.rows), 5);

    cv::Mat cropped_img;
    bgr_img(bbox).copyTo(cropped_img);

    cv::Mat rotated_cropped_img = rotate_image(cropped_img, angle);

    cv::Point2f bbox_center(bbox.width / 2.0, bbox.height / 2.0);

    cv::Mat M = get_translation_matrix(rotated_cropped_img.cols / 2.0, rotated_cropped_img.rows / 2.0)
        * get_rotation_matrix(-angle)
        * get_translation_matrix(-bbox_center.x, -bbox_center.y)
        * get_translation_matrix(-bbox.x, -bbox.y);

    std::vector<cv::Mat> tiles;
    for (const auto& rect : rects) {
        std::array<cv::Point2f, 4> corners;
        // TODO: check that the order of points is as expected in every orientation i.e.: bottom-left, top-left, top-right, bottom-right
        rect.points(corners.data());

        // transformed corners, ordered clockwise starting from bottom-left corner
        std::array<cv::Point2f, 4> p;
        cv::perspectiveTransform(corners, p, M);

        cv::Rect bbox = get_padded_bbox({ p[1], p[3] }, cv::Size(rotated_cropped_img.cols, rotated_cropped_img.rows));

        cv::Mat tile;
        rotated_cropped_img(bbox).copyTo(tile);

        tiles.push_back(tile);
    }

    return tiles;
}

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

        cv::Mat bgr_img = cv::imread("/home/peano/workspace/line-detector/test-images/banana-bg.jpg", cv::IMREAD_ANYCOLOR);
        cv::resize(bgr_img, bgr_img, cv::Size(500, 500));

        QImage qimg = to_qimage(bgr_img);
        m_display_widget->set_pixmap(QPixmap::fromImage(qimg));

        AdvancedLineItem* line1 = new AdvancedLineItem(QPointF(50, 50), QPointF(250, 100));
        AdvancedLineItem* line2 = new AdvancedLineItem(QPointF(100, 400), QPointF(400, 300));

        m_display_widget->scene()->addItem(line1);
        m_display_widget->scene()->addItem(line2);

        {
            std::vector<cv::RotatedRect> rects = line1->get_rect_regions();
            std::vector<cv::Mat> tiles = get_tiles(bgr_img, rects);
        }
        {
            std::vector<cv::RotatedRect> rects = line2->get_rect_regions();
            std::vector<cv::Mat> tiles = get_tiles(bgr_img, rects);
        }

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
