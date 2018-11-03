/**
 * Calculation of polarisation vector
 * @author Tobias Weber
 * @date Oct-2018
 * @license: see 'LICENSE.GPL' file
 */

#include <QtCore/QSettings>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>

#include <locale>
#include <iostream>
#include <vector>
#include <string>

#include "../glplot/glplot_nothread.h"
#include "libs/math_algos.h"
#include "libs/math_conts.h"

using namespace m_ops;

using t_real = double;
using t_cplx = std::complex<t_real>;
using t_vec = std::vector<t_cplx>;
using t_mat = m::mat<t_cplx, std::vector>;
using t_matvec = std::vector<t_mat>;


// ----------------------------------------------------------------------------
class PolDlg : public QDialog
{
private:
	QSettings m_sett{"tobis_stuff", "pol"};

	std::shared_ptr<GlPlot> m_plot{std::make_shared<GlPlot>(this)};

	QLineEdit* m_editNRe = new QLineEdit("0", this);
	QLineEdit* m_editNIm = new QLineEdit("0", this);

	QLineEdit* m_editMPerpReX = new QLineEdit("0", this);
	QLineEdit* m_editMPerpReY = new QLineEdit("1", this);
	QLineEdit* m_editMPerpReZ = new QLineEdit("0", this);
	QLineEdit* m_editMPerpImX = new QLineEdit("0", this);
	QLineEdit* m_editMPerpImY = new QLineEdit("0", this);
	QLineEdit* m_editMPerpImZ = new QLineEdit("0", this);

	QLineEdit* m_editPiX = new QLineEdit("1", this);
	QLineEdit* m_editPiY = new QLineEdit("0", this);
	QLineEdit* m_editPiZ = new QLineEdit("0", this);
	QLineEdit* m_editPfX = new QLineEdit(this);
	QLineEdit* m_editPfY = new QLineEdit(this);
	QLineEdit* m_editPfZ = new QLineEdit(this);

	QLabel* m_labelStatus = new QLabel(this);


	// 3d object handles
	std::size_t m_arrow_pi = 0;
	std::size_t m_arrow_pf = 0;
	std::size_t m_arrow_M_Re = 0;
	std::size_t m_arrow_M_Im = 0;
	bool m_3dobjsReady = false;


protected:
	virtual void closeEvent(QCloseEvent *evt) override
	{
		// save window size and position
		m_sett.setValue("geo", saveGeometry());

		// save values
		m_sett.setValue("n_re", m_editNRe->text().toDouble());
		m_sett.setValue("n_im", m_editNIm->text().toDouble());
		m_sett.setValue("mx_re", m_editMPerpReX->text().toDouble());
		m_sett.setValue("my_re", m_editMPerpReY->text().toDouble());
		m_sett.setValue("mz_re", m_editMPerpReZ->text().toDouble());
		m_sett.setValue("mx_im", m_editMPerpImX->text().toDouble());
		m_sett.setValue("my_im", m_editMPerpImY->text().toDouble());
		m_sett.setValue("mz_im", m_editMPerpImZ->text().toDouble());
		m_sett.setValue("pix", m_editPiX->text().toDouble());
		m_sett.setValue("piy", m_editPiY->text().toDouble());
		m_sett.setValue("piz", m_editPiZ->text().toDouble());
	}


protected slots:
	/**
	 * called after the plotter has initialised
	 */
	void AfterGLInitialisation()
	{
		std::cerr << "GL device: " + m_plot->GetImpl()->GetGlDescr() << "." << std::endl;

		if(!m_3dobjsReady)		// create 3d objects
		{
			m_arrow_pi = m_plot->GetImpl()->AddArrow(0.05, 1., 0.,0.,0.5,  0.,0.,0.85,1.);
			m_arrow_pf = m_plot->GetImpl()->AddArrow(0.05, 1., 0.,0.,0.5,  0.,0.85,0.,1.);
			m_arrow_M_Re = m_plot->GetImpl()->AddArrow(0.05, 1., 0.,0.,0.5,  0.85,0.,0.,1.);
			m_arrow_M_Im = m_plot->GetImpl()->AddArrow(0.05, 1., 0.,0.,0.5,  0.85,0.85,0.,1.);

			m_plot->GetImpl()->SetObjectLabel(m_arrow_pi, "P_i");
			m_plot->GetImpl()->SetObjectLabel(m_arrow_pf, "P_f");
			m_plot->GetImpl()->SetObjectLabel(m_arrow_M_Re, "Re{M_perp}");
			m_plot->GetImpl()->SetObjectLabel(m_arrow_M_Im, "Im{M_perp}");

			m_3dobjsReady = true;
			CalcPol();
		}
	}


