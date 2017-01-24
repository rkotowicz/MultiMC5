#include "ScriptEntityVersionList.h"

#include "Script.h"
#include "ScriptEntityVersion.h"
#include "LuaUtil.h"

static QHash<int, std::string> rolesMapping()
{
	static QHash<int, std::string> mapping;
	if (mapping.isEmpty())
	{
		mapping.insert(BaseVersionList::ParentGameVersionRole, "parent_game_version");
		mapping.insert(BaseVersionList::RecommendedRole, "recommended");
		mapping.insert(BaseVersionList::LatestRole, "latest");
		mapping.insert(BaseVersionList::BranchRole, "branch");
		mapping.insert(BaseVersionList::ArchitectureRole, "arch");
	}
	return mapping;
}

ScriptEntityVersionList::ScriptEntityVersionList(const sol::table &table, const EntityProvider::Entity &entity, EntityProvider *provider)
	: BaseVersionList(), m_table(table), m_entity(entity), m_provider(provider)
{
	const sol::optional<sol::table> rolesTable = table["roles"];
	if (rolesTable)
	{
		const auto mapping = rolesMapping();
		const sol::table table = rolesTable.value();
		for (auto it = mapping.cbegin(); it != mapping.cend(); ++it)
		{
			const sol::optional<sol::object> field = table[it.value()];
			if (field)
			{
				m_roles.append(it.key());
			}
		}
	}
}

Task *ScriptEntityVersionList::getLoadTask()
{
	return new ScriptTask([this](ScriptTask *task) -> void
	{
		auto internalFunc = LuaUtil::required<sol::protected_function>(m_table, "load");
		sol::protected_function_result rawResult = internalFunc(task->taskContext());
		if (!rawResult.valid() || !rawResult.get<sol::optional<sol::table>>())
		{
			const sol::error err = rawResult;
			throw Exception(QString("Unable to load versions for %1-%2: %3").arg(m_provider->id(), m_entity.internalId, err.what()));
		}

		QList<BaseVersionPtr> result;
		for (const auto &pair : rawResult.get<sol::table>()) {
			const sol::table table = pair.second;
			result.append(std::make_shared<ScriptEntityVersion>(table, this, m_provider));
		}
		updateListData(result);
	}, m_provider->script(), this);
}

const BaseVersionPtr ScriptEntityVersionList::at(int i) const
{
	return m_list.at(i);
}
int ScriptEntityVersionList::count() const
{
	return m_list.size();
}

void ScriptEntityVersionList::sortVersions()
{
	std::sort(m_list.begin(), m_list.end(), [this](const ScriptVersionPtr &a, const ScriptVersionPtr &b) { return compareLessThan(a.get(), b.get()); });
}

bool ScriptEntityVersionList::compareLessThan(const ScriptEntityVersion *a, const ScriptEntityVersion *b)
{
	auto comparator = m_table.get<std::function<bool(sol::table, sol::table, sol::table)>>("comparator");
	if (comparator)
	{
		return comparator(m_provider->script()->staticContext(), b->table(), a->table());
	}
	else
	{
		return b->name() < a->name();
	}
}

void ScriptEntityVersionList::updateListData(QList<BaseVersionPtr> versions)
{
	beginResetModel();
	m_list.clear();
	for (const auto &ver : versions) {
		m_list.append(std::dynamic_pointer_cast<ScriptEntityVersion>(ver));
	}
	m_loaded = true;
	sortVersions();
	endResetModel();
}

QVariant ScriptEntityVersionList::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (index.row() > count())
		return QVariant();

	const ScriptVersionPtr version = m_list.at(index.row());
	switch (role)
	{
	case VersionPointerRole: return qVariantFromValue(std::static_pointer_cast<BaseVersion>(version));
	case VersionRole: return version->name();
	case VersionIdRole: return version->descriptor();
	case TypeRole: return version->typeString();

	case ParentGameVersionRole:
	case LatestRole:
	case RecommendedRole:
	case BranchRole:
	case ArchitectureRole:
	{
		const sol::object obj = m_table["roles"][rolesMapping()[role]];
		if (obj.is<std::string>())
		{
			return LuaUtil::toVariant(version->table()[obj.as<std::string>()]);
		}
		else if (obj.is<sol::function>())
		{
			const sol::protected_function func = obj.as<sol::protected_function>();
			const sol::protected_function_result res = func(m_provider->script()->staticContext(), version->table());
			if (!res)
			{
				sol::error err = res;
				qCritical() << "Unable to run Lua function for" << m_entity.internalId << version->name() << ":" << err.what();
				return QVariant();
			}
			else
			{
				return LuaUtil::toVariant(res.get<sol::object>());
			}
		}
	}

	default:
		return QVariant();
	}
}

BaseVersionList::RoleList ScriptEntityVersionList::providesRoles() const
{
	return m_roles + QList<int>({VersionPointerRole, VersionRole, VersionIdRole, TypeRole});
}
