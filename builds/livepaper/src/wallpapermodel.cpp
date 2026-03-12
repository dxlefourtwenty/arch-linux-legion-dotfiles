#include "wallpapermodel.h"

WallpaperModel::WallpaperModel(QObject *parent)
    : QAbstractListModel(parent) {}

int WallpaperModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_items.size();
}

QVariant WallpaperModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) return {};
    const int row = index.row();
    if (row < 0 || row >= m_items.size()) return {};

    const WallpaperItem &item = m_items[row];
    switch (role) {
    case PathRole:
        return item.path;
    case ThumbPathRole:
        return item.thumbPath;
    case FileNameRole:
        return item.fileName;
    case LabelRole:
        return item.label;
    default:
        return {};
    }
}

QHash<int, QByteArray> WallpaperModel::roleNames() const {
    return {
        {PathRole, "path"},
        {ThumbPathRole, "thumbPath"},
        {FileNameRole, "fileName"},
        {LabelRole, "label"}
    };
}

void WallpaperModel::setItems(const QVector<WallpaperItem> &items) {
    beginResetModel();
    m_items = items;
    endResetModel();
}

const WallpaperItem *WallpaperModel::itemAt(int index) const {
    if (index < 0 || index >= m_items.size()) return nullptr;
    return &m_items[index];
}
