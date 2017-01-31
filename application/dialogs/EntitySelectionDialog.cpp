#include "EntitySelectionDialog.h"
#include "ui_EntitySelectionDialog.h"

#include <QPushButton>
#include <QAbstractItemView>
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QDebug>
#include <QSortFilterProxyModel>
#include <QTimer>

#include "dialogs/ProgressDialog.h"
#include "scripting/EntityProviderManager.h"
#include "scripting/ScriptManager.h"
#include "IconUrlProxyModel.h"
#include "tasks/Task.h"
#include "Env.h"
#include "ProxyModelChain.h"

extern void qt_blurImage(QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0);

class FancyView : public QWidget
{
	Q_OBJECT
public:
	explicit FancyView(QAbstractItemModel *model, QItemSelectionModel *selectionModel, QWidget *parent = nullptr)
		: QWidget(parent), m_model(model), m_selectionModel(selectionModel)
	{
		connect(selectionModel, &QItemSelectionModel::currentRowChanged, this, &FancyView::update);
		connect(model, &QAbstractItemModel::modelReset, this, &FancyView::update);
		connect(model, &QAbstractItemModel::layoutChanged, this, &FancyView::update);
		connect(model, &QAbstractItemModel::rowsInserted, this, [this](QModelIndex, const int from, const int to)
		{
			if (isAffected(from, to))
			{
				update();
			}
		});
		connect(model, &QAbstractItemModel::rowsRemoved, this, [this](QModelIndex, const int from, const int to)
		{
			if (isAffected(from, to))
			{
				update();
			}
		});
		connect(model, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex &tl, const QModelIndex &br, const QVector<int> &roles)
		{
			if (isAffected(tl.row(), br.row()) && (roles.contains(Qt::DisplayRole) || roles.contains(Qt::DecorationRole)))
			{
				update();
			}
		});
	}

	bool isAffected(const int from, const int to)
	{
		return (from > m_visibleRangeLeft || m_visibleRangeLeft != -1) || (to < m_visibleRangeRight || m_visibleRangeRight != -1);
	}

	QSize sizeHint() const override
	{
		return QSize(640, 240);
	}
	void paintEvent(QPaintEvent *event) override
	{
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.setPen(Qt::NoPen);

		const QModelIndex current = m_selectionModel->currentIndex();
		const int currentRow = current.isValid() ? current.row() : 0;

		m_visibleItems.clear();

		if (m_model->rowCount() == 0)
		{
			painter.setBrush(palette().background());
			painter.drawRect(event->rect());
		}
		else
		{
			const int tenthWidth = width()/10;

			m_visibleRangeLeft = std::max(0, currentRow - m_range);
			for (int i = m_visibleRangeLeft, j = 0; i < currentRow; ++i, ++j)
			{
				qDebug() << "drawing left" << i;
				draw(&painter, i,
					 QPoint(tenthWidth + tenthWidth * j, height() - 10 * (4 - j) + height()/20),
					 QSize(tenthWidth * 2, height() * 0.9 * (1 - j*0.1)),
					 -0.1);
			}
			m_visibleRangeRight = std::min(m_model->rowCount() - 1, currentRow + m_range);
			for (int i = m_visibleRangeRight, j = 0; i > currentRow; --i, ++j)
			{
				qDebug() << "drawing right" << i;
				draw(&painter, i,
					 QPoint(width() - tenthWidth - tenthWidth * j, height() - 10 * (4 - j) - height()/4.5),
					 QSize(tenthWidth * 2, height() * 0.9 * (1 - j*0.1)),
					 0.1);
			}

			draw(&painter, currentRow, QPoint(width()/2, height() - 10), QSize(width()/3, height() - 20), 0);
		}
	}
	void mouseReleaseEvent(QMouseEvent *event) override
	{
		for (const auto &pair : m_visibleItems)
		{
			if (pair.first.contains(event->pos()) && pair.second.isValid())
			{
				m_selectionModel->setCurrentIndex(pair.second, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current);
				break;
			}
		}
	}
	void mouseDoubleClickEvent(QMouseEvent *event) override
	{
		for (const auto &pair : m_visibleItems)
		{
			if (pair.first.contains(event->pos()) && pair.second.isValid())
			{
				m_selectionModel->setCurrentIndex(pair.second, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current);
				break;
			}
		}
		emit doubleClicked();
	}

	void draw(QPainter *painter, const int row, const QPoint &bottomCenter, const QSize &maxSize, const qreal angle)
	{
		painter->save();
		painter->shear(0, angle);

		QRect maxRect = QRect(bottomCenter.x() - maxSize.width()/2, bottomCenter.y() - maxSize.height(), maxSize.width(), maxSize.height());

		const QVariant data = m_model->index(row, 0).data(IconUrlProxyModel::SourceSizeDecorationRole);
		if (data.canConvert<QPixmap>())
		{
			const QPixmap pixmap = data.value<QPixmap>();
			const QPixmap scaled = pixmap.scaled(maxRect.size(), Qt::KeepAspectRatio);
			maxRect.setSize(scaled.size());
			maxRect.setY(bottomCenter.y() - scaled.height());
			painter->drawPixmap(maxRect, scaled);
			//drawShadowBehind(painter, maxRect, scaled);
		}
		else if (data.canConvert<QIcon>())
		{
			const QIcon icon = data.value<QIcon>();
			const QPixmap pixmap = icon.pixmap(maxRect.size());
			painter->drawPixmap(maxRect, pixmap);
			//drawShadowBehind(painter, maxRect, pixmap);
		}
		else
		{
			painter->setBrush(QBrush(QColor(255, 0, 0, 128)));
			painter->drawRect(maxRect);
			painter->setPen(Qt::white);
			painter->drawText(maxRect, tr("No Image"), QTextOption(Qt::AlignCenter | Qt::AlignHCenter));
			//drawShadowBehind(painter, maxRect, QPixmap());
		}

		m_visibleItems.append(qMakePair(maxRect, m_model->index(row, 0)));

		painter->restore();
	}

	void update()
	{
		QWidget::update();
	}

	void drawShadowBehind(QPainter *painter, const QRect &rect, const QPixmap &pixmap)
	{
		painter->save();
		if (pixmap.isNull())
		{
			QLinearGradient grad;
			grad.setColorAt(0, Qt::transparent);
			grad.setColorAt(0.5, Qt::black);
			grad.setColorAt(1, Qt::transparent);
			painter->setPen(QPen(QBrush(grad), 20));
			painter->setBrush(Qt::NoBrush);
			painter->drawRect(rect);
		}
		else
		{
			// from QPixmapDropShadowFilter::draw

			QImage tmp(pixmap.size(), QImage::Format_ARGB32_Premultiplied);
			tmp.fill(0);
			QPainter tmpPainter(&tmp);
			tmpPainter.setCompositionMode(QPainter::CompositionMode_Source);
			tmpPainter.drawPixmap(0, 0, pixmap); //< adjust position here for offset
			tmpPainter.end();

			// blur the alpha channel
			QImage blurred(tmp.size() + QSize(40, 40), QImage::Format_ARGB32_Premultiplied);
			blurred.fill(0);
			QPainter blurPainter(&blurred);
			blurPainter.translate(20, 20);
			qt_blurImage(&blurPainter, tmp, 20, false, true); //< adjust 3rd parameter here for radius
			blurPainter.end();

			tmp = blurred;

			// blacken the image...
			tmpPainter.begin(&tmp);
			tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
			tmpPainter.fillRect(tmp.rect(), QColor(Qt::black)); //< adjust 2nd parameter for shadow color
			tmpPainter.end();

			// draw the blurred drop shadow...
			painter->drawImage(rect.topLeft() - QPoint(20, 20), tmp);

			// Draw the actual pixmap...
			//painter->drawPixmap(rect, pixmap);
			painter->setPen(QPen(Qt::white, 1));
			painter->setBrush(Qt::NoBrush);
			painter->drawRect(rect);
		}
		painter->restore();
	}

