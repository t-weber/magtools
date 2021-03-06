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
{ /*Q_OBJECT*/
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
	bool m_mouseDown[3] = {false, false, false};
	long m_curPickedObj = -1;
	long m_curDraggedObj = -1;


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
			m_arrow_pf = m_plot->GetImpl()->AddArrow(0.05, 1., 0.,0.,0.5,  0.,0.5,0.,1.);
			m_arrow_M_Re = m_plot->GetImpl()->AddArrow(0.05, 1., 0.,0.,0.5,  0.85,0.,0.,1.);
			m_arrow_M_Im = m_plot->GetImpl()->AddArrow(0.05, 1., 0.,0.,0.5,  0.85,0.25,0.,1.);

			m_plot->GetImpl()->SetObjectLabel(m_arrow_pi, "P_i");
			m_plot->GetImpl()->SetObjectLabel(m_arrow_pf, "P_f");
			m_plot->GetImpl()->SetObjectLabel(m_arrow_M_Re, "Re{M_perp}");
			m_plot->GetImpl()->SetObjectLabel(m_arrow_M_Im, "Im{M_perp}");

			m_3dobjsReady = true;
			CalcPol();
		}
	}


	/**
	 * get the length of a vector
	 */
	t_real GetArrowLen(std::size_t objIdx) const
	{
		if(objIdx == m_arrow_pi)
		{
			return std::sqrt(std::pow(m_editPiX->text().toDouble(), 2.)
				+ std::pow(m_editPiY->text().toDouble(), 2.)
				+ std::pow(m_editPiZ->text().toDouble(), 2.));
		}
		else if(objIdx == m_arrow_pf)
		{
			return std::sqrt(std::pow(m_editPfX->text().toDouble(), 2.)
				+ std::pow(m_editPfY->text().toDouble(), 2.)
				+ std::pow(m_editPfZ->text().toDouble(), 2.));
		}
		else if(objIdx == m_arrow_M_Re)
		{
			return std::sqrt(std::pow(m_editMPerpReX->text().toDouble(), 2.)
				+ std::pow(m_editMPerpReY->text().toDouble(), 2.)
				+ std::pow(m_editMPerpReZ->text().toDouble(), 2.));
		}
		else if(objIdx == m_arrow_M_Im)
		{
			return std::sqrt(std::pow(m_editMPerpImX->text().toDouble(), 2.)
				+ std::pow(m_editMPerpImY->text().toDouble(), 2.)
				+ std::pow(m_editMPerpImZ->text().toDouble(), 2.));
		}

		return -1.;
	}


	/**
	 * called when the mouse hovers over an object
	 */
	void PickerIntersection(const t_vec3_gl* pos, std::size_t objIdx, const t_vec3_gl* posSphere)
	{
		/*if(pos)
		{
			std::cout << "Intersecting object " << objIdx << " at position "
				<< (*pos)[0] << ", " << (*pos)[1] << ", " << (*pos)[2] << std::endl;
		}*/


		m_curPickedObj = -1;
		if(pos)
		{	// object selected?
			m_curPickedObj = long(objIdx);

			if(objIdx == m_arrow_pi) m_labelStatus->setText("P_i");
			else if(objIdx == m_arrow_pf) m_labelStatus->setText("P_f");
			else if(objIdx == m_arrow_M_Re) m_labelStatus->setText("Re{M_perp}");
			else if(objIdx == m_arrow_M_Im) m_labelStatus->setText("Im{M_perp}");
			else { m_labelStatus->setText(""); m_curPickedObj = -1; }
		}
		else m_labelStatus->setText("");


		if(posSphere && m_mouseDown[0])
		{	// picker intersecting unit sphere and mouse dragged?
			/*std::cout << "Dragging object " << m_curDraggedObj << " to "
				<< (*posSphere)[0] << ", " << (*posSphere)[1] << ", " << (*posSphere)[2] << std::endl;*/

			t_vec3_gl posSph = *posSphere;

			if(m_curDraggedObj == m_arrow_pi)
			{
				m_editPiX->setText(QVariant(posSph[0]).toString());
				m_editPiY->setText(QVariant(posSph[1]).toString());
				m_editPiZ->setText(QVariant(posSph[2]).toString());
				CalcPol();
			}
			else if(m_curDraggedObj == m_arrow_M_Re)
			{
				m_editMPerpReX->setText(QVariant(posSph[0]).toString());
				m_editMPerpReY->setText(QVariant(posSph[1]).toString());
				m_editMPerpReZ->setText(QVariant(posSph[2]).toString());
				CalcPol();
			}
			else if(m_curDraggedObj == m_arrow_M_Im)
			{
				m_editMPerpImX->setText(QVariant(posSph[0]).toString());
				m_editMPerpImY->setText(QVariant(posSph[1]).toString());
				m_editMPerpImZ->setText(QVariant(posSph[2]).toString());
				CalcPol();
			}
		}
	}


	/**
	 * mouse button pressed
	 */
	void MouseDown(bool left, bool mid, bool right)
	{
		if(left) m_mouseDown[0] = true;
		if(mid) m_mouseDown[1] = true;
		if(right) m_mouseDown[2] = true;

		if(m_mouseDown[0])
		{
			m_curDraggedObj = m_curPickedObj;
			auto lenVec = GetArrowLen(m_curDraggedObj);
			if(lenVec > 0.)
				m_plot->GetImpl()->SetPickerSphereRadius(lenVec);
		}
	}


	/**
	 * mouse button released
	 */
	void MouseUp(bool left, bool mid, bool right)
	{
		if(left) m_mouseDown[0] = false;
		if(mid) m_mouseDown[1] = false;
		if(right) m_mouseDown[2] = false;

		if(!m_mouseDown[0])
			m_curDraggedObj = -1;
	}


