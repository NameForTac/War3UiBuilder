#ifndef UIVIEW_H
#define UIVIEW_H

#include <QGraphicsView>

class UiScene;

class UiView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit UiView(UiScene *scene, QWidget *parent = nullptr);
    ~UiView() = default;

public slots:
    void zoomIn();
    void zoomOut();
    void fitAll();

protected:
    void wheelEvent(QWheelEvent *event) override;
    void drawBackground(QPainter *painter, const QRectF &rect) override;

private:
signals:
    void zoomChanged(double factor);

private:
    double m_zoomFactor = 1.0;
    static constexpr double MIN_ZOOM = 0.1;
    static constexpr double MAX_ZOOM = 10.0;
};

#endif // UIVIEW_H
