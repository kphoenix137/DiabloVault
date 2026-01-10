#pragma once

#include <QAbstractTableModel>

#include <vector>

#include "Workspace.h"

class ItemModel final : public QAbstractTableModel {
	Q_OBJECT

public:
	explicit ItemModel(QObject* parent = nullptr);

	void setItems(const std::vector<dv::ItemRecord> &items);

	int rowCount(const QModelIndex& parent = {}) const override;
	int columnCount(const QModelIndex& parent = {}) const override;
	QVariant data(const QModelIndex& index, int role) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
	std::vector<dv::ItemRecord> items_;
};
