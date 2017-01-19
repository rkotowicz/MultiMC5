#include "EntitySelectionDialog.h"
#include "ui_EntitySelectionDialog.h"

#include "scripting/EntityProviderManager.h"
#include "scripting/ScriptManager.h"
#include "Env.h"

EntitySelectionDialog::EntitySelectionDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::EntitySelectionDialog)
{
	ui->setupUi(this);

	ui->buttonBox->addButton(tr("Install"), QDialogButtonBox::AcceptRole);
	ui->treeView->setModel(ENV.scripts()->entityProviderManager()->entitiesModel());

	connect(ui->treeView, &QTreeView::doubleClicked, [this](const QModelIndex &index)
	{
		if (index.isValid())
		{
			accept();
		}
	});
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
