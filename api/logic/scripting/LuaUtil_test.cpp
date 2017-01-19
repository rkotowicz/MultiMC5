#include <QTest>
#include "TestUtil.h"

#include "LuaUtil.h"

class LuaUtilTest : public QObject
{
	Q_OBJECT
private
slots:
	void test_isMap()
	{
		sol::state state;

		state.create_named_table("empty");
		state.create_named_table("map",
								 "a", "a",
								 "b", "b");
		state.create_named_table("array",
								 1, "a",
								 2, "b");

		QVERIFY(LuaUtil::isMap(state["map"]));
		QVERIFY(!LuaUtil::isMap(state["empty"]));
		QVERIFY(!LuaUtil::isMap(state["array"]));
	}
};

QTEST_GUILESS_MAIN(LuaUtilTest)

#include "LuaUtil_test.moc"
