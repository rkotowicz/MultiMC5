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

#include "PackWindow.h"
#include "MultiMC.h"

#include <QScrollBar>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <qlayoutitem.h>
#include <QCloseEvent>

#include "widgets/PageContainer.h"
#include "InstancePageProvider.h"

Technic::PackWindow::PackWindow(QWidget *parent)
	: QMainWindow(parent)
{
	setAttribute(Qt::WA_DeleteOnClose);

	// Set window properties
	{
		setWindowIcon(MMC->getThemedIcon("technic"));
		setWindowTitle(tr("Technic pack catalog "));
	}

	// Add page container
	{
		m_widget = new PackWidget(this);
		setCentralWidget(m_widget);
	}

	// restore window state
	{
		auto base64State = MMC->settings()->get("TechnicWindowState").toByteArray();
		restoreState(QByteArray::fromBase64(base64State));
		auto base64Geometry = MMC->settings()->get("TechnicWindowGeometry").toByteArray();
		restoreGeometry(QByteArray::fromBase64(base64Geometry));
	}

	show();
}

void Technic::PackWindow::on_closeButton_clicked()
{
	close();
}

void Technic::PackWindow::closeEvent(QCloseEvent *event)
{
	MMC->settings()->set("TechnicWindowState", saveState().toBase64());
	MMC->settings()->set("TechnicWindowGeometry", saveGeometry().toBase64());
	emit isClosing();
	event->accept();
}

Technic::PackWindow::~PackWindow()
{
}
