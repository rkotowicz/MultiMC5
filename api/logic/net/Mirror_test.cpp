/* Copyright 2013-2016 MultiMC Contributors
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

#include <QTest>

#include "TestUtil.h"
#include <net/Mirror.h>

static const QByteArray hardcodedMirrorJson ="\
{\
	\"version\":1,\
	\"name\":\"BMCLAPI\",\
	\"mirrors\":\
	[\
		{\
			\"name\":\"assetsURL\",\
			\"officialPrefix\":\"http://resources.download.minecraft.net/\",\
			\"mirrorPrefix\":\"http://bmclapi2.bangbang93.com/objects/\"\
		},\
		{\
			\"name\":\"librariesURL\",\
			\"officialPrefix\":\"https://libraries.minecraft.net/\",\
			\"mirrorPrefix\":\"http://bmclapi2.bangbang93.com/libraries/\"\
		},\
		{\
			\"name\":\"versionsURL\",\
			\"officialPrefix\":\"https://launcher.mojang.com/mc/game/\",\
			\"mirrorPrefix\":\"http://bmclapi2.bangbang93.com/mc/game/\"\
		}\
	]\
}\
";

class MirrorTest : public QObject
{
	Q_OBJECT
private slots:
	void test_mirror_passthrough_data()
	{
		QTest::addColumn<QString>("url");
		QTest::addColumn<QStringList>("result");

		QTest::newRow("passthrough domain url") << "http://google.com" << QStringList("http://google.com");
		QTest::newRow("passthrough full url") << "http://foo.bar/baz.xyx" << QStringList("http://foo.bar/baz.xyx");
	}
	void test_mirror_passthrough()
	{
		QFETCH(QString, url);
		QFETCH(QStringList, result);

		Mirror m;
		QCOMPARE(m.lookup(url), result);
	}

	void test_mirror_data()
	{
		QTest::addColumn<QString>("url");
		QTest::addColumn<QStringList>("expected");

		QTest::newRow("unrelated url")
			<< "http://google.com"
			<< QStringList("http://google.com");
		QTest::newRow("contained url 1")
			<< "http://resources.download.minecraft.net/"
			<< QStringList({"http://bmclapi2.bangbang93.com/objects/", "http://resources.download.minecraft.net/"});
		QTest::newRow("contained url 2")
			<< "https://libraries.minecraft.net/"
			<< QStringList({"http://bmclapi2.bangbang93.com/libraries/", "https://libraries.minecraft.net/"});
		QTest::newRow("contained url 3")
			<< "https://launcher.mojang.com/mc/game/"
			<< QStringList({"http://bmclapi2.bangbang93.com/mc/game/", "https://launcher.mojang.com/mc/game/"});
	}
	void test_mirror()
	{
		QFETCH(QString, url);
		QFETCH(QStringList, expected);

		Mirror m(hardcodedMirrorJson);
		auto result = m.lookup(url);
		qDebug() << result.join(' ');
		QCOMPARE(expected, result);
	}
};

QTEST_GUILESS_MAIN(MirrorTest)

#include "Mirror_test.moc"
