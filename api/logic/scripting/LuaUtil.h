#pragma once

#include <sol.hpp>

#include "Exception.h"
#include "multimc_logic_export.h"

class LuaException : public Exception
{
public:
	using Exception::Exception;
};

namespace LuaUtil
{

// TODO: this is very similar to the Json API, make it have the same naming scheme to avoid confusion

template <typename T>
static T required(const sol::object &object)
{
	if (!object.is<T>())
	{
		throw LuaException("Wrong type");
	}
	return object.as<T>();
}
template <>
QString required<QString>(const sol::object &object)
{
	return QString::fromStdString(required<std::string>(object));
}
template <typename T, typename Table>
static T required(const Table &table, const std::string &key)
{
	auto opt = table[key];
	if (!opt)
	{
		throw LuaException(QString("Missing key '%1' in table").arg(QString::fromStdString(key)));
	}
	return required<T>(opt);
}
template <typename Table>
static QString requiredString(const Table &table, const std::string &key)
{
	return required<QString, Table>(table, key);
}

template <typename T, typename Table>
static T optional(const Table &table, const std::string &key)
{
	sol::optional<T> opt = table[key];
	if (!opt)
	{
		return T{};
	}
	else
	{
		return opt.value();
	}
}
template <typename Table>
static QString optionalString(const Table &table, const std::string &key)
{
	return QString::fromStdString(optional<std::string, Table>(table, key));
}

/// Only checks the type of the first key
MULTIMC_LOGIC_EXPORT bool isMap(const sol::table &table);

QVariant toVariant(const sol::object &object);
QVariantHash toVariantHash(const sol::table &table);
QVariantList toVariantList(const sol::table &table);

sol::table fromJson(sol::state_view &state, const QJsonObject &obj);
sol::table fromJson(sol::state_view &state, const QJsonArray &arr);
sol::table fromJson(sol::state_view &state, const std::string &json);
std::string toJson(const sol::table &table);
QJsonObject toJsonObject(const sol::table &table);

}
