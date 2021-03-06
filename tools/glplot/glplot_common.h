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

#ifndef __GL_PLOTTER_COMMON_H__
#define __GL_PLOTTER_COMMON_H__


#define _GL_USE_TIMER 0

#include <QtCore/QTimer>
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

// GL surface format
extern void set_gl_format(bool bCore=true, int iMajorVer=3, int iMinorVer=3, int iSamples=8);


// error codes: https://www.khronos.org/opengl/wiki/OpenGL_Error
#define LOGGLERR(pGl) { if(auto err = pGl->glGetError(); err != GL_NO_ERROR) \
	std::cerr << "gl error in " << __func__ \
	<< " line " << std::dec <<  __LINE__  << ": " \
	<< std::hex << err << std::endl; }
// ----------------------------------------------------------------------------


using t_real_gl = GLfloat;
using t_vec3_gl = m::qvecN_adapter<int, 3, t_real_gl, QVector3D>;
using t_vec_gl = m::qvecN_adapter<int, 4, t_real_gl, QVector4D>;
using t_mat_gl = m::qmatNN_adapter<int, 4, 4, t_real_gl, QMatrix4x4>;


// forward declarations
class GlPlot_impl_base;
class GlPlot_impl;
class GlPlot;


enum class GlPlotObjType
{
	TRIANGLES,
	LINES
};


struct GlPlotObj
{
	friend class GlPlot_impl_base;
	friend class GlPlot_impl;

private:
	GlPlotObjType m_type = GlPlotObjType::TRIANGLES;
	GLuint m_vertexarr = 0;

	std::shared_ptr<QOpenGLBuffer> m_pvertexbuf;
	std::shared_ptr<QOpenGLBuffer> m_pnormalsbuf;
	std::shared_ptr<QOpenGLBuffer> m_pcolorbuf;

	std::vector<t_vec3_gl> m_vertices, m_triangles;
	t_vec_gl m_color = m::create<t_vec_gl>({ 0., 0., 1., 1. });	// rgba

	t_mat_gl m_mat = m::unit<t_mat_gl>();

	bool m_visible = true;		// object shown?
	//std::vector<t_vec3_gl> m_pickerInters;		// intersections with mouse picker?

	t_vec3_gl m_labelPos = m::create<t_vec3_gl>({0., 0., 0.});
	std::string m_label;

public:
	GlPlotObj() = default;
	~GlPlotObj() = default;
};



class GlPlot_impl_base : public QObject
{ Q_OBJECT
protected:
	GlPlot *m_pPlot = nullptr;
	std::string m_strGlDescr;
	std::shared_ptr<QOpenGLShaderProgram> m_pShaders;

	GLint m_attrVertex = -1;
	GLint m_attrVertexNormal = -1;
	GLint m_attrVertexColor = -1;
	GLint m_uniMatrixProj = -1;
	GLint m_uniMatrixCam = -1;
	GLint m_uniMatrixObj = -1;

	t_mat_gl m_matPerspective = m::unit<t_mat_gl>();
	t_mat_gl m_matPerspective_inv = m::unit<t_mat_gl>();
	t_mat_gl m_matViewport = m::unit<t_mat_gl>();
	t_mat_gl m_matViewport_inv = m::unit<t_mat_gl>();
	t_mat_gl m_matCamBase = m::create<t_mat_gl>({1,0,0,0,  0,1,0,0,  0,0,1,-5,  0,0,0,1});
	t_mat_gl m_matCamRot = m::unit<t_mat_gl>();
	t_mat_gl m_matCam = m::unit<t_mat_gl>();
	t_mat_gl m_matCam_inv = m::unit<t_mat_gl>();
	t_vec_gl m_vecCamX = m::create<t_vec_gl>({1.,0.,0.,0.});
	t_vec_gl m_vecCamY = m::create<t_vec_gl>({0.,1.,0.,0.});
	t_real_gl m_phi_saved = 0, m_theta_saved = 0;
	t_real_gl m_zoom = 1.;

