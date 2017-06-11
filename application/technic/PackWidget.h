/* Copyright 2013-2017 MultiMC Contributors
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

#include <QWidget>
#include "minecraft/Mod.h"
#include "QObjectPtr.h"

class PackModel;
namespace Ui
{
class TechnicPackWidget;
}

namespace Technic
{
class PackWidget : public QWidget
{
	Q_OBJECT

public:
	explicit PackWidget(QWidget *parent = 0);
	~PackWidget();

private:
	Ui::TechnicPackWidget *ui;
	shared_qobject_ptr<PackModel> m_mainModel;
};
}
