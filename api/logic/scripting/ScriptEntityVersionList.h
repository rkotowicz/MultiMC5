#pragma once

#include "BaseVersionList.h"

#include <sol.hpp>

#include "EntityProvider.h"
#include "ScriptEntityVersion.h"

class ScriptEntityVersionList : public BaseVersionList
{
public:
	explicit ScriptEntityVersionList(const sol::table &table, const EntityProvider::Entity &entity, EntityProvider *provider);

	Task *getLoadTask() override;
	bool isLoaded() override { return m_loaded; }
	const BaseVersionPtr at(int i) const override;
	int count() const override;
	BaseVersionPtr getLatestStable() const override;
	BaseVersionPtr getRecommended() const override;
	void sortVersions() override;

	bool compareLessThan(const ScriptEntityVersion *a, const ScriptEntityVersion *b);

	sol::table table() const { return m_table; }
	EntityProvider::Entity entity() const { return m_entity; }

	BaseVersionList::RoleList providesRoles() const override;
	QVariant data(const QModelIndex &index, int role) const override;

private:
	sol::table m_table;
	EntityProvider::Entity m_entity;
	EntityProvider *m_provider;
	QVector<ScriptVersionPtr> m_list;
	QList<int> m_roles;
	bool m_loaded = false;

protected slots:
	void updateListData(QList<BaseVersionPtr> versions) override;
};
