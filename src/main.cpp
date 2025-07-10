#include <QApplication>
#include <QLayout>
#include <QMainWindow>
#include <opencv2/opencv.hpp>

#include "advanced_line_item.h"
#include "display_widget.h"
#include "matplotlibcpp.h"

#include <QLabel>
#include <qdebug.h>

namespace plt = matplotlibcpp;

static QImage cv_mat_to_qimage(const cv::Mat& m)
{
    return QImage((uint8_t*)m.data, m.cols, m.rows, m.step, QImage::Format_BGR888);
}

static cv::Mat create_cv_mask(int height, int width, const std::array<QPointF, 4>& poly)
{
    cv::Mat mask(height, width, CV_8UC1, cv::Scalar(false));

    std::vector<cv::Point> contour;
    contour.reserve(poly.size());
    for (const QPointF& p : poly) {
        contour.emplace_back(static_cast<int>(p.x()), static_cast<int>(p.y()));
    }

    cv::fillConvexPoly(mask, contour, cv::Scalar(255));

    return mask;
}

// static cv::Mat create_cv_mask(int height, int width, const QPolygonF& poly)
// {
//     cv::Mat mask(height, width, CV_8UC1, cv::Scalar(false));

//     std::vector<cv::Point> contour;
//     contour.reserve(poly.size());
//     for (const QPointF& p : poly) {
//         contour.emplace_back(static_cast<int>(p.x()), static_cast<int>(p.y()));
//     }

//     cv::fillConvexPoly(mask, contour, true);

//     return mask;
// }

cv::Mat rotate_image(const cv::Mat& src, double angle_degrees)
{
    cv::Point2f center(src.cols / 2.0F, src.rows / 2.0F);

    cv::Mat R = cv::getRotationMatrix2D(center, angle_degrees, 1.0);

    cv::Rect2f bbox = cv::RotatedRect(center, src.size(), angle_degrees).boundingRect2f();

    R.at<double>(0, 2) += bbox.width / 2.0 - center.x;
    R.at<double>(1, 2) += bbox.height / 2.0 - center.y;

    cv::Mat dst;
    cv::warpAffine(src, dst, R, bbox.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT);

    return dst;
}

// static std::array<cv::Point2f, 4> polar_rect_to_cartesian_corners(const PolarCoordRectangle& rect)
// {
//     double angle_degrees = rect.angle * M_PI / 180.0f;
//     // Center in Cartesian
//     double cx = rect.r * std::cos(angle_degrees);
//     double cy = rect.r * std::sin(angle_degrees);

//     // Half extents
//     double half_w = rect.width / 2.0;
//     double half_h = rect.height / 2.0;

//     // Unit vectors in radial and tangential directions
//     double cosA = std::cos(angle_degrees);
//     double sinA = std::sin(angle_degrees);

//     // Tangential (perpendicular) direction
//     double tx = -sinA;
//     double ty = cosA;

//     // Corner offsets from center
//     std::array<cv::Point2f, 4> corners;

//     corners[0] = cv::Point2f(cx - half_w * tx - half_h * cosA, cy - half_w * ty - half_h * sinA); // bottom-left
//     corners[1] = cv::Point2f(cx + half_w * tx - half_h * cosA, cy + half_w * ty - half_h * sinA); // bottom-right
//     corners[2] = cv::Point2f(cx + half_w * tx + half_h * cosA, cy + half_w * ty + half_h * sinA); // top-right
//     corners[3] = cv::Point2f(cx - half_w * tx + half_h * cosA, cy - half_w * ty + half_h * sinA); // top-left

//     return corners;
// }
static QPointF get_rect_center(const std::array<QPointF, 4>& corners)
{
    double sumX = 0.0;
    double sumY = 0.0;

    for (const QPointF& pt : corners) {
        sumX += pt.x();
        sumY += pt.y();
    }

    return QPointF(sumX / 4.0, sumY / 4.0);
}

