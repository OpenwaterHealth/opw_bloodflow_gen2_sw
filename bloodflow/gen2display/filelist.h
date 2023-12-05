#include <QAbstractListModel>
#include <QObject>
#include <QDebug>

class FileList : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit FileList(QObject* parent = nullptr);

    // Implement the required functions for the model
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool removeRows(int row, int count, const QModelIndex &parent) override;

    void clearModel();
    void addRow(QString patientName, QString scanTime, QString filePath);
    QString getPath(int index);
    QString getCompressableName(int index);

public slots:
    void testUpdate();

private:
    struct Item {
            QString patientName;
            QString scanTime;
            QString filePath;
        };
     QList<Item> itemList;
};

//enum FileListRoles {
//    nameRole = Qt::UserRole + 1,
//    timeRole,
//    pathRole
//};
