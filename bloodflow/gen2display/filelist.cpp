#include "filelist.h"
#include <QProcess>

FileList::FileList(QObject* parent) : QAbstractListModel(parent)
{

}

int FileList::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return itemList.count();
}

QVariant FileList::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= itemList.count())
        return QVariant();

    const Item& item = itemList[index.row()];
    if (role == Qt::DisplayRole) {
        // Return the data for the display role (name and scan)
        return item.patientName + " - " + item.scanTime;
    }

    return QVariant();
}
void FileList::clearModel()
{
    // Clear all items from the model
    itemList.clear();
    testUpdate();
}

void FileList::addRow(QString patientName, QString scanTime, QString filePath){
    itemList.append({patientName,scanTime,filePath});
    testUpdate();
}

QString FileList::getPath(int index){
    return itemList[index].filePath;
}

QString FileList::getCompressableName(int index){
    return itemList[index].patientName + "." + itemList[index].scanTime;
}

bool FileList::removeRows(int row, int count, const QModelIndex &parent = QModelIndex())  {
    Q_UNUSED(parent);
    QProcess process;

    QString filePath = getPath(row);
    QString remove_tool = "rm";
    QStringList arguments;
    arguments << "-rf";
    arguments << filePath;
    qDebug() << "Deleting: " << filePath << " at index " << row;
    process.start(remove_tool, arguments);
    process.waitForFinished();

    beginRemoveRows(QModelIndex(), row, row + count - 1);
    while (count--) itemList.removeAt(row);
    endRemoveRows();
    return true;
}

void FileList::testUpdate() {
    QModelIndex index = createIndex(0, 0);
    emit dataChanged(index, index);
}
