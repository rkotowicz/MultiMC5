#include "ScriptPage.h"
#include "ui_ScriptPage.h"

#include <QMessageBox>

#include "MultiMC.h"
#include "Env.h"
#include "BaseVersionList.h"
#include "scripting/ScriptManager.h"
#include "scripting/Script.h"
#include "scripting/EntityProviderManager.h"
#include "scripting/EntityProvider.h"
#include "dialogs/ProgressDialog.h"


ScriptPage::ScriptPage(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::ScriptPage)
{
	ui->setupUi(this);

	ui->scriptsView->setModel(ENV.scripts().get());
	ui->providersView->setModel(ENV.scripts()->entityProviderManager()->infoModel());

	connect(ui->scriptsView->selectionModel(), &QItemSelectionModel::currentRowChanged, this, [this](const QModelIndex &newRow)
	{
		if (ui->scriptsView->currentIndex().isValid())
		{
			ui->reloadBtn->setEnabled(true);
		}
		else
		{
			ui->reloadBtn->setEnabled(false);
		}
	});
}

ScriptPage::~ScriptPage()
{
	delete ui;
}

void ScriptPage::changeEvent(QEvent *e)
{
	QWidget::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}

QIcon ScriptPage::icon() const
{
	return MMC->getThemedIcon("coremods");
}

void ScriptPage::on_updateAllBtn_clicked()
{
	ProgressDialog(this).execWithTask(ENV.scripts()->createScriptUpdateTask());
}
void ScriptPage::on_reloadAllBtn_clicked()
{
	try
	{
		ENV.scripts()->loadScripts(QDir::current().absoluteFilePath(MMC->settings()->get("ScriptsDirectory").toString()));
	}
	catch (Exception &e)
	{
		QMessageBox::critical(this, tr("Unable to reload scripts"), tr("Unable to reload scripts: %1").arg(e.cause()));
	}
}
void ScriptPage::on_reloadBtn_clicked()
{
	if (!ui->scriptsView->currentIndex().isValid())
	{
		return;
	}
	Script *script = ENV.scripts()->script(ui->scriptsView->currentIndex());
	try
	{
		script->reload();
	}
	catch (Exception &e)
	{
		QMessageBox::critical(this, tr("Unable to reload script"), tr("Unable to reload script %1: %2").arg(script->id(), e.cause()));
	}
}

void ScriptPage::on_updateAllProvidersBtn_clicked()
{
	ProgressDialog(this).execWithTask(ENV.scripts()->entityProviderManager()->createUpdateAllTask());
}
void ScriptPage::on_updateProviderBtn_clicked()
{
	EntityProvider *provider = ENV.scripts()->entityProviderManager()->providerForIndex(ui->providersView->currentIndex());
	if (provider)
	{
		ProgressDialog(this).execWithTask(provider->createUpdateEntitiesTask());
	}

	EntityProvider::Entity entity = ENV.scripts()->entityProviderManager()->entityForIndex(ui->providersView->currentIndex());
	if (!entity.internalId.isNull())
	{
		ProgressDialog(this).execWithTask(entity.provider->versionList(entity)->getLoadTask());
	}
}
