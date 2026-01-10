#include "ItemModel.h"

#include <QString>

ItemModel::ItemModel(QObject* parent)
	: QAbstractTableModel(parent)
{
}

void ItemModel::setItems(const std::vector<dv::ItemRecord> &items)
{
	beginResetModel();
	items_ = items;
	endResetModel();
}

int ItemModel::rowCount(const QModelIndex& parent) const
{
	if (parent.isValid())
		return 0;
	return static_cast<int>(items_.size());
}

int ItemModel::columnCount(const QModelIndex& /*parent*/) const
{
	return 7; // Name, Base, Quality, Affixes, ilvl, Req, Location
}

QVariant ItemModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return {};

	const int row = index.row();
	const int col = index.column();
	if (row < 0 || static_cast<size_t>(row) >= items_.size())
		return {};
	const dv::ItemRecord &r = items_[row];

	if (role == Qt::DisplayRole) {
		switch (col) {
		case 0: return QString::fromStdString(r.name);
		case 1: return QString::fromStdString(r.baseType);
		case 2: return QString::fromStdString(r.quality);
		case 3: return QString::fromStdString(r.affixes);
		case 4: return r.ilvl;
		case 5: return r.reqLvl;
		case 6: return QString::fromStdString(r.location);
		default: return {};
		}
	}

	if (role == Qt::ToolTipRole) {
		return QString::fromStdString(r.sourcePath);
	}

	return {};
}

QVariant ItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole)
		return {};
	if (orientation != Qt::Horizontal)
		return {};

	switch (section) {
	case 0: return "Name";
	case 1: return "Base";
	case 2: return "Quality";
	case 3: return "Affixes";
	case 4: return "ilvl";
	case 5: return "Req";
	case 6: return "Location";
	default: return {};
	}
}
