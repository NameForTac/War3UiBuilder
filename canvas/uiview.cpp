#include "uiview.h"
#include "uiscene.h"

#include <QWheelEvent>
#include <QPainter>
#include <QScrollBar>

UiView::UiView(UiScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent)
{
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setDragMode(QGraphicsView::RubberBandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    setAcceptDrops(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

void UiView::zoomIn()
{
    const double factor = 1.25;
    if (m_zoomFactor * factor <= MAX_ZOOM) {
        scale(factor, factor);
        m_zoomFactor *= factor;
    }
}

void UiView::zoomOut()
{
    const double factor = 1.0 / 1.25;
    if (m_zoomFactor * factor >= MIN_ZOOM) {
        scale(factor, factor);
        m_zoomFactor *= factor;
    }
}

void UiView::fitAll()
{
    UiScene *s = qobject_cast<UiScene *>(scene());
    if (s && !s->items().isEmpty()) {
        fitInView(s->itemsBoundingRect(), Qt::KeepAspectRatio);
        m_zoomFactor = transform().m11();
    } else {
        resetTransform();
        m_zoomFactor = 1.0;
    }
}

void UiView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        const double factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
        double newZoom = m_zoomFactor * factor;
        if (newZoom >= MIN_ZOOM && newZoom <= MAX_ZOOM) {
            scale(factor, factor);
            m_zoomFactor = newZoom;
        }
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

static constexpr double SCREEN_W = 1920.0;
static constexpr double SCREEN_H = 1080.0;

void UiView::drawBackground(QPainter *painter, const QRectF &rect)
{
    // Dark background for off-screen area
    painter->fillRect(rect, QColor(45, 45, 48));

    // White background for the visible screen area
    QRectF screenRect(0, 0, SCREEN_W, SCREEN_H);
    painter->fillRect(screenRect, QColor(245, 245, 240));

    // Grid
    const int gridSize = 20;
    QPen gridPen(QColor(80, 80, 85), 0.5);

    qreal left = int(rect.left()) - (int(rect.left()) % gridSize);
    qreal top = int(rect.top()) - (int(rect.top()) % gridSize);

    QVector<QLineF> lines;
    for (qreal x = left; x < rect.right(); x += gridSize) {
        lines.append(QLineF(x, rect.top(), x, rect.bottom()));
    }
    for (qreal y = top; y < rect.bottom(); y += gridSize) {
        lines.append(QLineF(rect.left(), y, rect.right(), y));
    }
    painter->setPen(gridPen);
    painter->drawLines(lines);

    // Screen border
    painter->setPen(QPen(QColor(0, 160, 255), 2));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(screenRect);

    // Screen dimension label
    painter->setPen(QColor(0, 160, 255));
    painter->drawText(screenRect.adjusted(-200, -24, -4, 0),
                      Qt::AlignRight | Qt::AlignBottom,
                      QString("Screen 1920 x 1080"));
}