	/**
	 * called when the mouse hovers over an object
	 */
	void PickerIntersection(const t_vec3_gl* pos, std::size_t objIdx)
	{
		/*if(pos)
		{
			std::cout << "Intersecting object " << objIdx << " at position "
				<< (*pos)[0] << ", " << (*pos)[1] << ", " << (*pos)[2] << std::endl;
		}*/

		if(objIdx == m_arrow_pi) m_labelStatus->setText("P_i");
		else if(objIdx == m_arrow_pf) m_labelStatus->setText("P_f");
		else if(objIdx == m_arrow_M_Re) m_labelStatus->setText("Re{M_perp}");
		else if(objIdx == m_arrow_M_Im) m_labelStatus->setText("Im{M_perp}");
		else m_labelStatus->setText("");
	}


public:
	using QDialog::QDialog;

	/**
	 * create UI
	 */
	PolDlg(QWidget* pParent) : QDialog{pParent, Qt::Window}
	{
		setWindowTitle("Polarisation");
		setSizeGripEnabled(true);


		auto labelN = new QLabel("Re{N}, Im{N}:", this);
		auto labelMPerpRe = new QLabel("Re{M_perp}:", this);
		auto labelMPerpIm = new QLabel("Im{M_perp}:", this);
		auto labelPi = new QLabel("P_i:", this);
		auto labelPf = new QLabel("P_f:", this);


		for(auto* label : {labelMPerpRe, labelMPerpIm, labelPi, labelPf})
			label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

		for(auto* editPf : {m_editPfX, m_editPfY, m_editPfZ})
			editPf->setReadOnly(true);

		m_labelStatus->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		m_labelStatus->setFrameStyle(QFrame::Sunken | QFrame::Panel);
		m_labelStatus->setLineWidth(1);


		// connections
		for(auto* edit : {m_editNRe, m_editNIm,
			m_editMPerpReX, m_editMPerpReY, m_editMPerpReZ,
			m_editMPerpImX, m_editMPerpImY, m_editMPerpImZ,
			m_editPiX, m_editPiY, m_editPiZ,
			m_editPfX, m_editPfY, m_editPfZ})
			connect(edit, &QLineEdit::textEdited, this, &PolDlg::CalcPol);

		connect(m_plot.get(), &GlPlot::AfterGLInitialisation, this, &PolDlg::AfterGLInitialisation);
		connect(m_plot->GetImpl(), &GlPlot_impl::PickerIntersection, this, &PolDlg::PickerIntersection);


		auto pGrid = new QGridLayout(this);
		pGrid->setSpacing(4);
		pGrid->setContentsMargins(8,8,8,8);

		pGrid->addWidget(m_plot.get(), 0,0,1,4);

		pGrid->addWidget(labelN, 1,0,1,1);
		pGrid->addWidget(labelMPerpRe, 2,0,1,1);
		pGrid->addWidget(labelMPerpIm, 3,0,1,1);
		pGrid->addWidget(labelPi, 4,0,1,1);
		pGrid->addWidget(labelPf, 5,0,1,1);

		pGrid->addWidget(m_editNRe, 1,1,1,1);
		pGrid->addWidget(m_editNIm, 1,2,1,1);

		pGrid->addWidget(m_editMPerpReX, 2,1,1,1);
		pGrid->addWidget(m_editMPerpReY, 2,2,1,1);
		pGrid->addWidget(m_editMPerpReZ, 2,3,1,1);
		pGrid->addWidget(m_editMPerpImX, 3,1,1,1);
		pGrid->addWidget(m_editMPerpImY, 3,2,1,1);
		pGrid->addWidget(m_editMPerpImZ, 3,3,1,1);

		pGrid->addWidget(m_editPiX, 4,1,1,1);
		pGrid->addWidget(m_editPiY, 4,2,1,1);
		pGrid->addWidget(m_editPiZ, 4,3,1,1);
		pGrid->addWidget(m_editPfX, 5,1,1,1);
		pGrid->addWidget(m_editPfY, 5,2,1,1);
		pGrid->addWidget(m_editPfZ, 5,3,1,1);

		pGrid->addWidget(m_labelStatus, 6,0,1,4);


		// restory window size and position
		if(m_sett.contains("geo"))
			restoreGeometry(m_sett.value("geo").toByteArray());
		else
			resize(800, 600);

		// restore last values
		if(m_sett.contains("n_re")) m_editNRe->setText(m_sett.value("n_re").toString());
		if(m_sett.contains("n_im")) m_editNIm->setText(m_sett.value("n_im").toString());
		if(m_sett.contains("mx_re")) m_editMPerpReX->setText(m_sett.value("mx_re").toString());
		if(m_sett.contains("my_re")) m_editMPerpReY->setText(m_sett.value("my_re").toString());
		if(m_sett.contains("mz_re")) m_editMPerpReZ->setText(m_sett.value("mz_re").toString());
		if(m_sett.contains("mx_im")) m_editMPerpImX->setText(m_sett.value("mx_im").toString());
		if(m_sett.contains("my_im")) m_editMPerpImY->setText(m_sett.value("my_im").toString());
		if(m_sett.contains("mz_im")) m_editMPerpImZ->setText(m_sett.value("mz_im").toString());
		if(m_sett.contains("pix")) m_editPiX->setText(m_sett.value("pix").toString());
		if(m_sett.contains("piy")) m_editPiY->setText(m_sett.value("piy").toString());
		if(m_sett.contains("piz")) m_editPiZ->setText(m_sett.value("piz").toString());


		CalcPol();
	}


