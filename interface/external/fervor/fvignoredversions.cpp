#include "fvignoredversions.h"
#include "fvversioncomparator.h"
#include <QSettings>
#include <QCoreApplication>
#include <string>

// QSettings key for the latest skipped version
#define FV_IGNORED_VERSIONS_LATEST_SKIPPED_VERSION_KEY	"FVLatestSkippedVersion"


FVIgnoredVersions::FVIgnoredVersions(QObject *parent) :
QObject(parent)
{
	// noop
}

bool FVIgnoredVersions::VersionIsIgnored(QString version)
{
	// We assume that variable 'version' contains either:
	//	1) The current version of the application (ignore)
	//	2) The version that was skipped before and thus stored in QSettings (ignore)
	//	3) A newer version (don't ignore)
	// 'version' is not likely to contain an older version in any case.
    
	if (version == QCoreApplication::applicationVersion()) {
		return true;
	}
    
    QSettings settings(QSettings::NativeFormat,
					   QSettings::UserScope,
					   QCoreApplication::organizationDomain(),
					   QCoreApplication::applicationName());
    
	//QSettings settings;
	if (settings.contains(FV_IGNORED_VERSIONS_LATEST_SKIPPED_VERSION_KEY)) {
		QString lastSkippedVersion = settings.value(FV_IGNORED_VERSIONS_LATEST_SKIPPED_VERSION_KEY).toString();
		if (version == lastSkippedVersion) {
			// Implicitly skipped version - skip
			return true;
		}
	}
    
	std::string currentAppVersion = QCoreApplication::applicationVersion().toStdString();
	std::string suggestedVersion = std::string(version.toStdString());
	if (FvVersionComparator::CompareVersions(currentAppVersion, suggestedVersion) == FvVersionComparator::kAscending) {
		// Newer version - do not skip
		return false;
	}
    
	// Fallback - skip
	return true;
}

void FVIgnoredVersions::IgnoreVersion(QString version)
{
	if (version == QCoreApplication::applicationVersion()) {
		// Don't ignore the current version
		return;
	}
    
	if (version.isEmpty()) {
		return;
	}
    
    QSettings settings(QSettings::NativeFormat,
					   QSettings::UserScope,
					   QCoreApplication::organizationDomain(),
					   QCoreApplication::applicationName());
    

	settings.setValue(FV_IGNORED_VERSIONS_LATEST_SKIPPED_VERSION_KEY, version);
    
	return;
}