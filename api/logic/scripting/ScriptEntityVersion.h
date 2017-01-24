#pragma once

#include "BaseVersion.h"

#include <sol.hpp>

class ScriptEntityVersionList;
class EntityProvider;
class BaseInstance;

class ScriptEntityVersion : public BaseVersion
{
public:
	explicit ScriptEntityVersion(const sol::table &table, ScriptEntityVersionList *versionList, EntityProvider *provider);

	QString descriptor() const;
	QString descriptor() override { return const_cast<const ScriptEntityVersion *>(this)->descriptor(); }
	QString name() const;
	QString name() override { return const_cast<const ScriptEntityVersion *>(this)->name(); }
	QString typeString() const override;
	bool operator <(BaseVersion &other) override;
	bool operator >(BaseVersion &other) override;

	sol::table table() const { return m_table; }
	ScriptEntityVersionList *versionList() const { return m_versionList; }

	QString patchFilename(const BaseInstance *instance) const;

private:
	sol::table m_table;
	ScriptEntityVersionList *m_versionList;
	EntityProvider *m_provider;
};
using ScriptVersionPtr = std::shared_ptr<ScriptEntityVersion>;
