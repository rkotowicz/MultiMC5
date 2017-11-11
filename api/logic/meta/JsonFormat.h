/* Copyright 2015-2017 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <QJsonObject>
#include <memory>

#include "Exception.h"
#include "meta/BaseEntity.h"

namespace Meta
{
class Index;
class Version;
class VersionList;

class ParseException : public Exception
{
public:
	using Exception::Exception;
};
struct Require
{
	Require(const QString& uid_, const QString & equalsVersion_ = QString())
	{
		uid = uid_;
		equalsVersion = equalsVersion_;
	}
	bool operator==(const Require & rhs) const
	{
		return uid == rhs.uid;
	}
	QString uid;
	QString equalsVersion;
};

inline Q_DECL_PURE_FUNCTION uint qHash(const Require &key, uint seed = 0) Q_DECL_NOTHROW
{
	return qHash(key.uid, seed);
}

using RequireSet = QSet<Require>;

void parseIndex(const QJsonObject &obj, Index *ptr);
void parseVersion(const QJsonObject &obj, Version *ptr);
void parseVersionList(const QJsonObject &obj, VersionList *ptr);

// FIXME: this has a different shape than the others...FIX IT!?
void parseRequires(const QJsonObject &obj, RequireSet * ptr);
void serializeRequires(QJsonObject & objOut, RequireSet* ptr);
}

Q_DECLARE_METATYPE(QSet<Meta::Require>);