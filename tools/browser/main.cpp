/**
 * space group browser
 * @author Tobias Weber
 * @date Apr-2018
 * @license see 'LICENSE.GPL' file
 */

#include "browser.h"

#include <QtCore/QSettings>
#include <QtGui/QApplication>

#include <locale>
#include <memory>


// ----------------------------------------------------------------------------


static inline void set_locales()
{
	std::ios_base::sync_with_stdio(false);

	::setlocale(LC_ALL, "C");
	std::locale::global(std::locale("C"));
	QLocale::setDefault(QLocale::C);
}



int main(int argc, char** argv)
{
	QSettings sett("tobis_stuff", "sgbrowser", nullptr);

	auto app = std::make_unique<QApplication>(argc, argv);
	set_locales();

	auto dlg = std::make_unique<SgBrowserDlg>(nullptr, &sett);
	dlg->show();

	return app->exec();
}

// ----------------------------------------------------------------------------
