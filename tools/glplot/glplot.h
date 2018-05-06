/**
 * GL plotter
 * @author Tobias Weber
 * @date Nov-2017
 * @license: see 'LICENSE.GPL' file
 *
 *  * References:
 *  * http://doc.qt.io/qt-5/qopenglwidget.html#details
 *  * http://code.qt.io/cgit/qt/qtbase.git/tree/examples/opengl/threadedqopenglwidget
 */

#ifndef __MAG_GL_PLOT_H__
#define __MAG_GL_PLOT_H__

// ----------------------------------------------------------------------------
// GL versions
#if !defined(_GL_MAJ_VER) || !defined(_GL_MIN_VER)
	#define _GL_MAJ_VER 3
	#define _GL_MIN_VER 3
#endif

// GL functions include
#define _GL_INC_IMPL(MAJ, MIN) <QtGui/QOpenGLFunctions_ ## MAJ ## _ ## MIN ## _Core>
#define _GL_INC(MAJ, MIN) _GL_INC_IMPL(MAJ, MIN)
#include _GL_INC(_GL_MAJ_VER, _GL_MIN_VER)

// GL functions typedef
#define _GL_FUNC_IMPL(MAJ, MIN) QOpenGLFunctions_ ## MAJ ## _ ## MIN ## _Core
#define _GL_FUNC(MAJ, MIN) _GL_FUNC_IMPL(MAJ, MIN)
using qgl_funcs = _GL_FUNC(_GL_MAJ_VER, _GL_MIN_VER);
// ----------------------------------------------------------------------------

#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtWidgets/QDialog>
#include <QtGui/QMouseEvent>

#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLBuffer>
#include <QtWidgets/QOpenGLWidget>

#include <QtGui/QMatrix4x4>
#include <QtGui/QVector4D>
#include <QtGui/QVector3D>

#include <memory>
#include <chrono>
#include <atomic>
#include "../../libs/math_algos.h"

using t_real_gl = GLfloat;
using t_vec3_gl = m::qvecN_adapter<int, 3, t_real_gl, QVector3D>;
using t_vec_gl = m::qvecN_adapter<int, 4, t_real_gl, QVector4D>;
using t_mat_gl = m::qmatNN_adapter<int, 4, 4, t_real_gl, QMatrix4x4>;


struct GlPlotObj
{
	GLuint m_vertexarr = 0;

	std::shared_ptr<QOpenGLBuffer> m_pvertexbuf;
	std::shared_ptr<QOpenGLBuffer> m_pnormalsbuf;
	std::shared_ptr<QOpenGLBuffer> m_pcolorbuf;

	std::vector<t_vec3_gl> m_vertices, m_triangles;
	t_real_gl m_color[4] = { 0., 0., 1., 1. };	// rgba

	t_mat_gl m_mat = m::unit<t_mat_gl>();
};


class GlPlot;

class GlPlot_impl : public QObject
{
public:
	GlPlot_impl(GlPlot* pPlot);
	virtual ~GlPlot_impl();

public:
	QPointF GlToScreenCoords(const t_vec_gl& vec, bool *pVisible=nullptr);

protected:
	qgl_funcs* GetGlFunctions();

	void tick(const std::chrono::milliseconds& ms);
	void updatePicker();

private:
	std::atomic<bool> m_bInitialised = false;
	std::atomic<bool> m_bWantsResize = false;
	GlPlot *m_pPlot = nullptr;

	std::vector<GlPlotObj> m_objs;

	std::shared_ptr<QOpenGLShaderProgram> m_pShaders;

	t_mat_gl m_matPerspective, m_matPerspective_inv;
	t_mat_gl m_matViewport, m_matViewport_inv;
	t_mat_gl m_matCam, m_matCam_inv;

	GLint m_attrVertex = -1;
	GLint m_attrVertexNormal = -1;
	GLint m_attrVertexColor = -1;
	GLint m_uniMatrixProj = -1;
	GLint m_uniMatrixCam = -1;
	GLint m_uniMatrixObj = -1;

	std::atomic<int> m_iScreenDims[2] = { 800, 600 };
	QPointF m_posMouse;

	QTimer m_timer;

public:
	void SetScreenDims(int w, int h);

public:
	GlPlotObj CreateObject(const std::vector<t_vec3_gl>& verts, const std::vector<t_vec3_gl>& triag_verts, const std::vector<t_vec3_gl>& norms);
	GlPlotObj CreateSphere(t_real_gl rad=1, t_real_gl x=0, t_real_gl y=0, t_real_gl z=0);

protected slots:
	void tick();

public slots:
	void paintGL();
	void initialiseGL();
	void resizeGL();

	void startedThread();
	void stoppedThread();

	void mouseMoveEvent(const QPointF& pos);
};


class GlPlot : public QOpenGLWidget
{
public:
	using QOpenGLWidget::QOpenGLWidget;

	GlPlot(QWidget *pParent);
	virtual ~GlPlot();

protected:
	virtual void paintEvent(QPaintEvent*) override {}; /* empty for threading */
	virtual void mouseMoveEvent(QMouseEvent *pEvt) override;

protected slots:
	void beforeComposing();
	void afterComposing();
	void beforeResizing();
	void afterResizing();

private:
	mutable QMutex m_mutex;
	std::unique_ptr<GlPlot_impl> m_impl;
	std::unique_ptr<QThread> m_thread_impl;

public:
	QMutex* GetMutex() { return &m_mutex; }

	void MoveContextToThread();
	bool IsContextInThread() const;
};


#endif
