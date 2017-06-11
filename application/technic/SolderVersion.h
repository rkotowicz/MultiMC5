#pragma once
#include <QString>
#include <QJsonObject>
#include <memory>
#include "BaseVersion.h"
#include "Version.h"

class SolderVersion : public BaseVersion
{
public:
	typedef std::shared_ptr<SolderVersion> Ptr;

public:
	SolderVersion(QJsonObject version);
	SolderVersion(QString versionId);

public:
	virtual QString descriptor() override;
	virtual QString name() override;
	virtual QString typeString() const override;
	virtual bool operator<(BaseVersion &a) override;
	virtual bool operator>(BaseVersion &a) override;

	QJsonObject toJson();

	QString filename();
	QString url();

public:
	QString base_url;
	QString pack_name;
	QString id;

	bool is_recommended = false;
	bool is_latest = false;
	bool is_complete = false;
private:
	Version version;
};

Q_DECLARE_METATYPE(SolderVersion::Ptr)
