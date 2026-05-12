#include "treepanel.h"
#include "elements/uielementdata.h"

#include <QVBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>
#include <QTimer>

static constexpr int COL_NAME    = 0;
static constexpr int COL_VISIBLE = 1;
static constexpr int COL_LOCK    = 2;
static constexpr int COL_COUNT   = 3;

TreePanel::TreePanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_tree = new QTreeWidget(this);
    m_tree->setColumnCount(COL_COUNT);
    m_tree->setHeaderLabels({tr("Elements"), QString(), QString()});
    m_tree->setAnimated(true);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tree->setDragDropMode(QAbstractItemView::InternalMove);

    m_tree->header()->setStretchLastSection(false);
    m_tree->header()->setSectionResizeMode(COL_NAME, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(COL_VISIBLE, QHeaderView::Fixed);
    m_tree->header()->setSectionResizeMode(COL_LOCK, QHeaderView::Fixed);
    m_tree->setColumnWidth(COL_VISIBLE, 24);
    m_tree->setColumnWidth(COL_LOCK, 24);

    layout->addWidget(m_tree);

    connect(m_tree, &QTreeWidget::itemClicked, this, &TreePanel::onItemClicked);
    connect(m_tree, &QTreeWidget::itemDoubleClicked, this, &TreePanel::onItemDoubleClicked);
    connect(m_tree, &QTreeWidget::customContextMenuRequested, this, &TreePanel::onCustomContextMenu);
    // Model structure change → deferred detection (rowsMoved may not fire for cross-parent D&D)
    auto *model = m_tree->model();
    connect(model, &QAbstractItemModel::rowsMoved, this, [this]() {
        QTimer::singleShot(0, this, &TreePanel::processPendingDrop);
    });
    connect(model, &QAbstractItemModel::rowsInserted, this, [this]() {
        QTimer::singleShot(0, this, &TreePanel::processPendingDrop);
    });
    connect(model, &QAbstractItemModel::rowsRemoved, this, [this]() {
        QTimer::singleShot(0, this, &TreePanel::processPendingDrop);
    });
}

void TreePanel::setItemStatus(QTreeWidgetItem *item, bool visible, bool locked)
{
    item->setText(COL_VISIBLE, visible ? QStringLiteral("○") : QString());
    item->setText(COL_LOCK, locked ? QStringLiteral("◆") : QString());
    item->setData(COL_VISIBLE, Qt::UserRole, visible);
    item->setData(COL_LOCK, Qt::UserRole, locked);
    // Dim name text when hidden
    QFont f = item->font(COL_NAME);
    f.setStrikeOut(!visible);
    item->setFont(COL_NAME, f);
}

void TreePanel::rebuildTree(const QList<UiElementData> &elements)
{
    m_tree->clear();
    if (elements.isEmpty()) {
        m_lastTreeState.clear();
        return;
    }

    QMap<QString, UiElementData> dataMap;
    for (const auto &el : elements)
        dataMap[el.name] = el;

    QMap<QString, QTreeWidgetItem *> itemMap;

    // Root elements
    for (const auto &el : elements) {
        if (el.parent.isEmpty() || !dataMap.contains(el.parent)) {
            auto *item = new QTreeWidgetItem(m_tree);
            item->setText(COL_NAME, el.name);
            item->setData(COL_NAME, Qt::UserRole, el.name);
            item->setToolTip(COL_NAME, QString("%1 (%2x%3)").arg(el.name).arg(el.width).arg(el.height));
            setItemStatus(item, el.visible, el.locked);
            itemMap[el.name] = item;
        }
    }

    // Children
    for (const auto &el : elements) {
        if (!el.parent.isEmpty() && itemMap.contains(el.parent)) {
            auto *child = new QTreeWidgetItem(itemMap[el.parent]);
            child->setText(COL_NAME, el.name);
            child->setData(COL_NAME, Qt::UserRole, el.name);
            child->setToolTip(COL_NAME, QString("%1 (%2x%3)").arg(el.name).arg(el.width).arg(el.height));
            setItemStatus(child, el.visible, el.locked);
            itemMap[el.name] = child;
        }
    }

    m_tree->expandAll();
    saveTreeState();
}

