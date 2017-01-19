#pragma once

#include <QWidget>

#include "pages/BasePage.h"

namespace Ui {
class ScriptPage;
}

class ScriptPage : public QWidget, public BasePage
{
	Q_OBJECT
public:
	explicit ScriptPage(QWidget *parent = 0);
	~ScriptPage();

	QString id() const override { return "script-global"; }
	QString displayName() const override { return tr("Scripts"); }
	QIcon icon() const override;

private slots:
	void on_updateAllBtn_clicked();
	void on_reloadAllBtn_clicked();
	void on_reloadBtn_clicked();
	void on_updateAllProvidersBtn_clicked();
	void on_updateProviderBtn_clicked();

protected:
	void changeEvent(QEvent *e);

private:
	Ui::ScriptPage *ui;
};
