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
#include "IconUrlProxyModel.h"


ScriptPage::ScriptPage(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::ScriptPage),
	m_proxy(new IconUrlProxyModel(this))
{
	ui->setupUi(this);

	ui->scriptsView->setModel(ENV.scripts().get());

	m_proxy->setSourceModel(ENV.scripts()->entityProviderManager()->infoModel());
	ui->providersView->setModel(m_proxy);

	connect(ui->scriptsView->selectionModel(), &QItemSelectionModel::currentRowChanged, this, [this](const QModelIndex &newRow)
	{
		ui->reloadBtn->setEnabled(newRow.isValid());
	});
	connect(ui->providersView->selectionModel(), &QItemSelectionModel::currentRowChanged, this, [this](const QModelIndex &newRow)
	{
		ui->updateProviderBtn->setEnabled(newRow.isValid());
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
	const QModelIndex index = m_proxy->mapToSource(ui->providersView->currentIndex());
	EntityProvider *provider = ENV.scripts()->entityProviderManager()->providerForIndex(index);
	if (provider)
	{
		ProgressDialog(this).execWithTask(provider->createUpdateEntitiesTask());
	}

	EntityProvider::Entity entity = ENV.scripts()->entityProviderManager()->entityForIndex(index);
	if (!entity.internalId.isNull())
	{
		ProgressDialog(this).execWithTask(entity.provider->versionList(entity)->getLoadTask());
	}
}
