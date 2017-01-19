#include "ScriptEntityVersion.h"

#include "ScriptEntityVersionList.h"
#include "LuaUtil.h"

ScriptEntityVersion::ScriptEntityVersion(const sol::table &table, ScriptEntityVersionList *versionList, EntityProvider *provider)
	: m_table(table), m_versionList(versionList), m_provider(provider) {}

QString ScriptEntityVersion::descriptor() const
{
	return LuaUtil::requiredString(m_table, "descriptor");
}

QString ScriptEntityVersion::name() const
{
	return LuaUtil::requiredString(m_table, "name");
}

QString ScriptEntityVersion::typeString() const
{
	return LuaUtil::requiredString(m_table, "type");
}

bool ScriptEntityVersion::operator <(BaseVersion &other)
{
	return m_versionList->compareLessThan(this, dynamic_cast<ScriptEntityVersion *>(&other));
}
bool ScriptEntityVersion::operator >(BaseVersion &other)
{
	return m_versionList->compareLessThan(dynamic_cast<ScriptEntityVersion *>(&other), this);
}
