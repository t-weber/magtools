/**
 * GL plotter
 * @author Tobias Weber
 * @date Nov-2017 -- 2018
 * @license: see 'LICENSE.GPL' file
 *
 * References:
 *  * http://doc.qt.io/qt-5/qopenglwidget.html#details
 *  * http://code.qt.io/cgit/qt/qtbase.git/tree/examples/opengl/threadedqopenglwidget
 */

#include "glplot_nothread.h"

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QSurfaceFormat>
#include <QtGui/QPainter>
#include <QtGui/QGuiApplication>
#include <QtCore/QMutex>

#include <iostream>
#include <boost/scope_exit.hpp>


GlPlot_impl::GlPlot_impl(GlPlot *pPlot) : GlPlot_impl_base(pPlot)
{}

GlPlot_impl::~GlPlot_impl()
{}


void GlPlot_impl::paintGL()
{
	auto *pContext = m_pPlot->context();
	if(!pContext) return;
	QPainter painter(m_pPlot);
	painter.setRenderHint(QPainter::HighQualityAntialiasing);


	// gl painting
	{
		BOOST_SCOPE_EXIT(m_pPlot, &painter)
		{
// if the frame is not already updated by the timer, directly update it
#if _GL_USE_TIMER == 0
			QMetaObject::invokeMethod(m_pPlot, static_cast<void (QOpenGLWidget::*)()>(&QOpenGLWidget::update),
				Qt::ConnectionType::QueuedConnection);
#endif
			painter.endNativePainting();
		}
		BOOST_SCOPE_EXIT_END

		if(m_bPickerNeedsUpdate)
			UpdatePicker();


		auto *pGl = GetGlFunctions();
		painter.beginNativePainting();


		// clear
		pGl->glClearColor(1., 1., 1., 1.);
		pGl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		pGl->glEnable(GL_DEPTH_TEST);


		// bind shaders
		m_pShaders->bind();
		BOOST_SCOPE_EXIT(m_pShaders) { m_pShaders->release(); } BOOST_SCOPE_EXIT_END
		LOGGLERR(pGl);


		// set cam matrix
		m_pShaders->setUniformValue(m_uniMatrixCam, m_matCam);

		// render triangle geometry
		for(auto& obj : m_objs)
		{
			if(!obj.m_visible) continue;

			// main vertex array object
			pGl->glBindVertexArray(obj.m_vertexarr);

			m_pShaders->setUniformValue(m_uniMatrixObj, obj.m_mat);

			pGl->glEnableVertexAttribArray(m_attrVertex);
			if(obj.m_type == GlPlotObjType::TRIANGLES)
				pGl->glEnableVertexAttribArray(m_attrVertexNormal);
			pGl->glEnableVertexAttribArray(m_attrVertexColor);
			BOOST_SCOPE_EXIT(pGl, &m_attrVertex, &m_attrVertexNormal, &m_attrVertexColor)
			{
				pGl->glDisableVertexAttribArray(m_attrVertexColor);
				pGl->glDisableVertexAttribArray(m_attrVertexNormal);
				pGl->glDisableVertexAttribArray(m_attrVertex);
			}
			BOOST_SCOPE_EXIT_END
			LOGGLERR(pGl);

			if(obj.m_type == GlPlotObjType::TRIANGLES)
				pGl->glDrawArrays(GL_TRIANGLES, 0, obj.m_triangles.size());
			else if(obj.m_type == GlPlotObjType::LINES)
				pGl->glDrawArrays(GL_LINES, 0, obj.m_vertices.size());
			else
				std::cerr << "Error: Unknown plot object." << std::endl;

			LOGGLERR(pGl);
		}

		pGl->glDisable(GL_DEPTH_TEST);
	}


	// qt painting
	{
		// coordinate labels
		painter.drawText(GlToScreenCoords(m::create<t_vec_gl>({0.,0.,0.,1.})), "0");
		for(t_real_gl f=-2.; f<=2.; f+=0.5)
		{
			if(m::equals<t_real_gl>(f, 0))
				continue;

			std::ostringstream ostrF;
			ostrF << f;
			painter.drawText(GlToScreenCoords(m::create<t_vec_gl>({f,0.,0.,1.})), ostrF.str().c_str());
			painter.drawText(GlToScreenCoords(m::create<t_vec_gl>({0.,f,0.,1.})), ostrF.str().c_str());
			painter.drawText(GlToScreenCoords(m::create<t_vec_gl>({0.,0.,f,1.})), ostrF.str().c_str());
		}

		painter.drawText(GlToScreenCoords(m::create<t_vec_gl>({3.,0.,0.,1.})), "x");
		painter.drawText(GlToScreenCoords(m::create<t_vec_gl>({0.,3.,0.,1.})), "y");
		painter.drawText(GlToScreenCoords(m::create<t_vec_gl>({0.,0.,3.,1.})), "z");


		// render object labels
		for(auto& obj : m_objs)
		{
			if(!obj.m_visible) continue;

			if(obj.m_label != "")
			{
				t_vec3_gl posLabel3d = obj.m_mat * obj.m_labelPos;
				auto posLabel2d = GlToScreenCoords(m::create<t_vec_gl>({posLabel3d[0], posLabel3d[1], posLabel3d[2], 1.}));

				QFont fontOrig = painter.font();
				QPen penOrig = painter.pen();
				QFont fontLabel = fontOrig;
				QPen penLabel = penOrig;

				fontLabel.setWeight(QFont::Medium);
				painter.setFont(fontLabel);
				painter.drawText(posLabel2d, obj.m_label.c_str());

				fontLabel.setWeight(QFont::Normal);
				penLabel.setColor(QColor(int(obj.m_color[0]*255.), int(obj.m_color[1]*255.), int(obj.m_color[2]*255.), int(obj.m_color[3]*255.)));
				painter.setFont(fontLabel);
				painter.setPen(penLabel);
				painter.drawText(posLabel2d, obj.m_label.c_str());

				// restore original styles
				painter.setFont(fontOrig);
				painter.setPen(penOrig);
			}
		}
	}
}