signals:
	void doubleClicked();

private:
	QAbstractItemModel *m_model;
	QItemSelectionModel *m_selectionModel;
	int m_visibleRangeLeft = -1;
	int m_visibleRangeRight = -1;
	const int m_range = 3;
	QVector<QPair<QRect, QPersistentModelIndex>> m_visibleItems;
};

class TypeFilterProxyModel : public QSortFilterProxyModel
{
public:
	explicit TypeFilterProxyModel(QObject *parent)
		: QSortFilterProxyModel(parent)
	{
		setFilterRole(EntityProviderManager::EntityTypeRole);
		setFilterKeyColumn(0);
	}

	void setType(const int type)
	{
		m_type = type;
		invalidate();
	}

	bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override
	{
		return m_type == -1 || sourceModel()->index(source_row, 0).data(EntityProviderManager::EntityTypeRole).toInt() == m_type;
	}

private:
	int m_type = -1;
};

EntitySelectionDialog::EntitySelectionDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::EntitySelectionDialog)
{
	ui->setupUi(this);

	TypeFilterProxyModel *typeFilter = new TypeFilterProxyModel(this);
	QSortFilterProxyModel *searchFilter = new QSortFilterProxyModel(this);

	// setup proxy chain and tree view
	{
		m_proxies = new ProxyModelChain(this);
		m_proxies->setSourceModel(ENV.scripts()->entityProviderManager()->entitiesModel());
		m_proxies->append(typeFilter);
		m_proxies->append(searchFilter);
		m_proxies->append(new IconUrlProxyModel(this));

		QAbstractItemModel *m = m_proxies->get();
		while (m)
		{
			qDebug() << m;
			QAbstractProxyModel *p = qobject_cast<QAbstractProxyModel *>(m);
			if (p)
			{
				m = p->sourceModel();
			}
			else
			{
				break;
			}
		}

		ui->treeView->setModel(m_proxies->get());
		connect(ui->treeView, &QTreeView::doubleClicked, [this](const QModelIndex &index)
		{
			if (index.isValid())
			{
				accept();
			}
		});
	}

	// setup refresh button
	{
		ui->buttonBox->addButton(tr("Install"), QDialogButtonBox::AcceptRole);
		QPushButton *refreshBtn = ui->buttonBox->addButton(tr("Refresh"), QDialogButtonBox::ActionRole);
		connect(refreshBtn, &QPushButton::clicked, this, [this]()
		{
			ProgressDialog(this).execWithTask(ENV.scripts()->entityProviderManager()->createUpdateAllTask());
		});

		// simulate click to refresh
		QMetaObject::invokeMethod(refreshBtn, "click", Qt::QueuedConnection);
	}

	// setup type filter box
	{
		ui->typeBox->addItem("");
		ui->typeBox->addItem(tr("Mods"), EntityProvider::Entity::Mod);
		ui->typeBox->addItem(tr("Modpacks"), EntityProvider::Entity::Modpack);
		ui->typeBox->addItem(tr("Worlds"), EntityProvider::Entity::World);
		ui->typeBox->addItem(tr("Resource Packs"), EntityProvider::Entity::TexturePack);
		ui->typeBox->addItem(tr("Patches"), EntityProvider::Entity::Patch);
		ui->typeBox->addItem(tr("Other"), EntityProvider::Entity::Other);
		connect(ui->typeBox, &QComboBox::currentTextChanged, this, [this, typeFilter]()
		{
			typeFilter->setType(ui->typeBox->currentData().toInt());
		});
	}

	// setup search filter
	{
		searchFilter->setFilterRole(Qt::DisplayRole);
		searchFilter->setFilterKeyColumn(0);

		QTimer *timer = new QTimer(this);
		timer->setInterval(250);
		timer->setSingleShot(true);
		connect(ui->searchEdit, &QLineEdit::textChanged, timer, qOverload<>(&QTimer::start));
		connect(timer, &QTimer::timeout, this, [this, searchFilter]()
		{
			searchFilter->setFilterRegExp(QRegExp("*" + ui->searchEdit->text() + "*", Qt::CaseInsensitive, QRegExp::WildcardUnix));
		});
	}

	// setup fancy view
	{
		FancyView *fancyView = new FancyView(m_proxies->get(), ui->treeView->selectionModel(), this);
		ui->verticalLayout->insertWidget(0, fancyView);
		connect(fancyView, &FancyView::doubleClicked, [this]()
		{
			if (ui->treeView->currentIndex().isValid())
			{
				accept();
			}
		});
	}
}

EntitySelectionDialog::~EntitySelectionDialog()
{
	delete ui;
}

EntityProvider::Entity EntitySelectionDialog::entity() const
{
	if (ui->treeView->currentIndex().isValid())
	{
		return ENV.scripts()->entityProviderManager()->entityForIndex(m_proxies->mapToSource(ui->treeView->currentIndex()));
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

#include "EntitySelectionDialog.moc"
