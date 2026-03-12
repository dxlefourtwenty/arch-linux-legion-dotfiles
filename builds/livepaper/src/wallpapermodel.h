#ifndef WALLPAPERMODEL_H
#define WALLPAPERMODEL_H

#include <QAbstractListModel>
#include <QString>
#include <QVector>

struct WallpaperItem {
    QString path;
    QString thumbPath;
    QString fileName;
    QString label;
};

class WallpaperModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        PathRole = Qt::UserRole + 1,
        ThumbPathRole,
        FileNameRole,
        LabelRole
    };

    explicit WallpaperModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setItems(const QVector<WallpaperItem> &items);
    const WallpaperItem *itemAt(int index) const;

private:
    QVector<WallpaperItem> m_items;
};

#endif