public:
	PolDlg() = delete;

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

		connect(m_plot.get(), &GlPlot::MouseDown, this, &PolDlg::MouseDown);
		connect(m_plot.get(), &GlPlot::MouseUp, this, &PolDlg::MouseUp);


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


		// have scattering plane in horizontal plane
		m_plot->GetImpl()->SetCamBase(m::create<t_mat_gl>({1,0,0,0,  0,0,1,0,  0,-1,0,-5,  0,0,0,1}),
			m::create<t_vec_gl>({1,0,0,0}), m::create<t_vec_gl>({0,0,1,0}));
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
			const t_real_gl lenReM = t_real_gl(std::sqrt(MPerpReX*MPerpReX + MPerpReY*MPerpReY + MPerpReZ*MPerpReZ));
			t_mat_gl matMRe = GlPlot_impl::GetArrowMatrix(
				m::create<t_vec_gl>({t_real_gl(MPerpReX), t_real_gl(MPerpReY), t_real_gl(MPerpReZ)}), 	// to
				lenReM,								// scale
				m::create<t_vec_gl>({0,0,0.5}),		// translate 
				m::create<t_vec_gl>({0,0,1}));		// from
			m_plot->GetImpl()->SetObjectMatrix(m_arrow_M_Re, matMRe);
			m_plot->GetImpl()->SetObjectVisible(m_arrow_M_Re, !m::equals(lenReM, t_real_gl(0)));

			// Im(M)
			const t_real_gl lenImM = t_real_gl(std::sqrt(MPerpImX*MPerpImX + MPerpImY*MPerpImY + MPerpImZ*MPerpImZ));
			t_mat_gl matMIm = GlPlot_impl::GetArrowMatrix(
				m::create<t_vec_gl>({t_real_gl(MPerpImX), t_real_gl(MPerpImY), t_real_gl(MPerpImZ)}), 	// to
				lenImM,								// scale
				m::create<t_vec_gl>({0,0,0.5}),		// translate 
				m::create<t_vec_gl>({0,0,1}));		// from
			m_plot->GetImpl()->SetObjectMatrix(m_arrow_M_Im, matMIm);
			m_plot->GetImpl()->SetObjectVisible(m_arrow_M_Im, !m::equals(lenImM, t_real_gl(0)));
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
	set_gl_format(1, _GL_MAJ_VER, _GL_MIN_VER, 8);
	set_locales();
	auto app = std::make_unique<QApplication>(argc, argv);

	auto dlg = std::make_unique<PolDlg>(nullptr);
	dlg->show();

	return app->exec();
}
// ----------------------------------------------------------------------------

//#include "pol.moc"
