#pragma once

#include <QObject>
#include <QVector>
#include <QHash>
#include <memory>

#include "multimc_logic_export.h"
#include "LuaFwd.h"

class Task;
class BaseVersionList;
class BaseVersion;
class BaseInstance;
class Script;

class MULTIMC_LOGIC_EXPORT EntityProvider : public QObject
{
	Q_OBJECT
public:
	struct Entity
	{
		EntityProvider *provider;
		QString internalId;
		QString name;
		QString iconUrl;

		enum Type
		{
			Mod,
			Modpack,
			TexturePack,
			World,
			Patch,
			Other
		} type = Other;

		QVariantHash extraData;

		bool isNull() const { return internalId.isNull(); }
		std::shared_ptr<BaseVersionList> versionList() const { return provider->versionList(*this); }
		sol::table toTable(sol::state_view &state) const;

		static Entity fromTable(EntityProvider *provider, const sol::table &table);
	};

	explicit EntityProvider(const sol::table &table, Script *script);

	QString id() const { return m_id; }
	Script *script() const { return m_script; }

	std::unique_ptr<Task> createUpdateEntitiesTask();

	QVector<Entity> entities() const;
	std::shared_ptr<BaseVersionList> versionList(const Entity &entity);
	std::unique_ptr<Task> createInstallTask(BaseInstance *instance, const std::shared_ptr<BaseVersion> &version);

signals:
	void beforeEntitiesUpdate();
	void afterEntitiesUpdate();

private:
	friend class UpdateEntitiesTask;

	Script *m_script;
	QString m_id;
	QVector<Entity> m_entities;
	QVector<Entity> m_staticEntities;
	QHash<QString, std::shared_ptr<BaseVersionList>> m_versionLists;
	std::function<QVector<Entity>(sol::table)> m_entitiesUpdateFunc;
	std::function<std::shared_ptr<BaseVersionList>(Entity)> m_versionListFactoryFunc;
};