void TreePanel::selectElement(const QString &name)
{
    auto *item = findItemByName(name);
    if (item) {
        m_tree->setCurrentItem(item);
    }
}

void TreePanel::refreshTree(const QList<UiElementData> &elements)
{
    auto *current = m_tree->currentItem();
    QString selectedName;
    if (current)
        selectedName = current->data(COL_NAME, Qt::UserRole).toString();

    rebuildTree(elements);

    if (!selectedName.isEmpty()) {
        auto *item = findItemByName(selectedName);
        if (item)
            m_tree->setCurrentItem(item);
    }
}

void TreePanel::clearPanel()
{
    m_tree->clear();
    m_lastTreeState.clear();
}

void TreePanel::processPendingDrop()
{
    if (m_checkPending) return;
    m_checkPending = true;

    QMap<QString, QString> currentState;
    scanTreeState(m_tree->invisibleRootItem(), QString(), currentState);

    for (auto it = currentState.begin(); it != currentState.end(); ++it) {
        QString prevParent = m_lastTreeState.value(it.key());
        if (prevParent != it.value()) {
            m_checkPending = false;
            emit parentChanged(it.key(), it.value());
            break; // tree was rebuilt by handler — re-scan below
        }
    }

    // Re-scan after potential rebuilds from parentChanged signals
    m_lastTreeState.clear();
    scanTreeState(m_tree->invisibleRootItem(), QString(), m_lastTreeState);
    m_checkPending = false;
}

void TreePanel::scanTreeState(QTreeWidgetItem *item, const QString &parentName,
                               QMap<QString, QString> &state) const
{
    for (int i = 0; i < item->childCount(); ++i) {
        QTreeWidgetItem *child = item->child(i);
        QString name = child->data(COL_NAME, Qt::UserRole).toString();
        if (!name.isEmpty()) {
            state[name] = parentName;
        }
        scanTreeState(child, name, state);
    }
}

void TreePanel::saveTreeState()
{
    m_lastTreeState.clear();
    scanTreeState(m_tree->invisibleRootItem(), QString(), m_lastTreeState);
}

void TreePanel::onItemClicked(QTreeWidgetItem *item, int column)
{
    if (!item) return;

    QString name = item->data(COL_NAME, Qt::UserRole).toString();

    if (column == COL_VISIBLE) {
        bool vis = !item->data(COL_VISIBLE, Qt::UserRole).toBool();
        setItemStatus(item, vis, item->data(COL_LOCK, Qt::UserRole).toBool());
        emit visibilityToggled(name, vis);
    } else if (column == COL_LOCK) {
        bool locked = !item->data(COL_LOCK, Qt::UserRole).toBool();
        setItemStatus(item, item->data(COL_VISIBLE, Qt::UserRole).toBool(), locked);
        emit lockToggled(name, locked);
    } else {
        emit elementSelected(name);
    }
}

void TreePanel::onItemDoubleClicked(QTreeWidgetItem *item, int)
{
    if (!item) return;

    QString oldName = item->data(COL_NAME, Qt::UserRole).toString();
    bool ok;
    QString newName = QInputDialog::getText(this, tr("Rename"),
                                              tr("Element name:"), QLineEdit::Normal,
                                              oldName, &ok);
    if (ok && !newName.isEmpty() && newName != oldName) {
        emit renameRequested(oldName, newName);
    }
}

void TreePanel::onCustomContextMenu(const QPoint &pos)
{
    auto *item = m_tree->itemAt(pos);
    if (!item) return;

    QString name = item->data(COL_NAME, Qt::UserRole).toString();

    QMenu menu;
    QAction *renameAct = menu.addAction(tr("Rename"));
    QAction *deleteAct = menu.addAction(tr("Delete"));

    QAction *selected = menu.exec(m_tree->mapToGlobal(pos));
    if (selected == renameAct) {
        onItemDoubleClicked(item, 0);
    } else if (selected == deleteAct) {
        emit deleteRequested(name);
    }
}

QTreeWidgetItem *TreePanel::findItemByName(const QString &name) const
{
    QList<QTreeWidgetItem *> items = m_tree->findItems(name, Qt::MatchExactly | Qt::MatchRecursive, COL_NAME);
    return items.isEmpty() ? nullptr : items.first();
}