// ----------------------------------------------------------------------------
// GLPlot wrapper class

GlPlot::GlPlot(QWidget *pParent) : QOpenGLWidget(pParent), m_impl(std::make_unique<GlPlot_impl>(this))
{
	setMouseTracking(true);
}

GlPlot::~GlPlot()
{
	setMouseTracking(false);
}


void GlPlot::initializeGL()
{
	m_impl->initialiseGL();
	emit AfterGLInitialisation();
}

void GlPlot::resizeGL(int w, int h)
{
	m_impl->SetScreenDims(w, h);
	m_impl->resizeGL();
}

void GlPlot::paintGL()
{
	m_impl->paintGL();
}


void GlPlot::mouseMoveEvent(QMouseEvent *pEvt)
{
	m_impl->mouseMoveEvent(pEvt->localPos());
	pEvt->accept();
}

void GlPlot::mousePressEvent(QMouseEvent *pEvt)
{
	bool bLeftDown = 0, bRightDown = 0, bMidDown = 0;

	if(pEvt->buttons() & Qt::LeftButton) bLeftDown = 1;
	if(pEvt->buttons() & Qt::RightButton) bRightDown = 1;
	if(pEvt->buttons() & Qt::MiddleButton) bMidDown = 1;

	if(bMidDown)
		m_impl->ResetZoom();
	if(bRightDown)
		m_impl->BeginRotation();

	pEvt->accept();
	emit MouseDown(bLeftDown, bMidDown, bRightDown);
}

void GlPlot::mouseReleaseEvent(QMouseEvent *pEvt)
{
	bool bLeftUp = 0, bRightUp = 0, bMidUp = 0;

	if((pEvt->buttons() & Qt::LeftButton) == 0) bLeftUp = 1;
	if((pEvt->buttons() & Qt::RightButton) == 0) bRightUp = 1;
	if((pEvt->buttons() & Qt::MiddleButton) == 0) bMidUp = 1;

	if(bRightUp)
		m_impl->EndRotation();

	pEvt->accept();
	emit MouseUp(bLeftUp, bMidUp, bRightUp);
}

void GlPlot::wheelEvent(QWheelEvent *pEvt)
{
	const t_real_gl degrees = pEvt->angleDelta().y() / 8.;
	m_impl->zoom(degrees);

	pEvt->accept();
}

// ----------------------------------------------------------------------------
