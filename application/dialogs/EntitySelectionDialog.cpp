#include "EntitySelectionDialog.h"
#include "ui_EntitySelectionDialog.h"

#include <QPushButton>

#include "dialogs/ProgressDialog.h"
#include "scripting/EntityProviderManager.h"
#include "scripting/ScriptManager.h"
#include "IconUrlProxyModel.h"
#include "tasks/Task.h"
#include "Env.h"

EntitySelectionDialog::EntitySelectionDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::EntitySelectionDialog),
	m_proxy(new IconUrlProxyModel(this))
{
	ui->setupUi(this);

	m_proxy->setSourceModel(ENV.scripts()->entityProviderManager()->entitiesModel());
	ui->treeView->setModel(m_proxy);
	ui->buttonBox->addButton(tr("Install"), QDialogButtonBox::AcceptRole);
	QPushButton *refreshBtn = ui->buttonBox->addButton(tr("Refresh"), QDialogButtonBox::ActionRole);
	connect(refreshBtn, &QPushButton::clicked, this, [this]()
	{
		ProgressDialog(this).execWithTask(ENV.scripts()->entityProviderManager()->createUpdateAllTask());
	});

	connect(ui->treeView, &QTreeView::doubleClicked, [this](const QModelIndex &index)
	{
		if (index.isValid())
		{
			accept();
		}
	});

	// simulate click to refresh
	QMetaObject::invokeMethod(refreshBtn, "click", Qt::QueuedConnection);
}

EntitySelectionDialog::~EntitySelectionDialog()
{
	delete ui;
}

EntityProvider::Entity EntitySelectionDialog::entity() const
{
	if (ui->treeView->currentIndex().isValid())
	{
		return ENV.scripts()->entityProviderManager()->entityForIndex(ui->treeView->currentIndex());
	}
	return EntityProvider::Entity{};
}

void EntitySelectionDialog::changeEvent(QEvent *e)
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
