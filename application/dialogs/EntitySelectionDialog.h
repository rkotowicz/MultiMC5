#pragma once

#include <QDialog>

#include "scripting/EntityProvider.h"

namespace Ui {
class EntitySelectionDialog;
}

class IconUrlProxyModel;

class EntitySelectionDialog : public QDialog
{
	Q_OBJECT

public:
	explicit EntitySelectionDialog(QWidget *parent = 0);
	~EntitySelectionDialog();

	EntityProvider::Entity entity() const;

protected:
	void changeEvent(QEvent *e);

private:
	Ui::EntitySelectionDialog *ui;
	IconUrlProxyModel *m_proxy;
};
