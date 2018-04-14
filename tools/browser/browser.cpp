/**
 * space group browser
 * @author Tobias Weber
 * @date Apr-2018
 * @license see 'LICENSE.GPL' file
 */

#include "browser.h"

#include <QtWidgets/QTreeWidgetItem>


// ----------------------------------------------------------------------------


SgBrowserDlg::SgBrowserDlg(QWidget* pParent, QSettings *pSett)
	: QDialog{pParent}, m_pSettings{pSett}
{
	this->setupUi(this);

	if(m_pSettings)
	{
		if(m_pSettings->contains("sgbrowser/geo"))
			this->restoreGeometry(m_pSettings->value("sgbrowser/geo").toByteArray());
		//if(m_pSettings->contains("sgbrowser/state"))
		//	this->restoreState(m_pSettings->value("sgbrowser/state").toByteArray());
	}

	SetupSpaceGroups();
}


// ----------------------------------------------------------------------------
/**
 * load space group list
 */
void SgBrowserDlg::SetupSpaceGroups()
{
	m_sgs.Load("../magsg.xml");

	const auto *pSgs = m_sgs.GetSpacegroups();
	if(pSgs)
	{
		for(const auto& sg : *pSgs)
			SetupSpaceGroup(sg);
	}
}


/**
 * add space group to tree widget
 */
void SgBrowserDlg::SetupSpaceGroup(const Spacegroup<t_mat_sg, t_vec_sg>& sg)
{
	int iNrStruct = sg.GetStructNumber();
	int iNrMag = sg.GetMagNumber();

	//std::cout << iNrStruct << std::endl;

	// find top-level item with given structural sg number
	auto get_topsg = [](QTreeWidget *pTree, int iStructNr) -> QTreeWidgetItem*
	{
		for(int item=0; item<pTree->topLevelItemCount(); ++item)
		{
			QTreeWidgetItem *pItem = pTree->topLevelItem(item);
			if(!pItem) continue;

			if(pItem->data(0, Qt::UserRole) == iStructNr)
				return pItem;
		}

		return nullptr;
	};


	// find existing top-level structural space group or insert new one
	auto *pTopItem = get_topsg(m_pTree, iNrStruct);
	if(!pTopItem)
	{
		QString structname = ("(" + std::to_string(iNrStruct) + ") " + sg.GetName()).c_str();

		pTopItem = new QTreeWidgetItem();
		pTopItem->setText(0, structname);
		pTopItem->setData(0, Qt::UserRole, iNrStruct);
		m_pTree->addTopLevelItem(pTopItem);
	}


	// create magnetic group and add it as sub-item for the corresponding structural group
	QString magname = ("(" + sg.GetNumber() + ") " + sg.GetName()).c_str();

	auto *pSubItem = new QTreeWidgetItem();
	pSubItem->setText(0, magname);
	pTopItem->setData(0, Qt::UserRole, iNrStruct);
	pTopItem->setData(0, Qt::UserRole+1, iNrMag);
	pTopItem->addChild(pSubItem);
}
// ----------------------------------------------------------------------------


void SgBrowserDlg::showEvent(QShowEvent *pEvt)
{
	QDialog::showEvent(pEvt);
}


void SgBrowserDlg::hideEvent(QHideEvent *pEvt)
{
	QDialog::hideEvent(pEvt);
}


void SgBrowserDlg::closeEvent(QCloseEvent *pEvt)
{
	if(m_pSettings)
	{
		m_pSettings->setValue("sgbrowser/geo", this->saveGeometry());
		//m_pSettings->setValue("sgbrowser/state", this->saveState());
	}

	QDialog::closeEvent(pEvt);
}


// ----------------------------------------------------------------------------
