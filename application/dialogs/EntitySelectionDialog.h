#pragma once

#include <QDialog>

#include "scripting/EntityProvider.h"

namespace Ui {
class EntitySelectionDialog;
}

class ProxyModelChain;

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
	ProxyModelChain *m_proxies;
};
