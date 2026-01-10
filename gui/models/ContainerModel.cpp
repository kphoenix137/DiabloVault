#include "ContainerModel.h"

#include <QString>

ContainerModel::ContainerModel(QObject* parent)
	: QAbstractItemModel(parent)
{
}

void ContainerModel::setContainers(const std::vector<dv::Container> &containers)
{
	beginResetModel();
	containers_ = containers;
	endResetModel();
}

QString ContainerModel::containerIdForIndex(const QModelIndex& index) const
{
	if (!index.isValid() || index.parent().isValid() || index.column() != 0)
		return {};
	const int row = index.row();
	if (row < 0 || static_cast<size_t>(row) >= containers_.size())
		return {};
	return QString::fromStdString(containers_[row].id);
}

QString ContainerModel::containerPathForIndex(const QModelIndex& index) const
{
	if (!index.isValid() || index.parent().isValid() || index.column() != 0)
		return {};
	const int row = index.row();
	if (row < 0 || static_cast<size_t>(row) >= containers_.size())
		return {};
	return QString::fromStdString(containers_[row].path);
}

QModelIndex ContainerModel::index(int row, int column, const QModelIndex& parent) const
{
	if (column != 0)
		return {};
	if (parent.isValid())
		return {}; // single-level tree
	if (row < 0 || static_cast<size_t>(row) >= containers_.size())
		return {};
	return createIndex(row, column, nullptr);
}

QModelIndex ContainerModel::parent(const QModelIndex& /*child*/) const
{
	return {}; // single-level
}

int ContainerModel::rowCount(const QModelIndex& parent) const
{
	if (parent.isValid())
		return 0;
	return static_cast<int>(containers_.size());
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
	if (row < 0 || static_cast<size_t>(row) >= containers_.size())
		return {};
	const dv::Container &c = containers_[row];

	if (role == Qt::DisplayRole)
		return QString::fromStdString(c.displayName);

	if (role == Qt::ToolTipRole)
		return QString::fromStdString(c.path);

	return {};
}

Qt::ItemFlags ContainerModel::flags(const QModelIndex& index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}