static cv::Rect get_bounding_rect(const std::vector<std::array<QPointF, 4>>& rects)
{
    std::vector<QPointF> all_corners;
    // TODO maybe only first and last rects are necessary
    for (auto& rect : rects) {
        QPointF rect_center = get_rect_center(rect);
        all_corners.insert(std::end(all_corners), std::begin(rect), std::end(rect));
    }

    auto [min_x, max_x] = std::minmax_element(std::begin(all_corners), std::end(all_corners), [](QPointF& p1, QPointF& p2) {
        return p1.x() < p2.x();
    });

    auto [min_y, max_y] = std::minmax_element(std::begin(all_corners), std::end(all_corners), [](QPointF& p1, QPointF& p2) {
        return p1.y() < p2.y();
    });

    return cv::Rect(min_x->x(), min_y->y(), max_x->x() - min_x->x(), max_y->y() - min_y->y());
}

static std::vector<cv::Mat> get_tiles(cv::Mat bgr_img, const AdvancedLineOutput& line_out)
{
    // cv::Mat rotated_img = rotate_image(bgr_img, polar_rects[0].angle);

    cv::Rect bbox = get_bounding_rect(line_out.rects);
    cv::Mat mask(bgr_img.rows, bgr_img.cols, CV_8UC1, cv::Scalar(0));

    mask(bbox).setTo(255);

    // for (auto& rect : line_out.rects) {
    //     cv::Mat line_mask = create_cv_mask(bgr_img.rows, bgr_img.cols, rect);
    //     cv::bitwise_not(line_mask, line_mask);
    //     cv::bitwise_and(mask, line_mask, mask);
    // }
    cv::Mat line_mask = create_cv_mask(bgr_img.rows, bgr_img.cols, line_out.rects[0]);
    cv::bitwise_not(line_mask, line_mask);
    cv::bitwise_and(mask, line_mask, mask);

    cv::Mat cropped_img;
    bgr_img(bbox).copyTo(cropped_img);

    unsigned int n_rows = 2;
    unsigned int n_cols = 3;
    unsigned int i = 1;

    // plt::subplot(n_rows, n_cols, i++);
    // plt::title("img");
    // plt::imshow(bgr_img);

    mask = rotate_image(mask, -line_out.angle);
    cv::Mat rotated_cropped_img = rotate_image(cropped_img, -line_out.angle);

    cv::Point2f bbox_center = (bbox.br() + bbox.tl()) * 0.5 - bbox.tl();
    cv::circle(cropped_img, bbox_center, 2, cv::Scalar(128));
    std::cout << "bbox center: " << "(" << bbox_center.x << ", " << bbox_center.y << ")" << std::endl;

    QTransform transform;
    transform.translate(rotated_cropped_img.cols / 2.0, rotated_cropped_img.rows / 2.0);
    transform.rotate(line_out.angle);
    transform.translate(-bbox_center.x, -bbox_center.y);
    transform.translate(-bbox.x, -bbox.y);
    std::cout << "bbox.x: " << bbox.x << " bbox.y: " << bbox.y << std::endl;
    for (const auto& rect : line_out.rects) {
        std::vector<QPointF> p;
        for (const QPointF& point : rect) {
            cv::circle(cropped_img, cv::Point2i(point.x(), point.y()), 2, cv::Scalar(128));
            QPointF transformed_p = transform.map(point);
            qDebug() << "point: " << point << " transformed point: " << transformed_p;
            // cv::circle(rotated_cropped_img, cv::Point2i(transformed_p.x(), transformed_p.y()), 2, cv::Scalar(128));
            p.push_back(transformed_p);
        }

        cv::Rect r(p[0].x(), p[0].y(), std::abs(p[1].x() - p[0].x()), std::abs(p[2].y() - p[1].y()));

        cv::Mat prova;
        rotated_cropped_img(r).copyTo(prova);
        plt::imshow(prova);
        plt::show();
    }

    // if (top_right_quadrant) {
    //     x = 0;
    //     y = width * sin(angle);
    // }
    // else if (top_left_quadrant) {
    //     x = width * cos(angle);
    //     y  = ?;
    // }
    // else if (bottom_left_quadrant) {
    //     x = width * cos(angle);
    //     y = 0;
    // }
    // else if (bottom_right_quadrant) {
    //     x = 0;
    //     y = height * sin(angle);
    // }

    // for (auto& rect : line_out.rects) {
    //     for (const QPointF& point : rect) {
    //         QPointF p = transform.map(point);
    //         qDebug() << "point: " << point << " transformed point: " << transform.map(point);
    //         cv::circle(mask, cv::Point2i(p.x(), p.y()), 2, cv::Scalar(128));
    //     }
    // }

    plt::subplot(n_rows, n_cols, i++);
    plt::title("mask");
    plt::imshow(mask);
    plt::subplot(n_rows, n_cols, i++);
    plt::title("cropped_img");
    plt::imshow(cropped_img);
    plt::subplot(n_rows, n_cols, i++);
    plt::title("rotated mask");
    plt::imshow(mask);
    plt::subplot(n_rows, n_cols, i++);
    plt::title("rotated cropped_img");
    plt::imshow(rotated_cropped_img);
    plt::show();

    // for (auto& polar_rect : polar_rects) {

    //     QRectF tile_region = poly.boundingRect();
    //     QPointF tile_center = tile_region.center();
    //     cv::Rect r(tile_region.x(), tile_region.y(), tile_region.width() + 2, tile_region.height() + 2);
    //     cv::Mat cropped_mask;
    //     mask(r).copyTo(cropped_mask);
    //     plt::imshow(cropped_mask);
    //     plt::show();

    //     unsigned int n_rows = 1;
    //     unsigned int n_cols = 3;
    //     unsigned int i = 1;

    //     plt::subplot(n_rows, n_cols, i++);
    //     plt::title("img");
    //     plt::imshow(bgr_img);
    //     plt::subplot(n_rows, n_cols, i++);
    //     plt::title("mask");
    //     plt::imshow(mask);
    //     plt::subplot(n_rows, n_cols, i++);
    //     plt::title("masked img");
    //     cv::Mat masked_img;
    //     cv::bitwise_and(bgr_img, bgr_img, masked_img, mask);
    //     plt::imshow(masked_img);
    //     plt::show();
    // }

    return std::vector<cv::Mat>();
}

