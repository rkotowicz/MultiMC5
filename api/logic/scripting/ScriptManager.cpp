#include "ScriptManager.h"

#include <QDir>

#include "Script.h"
#include "EntityProviderManager.h"

ScriptManager::ScriptManager(QObject *parent)
	: QAbstractListModel(parent), m_entityProviderManager(new EntityProviderManager(this)) {}

void ScriptManager::registerScript(Script *script)
{
	Q_ASSERT(!m_scripts.contains(script));
	script->setParent(this);
	script->setManagers(m_entityProviderManager);

	beginInsertRows(QModelIndex(), m_scripts.size(), m_scripts.size());
	m_scripts.append(script);
	endInsertRows();

	connect(script, &Script::metadataUpdated, this, [this, script]()
	{
		const int row = m_scripts.indexOf(script);
		emit dataChanged(index(row, 0), index(row, columnCount()-1), QVector<int>() << Qt::DisplayRole);
	});

	script->reload();
}
void ScriptManager::unregisterScript(Script *script)
{
	Q_ASSERT(m_scripts.contains(script));

	beginRemoveRows(QModelIndex(), m_scripts.indexOf(script), m_scripts.indexOf(script));
	m_scripts.removeOne(script);
	endRemoveRows();

	script->deleteLater();
}

void ScriptManager::loadScripts(const QDir &scriptDir)
{
	for (const QString &filename : scriptDir.entryList(QStringList() << "*.lua", QDir::Files))
	{
		const QString absoluteFilename = scriptDir.absoluteFilePath(filename);
		auto existingIt = std::find_if(m_scripts.begin(), m_scripts.end(), [absoluteFilename](const Script *s) { return s->filename() == absoluteFilename; });
		if (existingIt == m_scripts.end())
		{
			registerScript(new Script(absoluteFilename, this));
		}
	}

	for (Script *script : m_scripts)
	{
		script->reload();
	}
}

Script *ScriptManager::script(const QModelIndex &index) const
{
	return m_scripts.at(index.row());
}

std::unique_ptr<Task> ScriptManager::createScriptUpdateTask() const
{
	return nullptr; // TODO gather urls from the scripts and re-download them
}


int ScriptManager::rowCount(const QModelIndex &parent) const
{
	return m_scripts.size();
}
int ScriptManager::columnCount(const QModelIndex &parent) const
{
	return 3;
}

QVariant ScriptManager::data(const QModelIndex &index, int role) const
{
	const Script *script = m_scripts.at(index.row());
	if (role == Qt::DisplayRole)
	{
		switch (index.column())
		{
		case 0: return script->name();
		case 1: return script->version();
		case 2: return script->author();
		}
	}
	else if (role == Qt::ToolTipRole)
	{
		return script->id();
	}
	return QVariant();
}
QVariant ScriptManager::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
	{
		switch (section)
		{
		case 0: return tr("Name");
		case 1: return tr("Version");
		case 2: return tr("Author");
		}
	}
	return QVariant();
}
