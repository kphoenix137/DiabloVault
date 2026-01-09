#pragma once

#include <QAbstractTableModel>
#include <QDateTime>
#include <QString>
#include <QVector>

class ItemModel final : public QAbstractTableModel {
	Q_OBJECT

public:
	explicit ItemModel(QObject* parent = nullptr);

	void setDirectory(const QString& dirPath);
	QString directory() const { return dirPath_; }

	// QAbstractTableModel
	int rowCount(const QModelIndex& parent = {}) const override;
	int columnCount(const QModelIndex& parent = {}) const override;
	QVariant data(const QModelIndex& index, int role) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
	struct Row {
		QString name;
		QString absolutePath;
		qint64 sizeBytes = 0;
		QDateTime modified;
		bool isDir = false;
	};

	QString dirPath_;
	QVector<Row> rows_;
};
