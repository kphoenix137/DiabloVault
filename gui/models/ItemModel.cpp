#include "ItemModel.h"

#include <QDir>
#include <QFileInfo>
#include <QLocale>

ItemModel::ItemModel(QObject* parent)
	: QAbstractTableModel(parent)
{
}

void ItemModel::setDirectory(const QString& dirPath)
{
	beginResetModel();

	dirPath_.clear();
	rows_.clear();

	const QDir dir(dirPath);
	if (dir.exists()) {
		dirPath_ = QDir::cleanPath(dir.absolutePath());

		// Show files (and optionally dirs) in the selected container.
		const QFileInfoList entries = dir.entryInfoList(
			QDir::Files | QDir::Readable | QDir::NoSymLinks | QDir::Dirs | QDir::NoDotAndDotDot,
			QDir::DirsFirst | QDir::Name | QDir::IgnoreCase
		);

		rows_.reserve(entries.size());
		for (const QFileInfo& fi : entries) {
			Row r;
			r.name = fi.fileName();
			r.absolutePath = fi.absoluteFilePath();
			r.isDir = fi.isDir();
			r.sizeBytes = r.isDir ? 0 : fi.size();
			r.modified = fi.lastModified();
			rows_.push_back(std::move(r));
		}
	}

	endResetModel();
}

int ItemModel::rowCount(const QModelIndex& parent) const
{
	if (parent.isValid())
		return 0;
	return rows_.size();
}

int ItemModel::columnCount(const QModelIndex& /*parent*/) const
{
	// Name | Size | Modified
	return 3;
}

QVariant ItemModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return {};

	const int row = index.row();
	const int col = index.column();
	if (row < 0 || row >= rows_.size() || col < 0 || col >= 3)
		return {};

	const Row& r = rows_[row];

	if (role == Qt::DisplayRole) {
		switch (col) {
		case 0: return r.name;
		case 1:
			if (r.isDir) return QStringLiteral("<dir>");
			return QLocale().formattedDataSize(r.sizeBytes);
		case 2:
			return r.modified.isValid()
				? QLocale().toString(r.modified, QLocale::ShortFormat)
				: QString{};
		}
	}

	if (role == Qt::ToolTipRole) {
		return r.absolutePath;
	}

	return {};
}

QVariant ItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole)
		return {};

	if (orientation == Qt::Horizontal) {
		switch (section) {
		case 0: return QStringLiteral("Name");
		case 1: return QStringLiteral("Size");
		case 2: return QStringLiteral("Modified");
		default: return {};
		}
	}

	return {};
}
