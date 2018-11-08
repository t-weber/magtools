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

#include "glplot.h"

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QSurfaceFormat>
#include <QtGui/QPainter>
#include <QtGui/QGuiApplication>

#include <iostream>
#include <boost/scope_exit.hpp>
#include <boost/algorithm/string/replace.hpp>
namespace algo = boost::algorithm;



GlPlot_impl::GlPlot_impl(GlPlot *pPlot) : GlPlot_impl_base(pPlot)
{}

GlPlot_impl::~GlPlot_impl()
{}


void GlPlot_impl::startedThread()
{
}

void GlPlot_impl::stoppedThread()
{
}


void GlPlot_impl::paintGL()
{
	QThread *pThisThread = QThread::currentThread();
	if(!pThisThread->isRunning() || pThisThread->isInterruptionRequested())
		return;

	auto *pContext = m_pPlot->context();
	if(!pContext) return;

	QMetaObject::invokeMethod(m_pPlot, &GlPlot::MoveContextToThread, Qt::ConnectionType::BlockingQueuedConnection);
	if(!m_pPlot->IsContextInThread())
	{
		std::cerr << __func__ << ": Context is not in thread!" << std::endl;
		return;
	}

	m_pPlot->GetMutex()->lock();
	m_pPlot->makeCurrent();
	BOOST_SCOPE_EXIT(m_pPlot)
	{
		m_pPlot->doneCurrent();
		m_pPlot->context()->moveToThread(qGuiApp->thread());
		m_pPlot->GetMutex()->unlock();

// if the frame is not already updated by the timer, directly update it
#if _GL_USE_TIMER == 0
		QMetaObject::invokeMethod(m_pPlot, static_cast<void (QOpenGLWidget::*)()>(&QOpenGLWidget::update),
			Qt::ConnectionType::QueuedConnection);
#endif
	}
	BOOST_SCOPE_EXIT_END

	if(!m_bInitialised)
		initialiseGL();
	if(!m_bInitialised)
	{
		std::cerr << "Cannot initialise GL." << std::endl;
		return;
	}

	if(m_bWantsResize)
		resizeGL();
	if(m_bPickerNeedsUpdate)
		UpdatePicker();


	auto *pGl = GetGlFunctions();

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



// ----------------------------------------------------------------------------
// GLPlot wrapper class

GlPlot::GlPlot(QWidget *pParent) : QOpenGLWidget(pParent),
	m_thread_impl(std::make_unique<QThread>(this)),
	m_impl(std::make_unique<GlPlot_impl>(this))
{
	m_impl->moveToThread(m_thread_impl.get());

	connect(this, &QOpenGLWidget::aboutToCompose, this, &GlPlot::beforeComposing);
	connect(this, &QOpenGLWidget::frameSwapped, this, &GlPlot::afterComposing);
	connect(this, &QOpenGLWidget::aboutToResize, this, &GlPlot::beforeResizing);
	connect(this, &QOpenGLWidget::resized, this, &GlPlot::afterResizing);

	connect(m_thread_impl.get(), &QThread::started, m_impl.get(), &GlPlot_impl::startedThread);
	connect(m_thread_impl.get(), &QThread::finished, m_impl.get(), &GlPlot_impl::stoppedThread);

	m_thread_impl->start();
	setMouseTracking(true);
}


GlPlot::~GlPlot()
{
	setMouseTracking(false);

	m_thread_impl->requestInterruption();
	m_thread_impl->exit();
	m_thread_impl->wait();
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
}

void GlPlot::wheelEvent(QWheelEvent *pEvt)
{
	const t_real_gl degrees = pEvt->angleDelta().y() / 8.;
	m_impl->zoom(degrees);

	pEvt->accept();
}


/**
 * move the GL context to the associated thread
 */
void GlPlot::MoveContextToThread()
{
	auto *pContext = context();
	if(pContext)
		pContext->moveToThread(m_thread_impl.get());
}


/**
 * does the GL context run in the current thread?
 */
bool GlPlot::IsContextInThread() const
{
	auto *pContext = context();
	if(!pContext) return false;

	return pContext->thread() == m_thread_impl.get();
}


/**
 * main thread wants to compose -> wait for sub-threads to be finished
 */
void GlPlot::beforeComposing()
{
	m_mutex.lock();
}

/**
 * main thread has composed -> sub-threads can be unblocked
 */
void GlPlot::afterComposing()
{
	m_mutex.unlock();

	QMetaObject::invokeMethod(m_impl.get(), &GlPlot_impl::paintGL, Qt::ConnectionType::QueuedConnection);
}


/**
 * main thread wants to resize -> wait for sub-threads to be finished
 */
void GlPlot::beforeResizing()
{
	m_mutex.lock();
}

/**
 * main thread has resized -> sub-threads can be unblocked
 */
void GlPlot::afterResizing()
{
	m_mutex.unlock();

	const int w = width(), h = height();
	m_impl->SetScreenDims(w, h);
}

// ----------------------------------------------------------------------------