	std::atomic<bool> m_bInitialised = false;
	std::atomic<bool> m_bWantsResize = false;
	std::atomic<bool> m_bPickerNeedsUpdate = false;
	std::atomic<int> m_iScreenDims[2] = { 800, 600 };
	t_real_gl m_pickerSphereRadius = 1;

	std::vector<GlPlotObj> m_objs;

	QPointF m_posMouse;
	QPointF m_posMouseRotationStart, m_posMouseRotationEnd;
	bool m_bInRotation = false;

	#if _GL_USE_TIMER != 0
	QTimer m_timer;
	#endif

protected:
	qgl_funcs* GetGlFunctions(QOpenGLWidget *pWidget = nullptr);

	void UpdateCam();
	void UpdatePicker();

	void tick(const std::chrono::milliseconds& ms);

public:
	GlPlot_impl_base(GlPlot *pPlot = nullptr);
	virtual ~GlPlot_impl_base();

	const std::string& GetGlDescr() const { return m_strGlDescr; }

	QPointF GlToScreenCoords(const t_vec_gl& vec, bool *pVisible=nullptr);
	static t_mat_gl GetArrowMatrix(const t_vec_gl& vecTo, t_real_gl scale,
		const t_vec_gl& vecTrans = m::create<t_vec_gl>({0,0,0.5}), const t_vec_gl& vecFrom = m::create<t_vec_gl>({0,0,1}));

	void SetCamBase(const t_mat_gl& mat, const t_vec_gl& vecX, const t_vec_gl& vecY)
	{ m_matCamBase = mat; m_vecCamX = vecX; m_vecCamY = vecY; UpdateCam(); }
	void SetPickerSphereRadius(t_real_gl rad) { m_pickerSphereRadius = rad; }

	GlPlotObj CreateTriangleObject(const std::vector<t_vec3_gl>& verts,
		const std::vector<t_vec3_gl>& triag_verts, const std::vector<t_vec3_gl>& norms,
		const t_vec_gl& color, bool bUseVertsAsNorm=false);
	GlPlotObj CreateLineObject(const std::vector<t_vec3_gl>& verts, const t_vec_gl& color);

	std::size_t AddSphere(t_real_gl rad=1,
		t_real_gl x=0, t_real_gl y=0, t_real_gl z=0,
		t_real_gl r=0, t_real_gl g=0, t_real_gl b=0, t_real_gl a=1);
	std::size_t AddCylinder(t_real_gl rad=1, t_real_gl h=1,
		t_real_gl x=0, t_real_gl y=0, t_real_gl z=0,
		t_real_gl r=0, t_real_gl g=0, t_real_gl b=0, t_real_gl a=1);
	std::size_t AddCone(t_real_gl rad=1, t_real_gl h=1,
		t_real_gl x=0, t_real_gl y=0, t_real_gl z=0,
		t_real_gl r=0, t_real_gl g=0, t_real_gl b=0, t_real_gl a=1);
	std::size_t AddArrow(t_real_gl rad=1, t_real_gl h=1,
		t_real_gl x=0, t_real_gl y=0, t_real_gl z=0,
		t_real_gl r=0, t_real_gl g=0, t_real_gl b=0, t_real_gl a=1);
	std::size_t AddCoordinateCross(t_real_gl min, t_real_gl max);

	void SetObjectMatrix(std::size_t idx, const t_mat_gl& mat);
	void SetObjectLabel(std::size_t idx, const std::string& label);
	void SetObjectVisible(std::size_t idx, bool visible);

	void SetScreenDims(int w, int h);

public /*slots*/:
	void initialiseGL();
	void resizeGL();

	void mouseMoveEvent(const QPointF& pos);
	void zoom(t_real_gl val);
	void ResetZoom();

	void BeginRotation();
	void EndRotation();

protected slots:
	void tick();

signals:
	void PickerIntersection(const t_vec3_gl* pos, std::size_t objIdx, const t_vec3_gl* posSphere);
};


#endif
