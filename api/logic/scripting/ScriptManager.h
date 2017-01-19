#pragma once

#include <QAbstractListModel>
#include <QSet>
#include <memory>

#include "multimc_logic_export.h"

class QDir;

class EntityProviderManager;
class Script;
class Task;

// TODO some form of automatic reloading for script development/debuggning?
class MULTIMC_LOGIC_EXPORT ScriptManager : public QAbstractListModel
{
public:
	explicit ScriptManager(QObject *parent = nullptr);

	EntityProviderManager *entityProviderManager() const { return m_entityProviderManager; }

	void registerScript(Script *script);
	void unregisterScript(Script *script);
	void loadScripts(const QDir &scriptDir);

	QVector<Script *> scripts() const { return m_scripts; }
	Script *script(const QModelIndex &index) const;

	std::unique_ptr<Task> createScriptUpdateTask() const;

private:
	EntityProviderManager *m_entityProviderManager;
	QVector<Script *> m_scripts;

	// QAbstractItemModel interface
public:
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
};