	/**
	 * calculate final polarisation vector
	 */
	void CalcPol()
	{
		// get values from line edits
		t_real NRe = t_real(m_editNRe->text().toDouble());
		t_real NIm = t_real(m_editNIm->text().toDouble());

		t_real MPerpReX = t_real(m_editMPerpReX->text().toDouble());
		t_real MPerpReY = t_real(m_editMPerpReY->text().toDouble());
		t_real MPerpReZ = t_real(m_editMPerpReZ->text().toDouble());
		t_real MPerpImX = t_real(m_editMPerpImX->text().toDouble());
		t_real MPerpImY = t_real(m_editMPerpImY->text().toDouble());
		t_real MPerpImZ = t_real(m_editMPerpImZ->text().toDouble());

		t_real PiX = t_real(m_editPiX->text().toDouble());
		t_real PiY = t_real(m_editPiY->text().toDouble());
		t_real PiZ = t_real(m_editPiZ->text().toDouble());

		const t_cplx N(NRe, NIm);
		const t_vec Mperp = m::create<t_vec>({
			t_cplx(MPerpReX,MPerpImX),
			t_cplx(MPerpReY,MPerpImY),
			t_cplx(MPerpReZ,MPerpImZ) });
		const t_vec Pi = m::create<t_vec>({PiX, PiY, PiZ});

		// calculate final polarisation vector and intensity
		auto [I, P_f] = m::blume_maleev_indir<t_mat, t_vec, t_cplx>(Pi, Mperp, N);

		// set final polarisation
		m_editPfX->setText(std::to_string(P_f[0].real()).c_str());
		m_editPfY->setText(std::to_string(P_f[1].real()).c_str());
		m_editPfZ->setText(std::to_string(P_f[2].real()).c_str());


		// update 3d objects
		if(m_3dobjsReady)
		{
			// P_i
			t_mat_gl matPi = GlPlot_impl::GetArrowMatrix(
				m::create<t_vec_gl>({t_real_gl(PiX), t_real_gl(PiY), t_real_gl(PiZ)}), 	// to
				1., 								// scale
				m::create<t_vec_gl>({0,0,0.5}),		// translate 
				m::create<t_vec_gl>({0,0,1}));		// from
			m_plot->GetImpl()->SetObjectMatrix(m_arrow_pi, matPi);

			// P_f
			t_mat_gl matPf = GlPlot_impl::GetArrowMatrix(
				m::create<t_vec_gl>({t_real_gl(P_f[0].real()), t_real_gl(P_f[1].real()), t_real_gl(P_f[2].real())}), 	// to
				1., 								// scale
				m::create<t_vec_gl>({0,0,0.5}),		// translate 
				m::create<t_vec_gl>({0,0,1}));		// from
			m_plot->GetImpl()->SetObjectMatrix(m_arrow_pf, matPf);

			// Re(M)
			t_mat_gl matMRe = GlPlot_impl::GetArrowMatrix(
				m::create<t_vec_gl>({t_real_gl(MPerpReX), t_real_gl(MPerpReY), t_real_gl(MPerpReZ)}), 	// to
				1., 								// scale
				m::create<t_vec_gl>({0,0,0.5}),		// translate 
				m::create<t_vec_gl>({0,0,1}));		// from
			m_plot->GetImpl()->SetObjectMatrix(m_arrow_M_Re, matMRe);

			// Im(M)
			t_mat_gl matMIm = GlPlot_impl::GetArrowMatrix(
				m::create<t_vec_gl>({t_real_gl(MPerpImX), t_real_gl(MPerpImY), t_real_gl(MPerpImZ)}), 	// to
				1., 								// scale
				m::create<t_vec_gl>({0,0,0.5}),		// translate 
				m::create<t_vec_gl>({0,0,1}));		// from
			m_plot->GetImpl()->SetObjectMatrix(m_arrow_M_Im, matMIm);
		}
	}
};
// ----------------------------------------------------------------------------




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
	set_gl_format(1, _GL_MAJ_VER, _GL_MIN_VER);
	set_locales();
	auto app = std::make_unique<QApplication>(argc, argv);

	auto dlg = std::make_unique<PolDlg>(nullptr);
	dlg->show();

	return app->exec();
}
// ----------------------------------------------------------------------------
