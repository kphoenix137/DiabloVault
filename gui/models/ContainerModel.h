#pragma once

#include <QAbstractItemModel>

#include <vector>

#include "Workspace.h"

class ContainerModel final : public QAbstractItemModel {
	Q_OBJECT

public:
	explicit ContainerModel(QObject* parent = nullptr);

	void setContainers(const std::vector<dv::Container> &containers);

	// Returns container ID for index (or empty if invalid).
	QString containerIdForIndex(const QModelIndex& index) const;

	// Returns absolute path for the container at index (or empty if invalid).
	QString containerPathForIndex(const QModelIndex& index) const;

	// QAbstractItemModel
	QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;
	QModelIndex parent(const QModelIndex& child) const override;
	int rowCount(const QModelIndex& parent = {}) const override;
	int columnCount(const QModelIndex& parent = {}) const override;
	QVariant data(const QModelIndex& index, int role) const override;
	Qt::ItemFlags flags(const QModelIndex& index) const override;

private:
	std::vector<dv::Container> containers_;

};