// static std::vector<cv::Mat> get_tiles(cv::Mat bgr_img, std::vector<QPolygonF> polygons)
// {
//     for (auto& poly : polygons) {
//         cv::Mat mask = create_cv_mask(bgr_img.rows, bgr_img.cols, poly);

//         QRectF tile_region = poly.boundingRect();
//         QPointF tile_center = tile_region.center();
//         cv::Rect r(tile_region.x(), tile_region.y(), tile_region.width() + 2, tile_region.height() + 2);
//         cv::Mat cropped_mask;
//         mask(r).copyTo(cropped_mask);
//         plt::imshow(cropped_mask);
//         plt::show();

//         unsigned int n_rows = 1;
//         unsigned int n_cols = 3;
//         unsigned int i = 1;

//         plt::subplot(n_rows, n_cols, i++);
//         plt::title("img");
//         plt::imshow(bgr_img);
//         plt::subplot(n_rows, n_cols, i++);
//         plt::title("mask");
//         plt::imshow(mask);
//         plt::subplot(n_rows, n_cols, i++);
//         plt::title("masked img");
//         cv::Mat masked_img;
//         cv::bitwise_and(bgr_img, bgr_img, masked_img, mask);
//         plt::imshow(masked_img);
//         plt::show();
//     }

//     return std::vector<cv::Mat>();
// }

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

        QImage qimg = cv_mat_to_qimage(bgr_img);
        m_display_widget->set_pixmap(QPixmap::fromImage(qimg));

        AdvancedLineItem* line1 = new AdvancedLineItem(QPointF(50, 50), QPointF(250, 100));
        AdvancedLineItem* line2 = new AdvancedLineItem(QPointF(100, 400), QPointF(400, 300));

        m_display_widget->scene()->addItem(line1);
        m_display_widget->scene()->addItem(line2);

        // {
        //     AdvancedLineOutput pol = line1->get_rect_regions();
        //     std::vector<cv::Mat> tiles = get_tiles(bgr_img, pol);
        // }
        {
            AdvancedLineOutput pol = line2->get_rect_regions();
            std::vector<cv::Mat> tiles = get_tiles(bgr_img, pol);
        }
        // {
        //     std::vector<QPolygonF> pol = line1->get_rect_regions();
        //     std::vector<cv::Mat> tiles = get_tiles(bgr_img, pol);
        // }
        // {
        //     std::vector<QPolygonF> pol = line2->get_rect_regions();
        //     std::vector<cv::Mat> tiles = get_tiles(bgr_img, pol);
        // }

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
