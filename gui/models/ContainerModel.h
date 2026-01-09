#pragma once

#include <QAbstractItemModel>
#include <QStringList>

class ContainerModel final : public QAbstractItemModel {
	Q_OBJECT

public:
	explicit ContainerModel(QObject* parent = nullptr);

	// Sets the workspace root and enumerates immediate subdirectories as "containers".
	// If no subdirectories exist, the root itself becomes the only container.
	void setWorkspaceRoot(const QString& rootDir);

	QString workspaceRoot() const { return rootDir_; }

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
	QString rootDir_;
	QStringList containerNames_;  // display names
	QStringList containerPaths_;  // absolute paths; same length as containerNames_
};
