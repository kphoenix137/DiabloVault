#include "ContainerModel.h"

#include <QDir>
#include <QFileInfo>

ContainerModel::ContainerModel(QObject* parent)
	: QAbstractItemModel(parent)
{
}

void ContainerModel::setWorkspaceRoot(const QString& rootDir)
{
	beginResetModel();

	rootDir_.clear();
	containerNames_.clear();
	containerPaths_.clear();

	const QDir root(rootDir);
	if (root.exists()) {
		rootDir_ = QDir::cleanPath(root.absolutePath());

		// "Containers" = immediate subdirectories (non-hidden, not . / ..)
		const QFileInfoList dirs = root.entryInfoList(
			QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable | QDir::NoSymLinks,
			QDir::Name | QDir::IgnoreCase
		);

		for (const QFileInfo& fi : dirs) {
			containerNames_.push_back(fi.fileName());
			containerPaths_.push_back(fi.absoluteFilePath());
		}

		// If there are no subdirectories, treat the root itself as a single container.
		if (containerNames_.isEmpty()) {
			containerNames_.push_back(QFileInfo(rootDir_).fileName().isEmpty()
				? rootDir_
				: QFileInfo(rootDir_).fileName());
			containerPaths_.push_back(rootDir_);
		}
	}

	endResetModel();
}

QString ContainerModel::containerPathForIndex(const QModelIndex& index) const
{
	if (!index.isValid() || index.parent().isValid() || index.column() != 0)
		return {};
	const int row = index.row();
	if (row < 0 || row >= containerPaths_.size())
		return {};
	return containerPaths_[row];
}

QModelIndex ContainerModel::index(int row, int column, const QModelIndex& parent) const
{
	if (column != 0)
		return {};
	if (parent.isValid())
		return {}; // single-level tree
	if (row < 0 || row >= containerNames_.size())
		return {};
	return createIndex(row, column, nullptr);
}

QModelIndex ContainerModel::parent(const QModelIndex& /*child*/) const
{
	// Single-level: all items have invalid parent.
	return {};
}

int ContainerModel::rowCount(const QModelIndex& parent) const
{
	if (parent.isValid())
		return 0;
	return containerNames_.size();
}

int ContainerModel::columnCount(const QModelIndex& /*parent*/) const
{
	return 1;
}

QVariant ContainerModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid() || index.parent().isValid() || index.column() != 0)
		return {};

	const int row = index.row();
	if (row < 0 || row >= containerNames_.size())
		return {};

	if (role == Qt::DisplayRole)
		return containerNames_[row];

	if (role == Qt::ToolTipRole)
		return containerPaths_[row];

	return {};
}

Qt::ItemFlags ContainerModel::flags(const QModelIndex& index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}
