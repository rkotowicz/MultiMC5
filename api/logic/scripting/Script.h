#pragma once

#include <QObject>
#include <functional>

#include "Exception.h"
#include "tasks/Task.h"
#include "LuaFwd.h"

#include "multimc_logic_export.h"

class EntityProviderManager;

class ScriptLoadException : public Exception
{
public:
	using Exception::Exception;
};

class Script;
class QDir;

/// A task provided by a script
class ScriptTask : public Task
{
public:
	using Function = std::function<void(ScriptTask *)>;
	explicit ScriptTask(const Function &f, Script *script, QObject *parent);

	void executeTask() override;

	sol::table taskContext(const QDir &contextDir);

private:
	Function m_func;
	Script *m_script;
};

class MULTIMC_LOGIC_EXPORT Script : public QObject
{
	Q_OBJECT
public:
	explicit Script(const QString &filename, QObject *parent = nullptr);

	QString filename() const { return m_filename; }
	QString updateUrl() const { return m_updateUrl; }
	QString version() const { return m_version; }
	QString name() const { return m_name; }
	QString author() const { return m_author; }
	QString id () const { return m_id; }

	void reload();
	void load();
	void unload();

	void setManagers(EntityProviderManager *epm);

	sol::table staticContext();

signals:
	void metadataUpdated();

private:
	friend class ScriptTask;
	sol::state *m_lua;

	QString m_filename;
	bool m_isLoaded = false;
	EntityProviderManager *m_epm = nullptr;

	// metadata
	QString m_updateUrl;
	QString m_version;
	QString m_name;
	QString m_author;
	QString m_id;

	// data
	QVector<QObject *> m_scriptProvidedObjects;
};
