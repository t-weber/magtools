/**
 * space group browser
 * @author Tobias Weber
 * @date Apr-2018
 * @license see 'LICENSE.GPL' file
 */

#ifndef __SGBROWSE_H__
#define __SGBROWSE_H__


#include <QtCore/QSettings>
#include <QtGui/QGenericMatrix>
#include <QtWidgets/QDialog>
#include <QtWidgets/QTreeWidgetItem>
#include <QtWidgets/QListWidgetItem>

#include "libs/magsg.h"
#include "libs/math_conts.h"

#include "ui_browser.h"


using t_real_sg = double;
using t_vec_sg = m::qvec_adapter<int, 3, t_real_sg, QGenericMatrix>;
using t_mat_sg = m::qmat_adapter<int, 3, 3, t_real_sg, QGenericMatrix>;


class SgBrowserDlg : public QDialog, Ui::SgBrowserDlg
{
private:
	QSettings *m_pSettings = nullptr;
	Spacegroups<t_mat_sg, t_vec_sg> m_sgs;

	void SetupSpaceGroup(const Spacegroup<t_mat_sg, t_vec_sg>& sg);
	void SetupSpaceGroups();

protected:
	virtual void showEvent(QShowEvent *pEvt) override;
	virtual void hideEvent(QHideEvent *pEvt) override;
	virtual void closeEvent(QCloseEvent *pEvt) override;

	void SpaceGroupSelected(QTreeWidgetItem *pItem);

public:
	using QDialog::QDialog;
	SgBrowserDlg(QWidget* pParent = nullptr, QSettings* pSett = nullptr);
	~SgBrowserDlg() = default;
};


#endif
