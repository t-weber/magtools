/**
 * GL plotter
 * @author Tobias Weber
 * @date Nov-2017
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


// ----------------------------------------------------------------------------
// error codes: https://www.khronos.org/opengl/wiki/OpenGL_Error
#define LOGGLERR(pGl) { if(auto err = pGl->glGetError(); err != GL_NO_ERROR) \
	std::cerr << "gl error in " << __func__ \
		<< " line " << std::dec <<  __LINE__  << ": " \
		<< std::hex << err << std::endl; }


GlPlot_impl::GlPlot_impl(GlPlot *pPlot) : m_pPlot{pPlot},
	m_matPerspective{m::unit<t_mat_gl>()}, m_matPerspective_inv{m::unit<t_mat_gl>()},
	m_matViewport{m::unit<t_mat_gl>()}, m_matViewport_inv{m::unit<t_mat_gl>()},
	m_matCam{m::unit<t_mat_gl>()}, m_matCam_inv{m::unit<t_mat_gl>()},
	m_matCamRot{m::unit<t_mat_gl>()}
{
#if _GL_USE_TIMER != 0
	connect(&m_timer, &QTimer::timeout, this, static_cast<void (GlPlot_impl::*)()>(&GlPlot_impl::tick));
	m_timer.start(std::chrono::milliseconds(1000 / 60));
#endif

	updateCam();
}


GlPlot_impl::~GlPlot_impl()
{
#if _GL_USE_TIMER != 0
	m_timer.stop();
#endif
}


qgl_funcs* GlPlot_impl::GetGlFunctions()
{
	qgl_funcs *pGl = nullptr;

	if constexpr(std::is_same_v<qgl_funcs, QOpenGLFunctions>)
		pGl = (qgl_funcs*)m_pPlot->context()->functions();
	else
		pGl = (qgl_funcs*)m_pPlot->context()->versionFunctions<qgl_funcs>();

	if(!pGl)
		std::cerr << "No suitable GL interface found." << std::endl;

	return pGl;
}


GlPlotObj GlPlot_impl::CreateObject(const std::vector<t_vec3_gl>& verts,
	const std::vector<t_vec3_gl>& triagverts, const std::vector<t_vec3_gl>& norms,
	const t_vec_gl& color, bool bUseVertsAsNorm)
{
	qgl_funcs* pGl = GetGlFunctions();
	GLint attrVertex = m_attrVertex;
	GLint attrVertexNormal = m_attrVertexNormal;
	GLint attrVertexColor = m_attrVertexColor;

	GlPlotObj obj;
	obj.m_color = color;

	// flatten vertex array into raw float array
	auto to_float_array = [](const std::vector<t_vec3_gl>& verts, int iRepeat=1, int iElems=3, bool bNorm=false)
		-> std::vector<t_real_gl>
	{
		std::vector<t_real_gl> vecRet;
		vecRet.reserve(iRepeat*verts.size()*iElems);

		for(const t_vec3_gl& vert : verts)
		{
			t_real_gl norm = bNorm ? m::norm<t_vec3_gl>(vert) : 1;

			for(int i=0; i<iRepeat; ++i)
				for(int iElem=0; iElem<iElems; ++iElem)
					vecRet.push_back(vert[iElem] / norm);
		}

		return vecRet;
	};

	// main vertex array object
	pGl->glGenVertexArrays(1, &obj.m_vertexarr);
	pGl->glBindVertexArray(obj.m_vertexarr);

	{	// vertices
		obj.m_pvertexbuf = std::make_shared<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);

		obj.m_pvertexbuf->create();
		obj.m_pvertexbuf->bind();
		BOOST_SCOPE_EXIT(&obj) { obj.m_pvertexbuf->release(); } BOOST_SCOPE_EXIT_END

		auto vecVerts = to_float_array(triagverts, 1,3, false);
		obj.m_pvertexbuf->allocate(vecVerts.data(), vecVerts.size()*sizeof(typename decltype(vecVerts)::value_type));
		pGl->glVertexAttribPointer(attrVertex, 3, GL_FLOAT, 0, 0, nullptr);
	}

	{	// normals
		obj.m_pnormalsbuf = std::make_shared<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);

		obj.m_pnormalsbuf->create();
		obj.m_pnormalsbuf->bind();
		BOOST_SCOPE_EXIT(&obj) { obj.m_pnormalsbuf->release(); } BOOST_SCOPE_EXIT_END

		auto vecNorms = bUseVertsAsNorm ?
			to_float_array(triagverts, 1,3, true) :
			to_float_array(norms, 3,3, false);
		obj.m_pnormalsbuf->allocate(vecNorms.data(), vecNorms.size()*sizeof(typename decltype(vecNorms)::value_type));
		pGl->glVertexAttribPointer(attrVertexNormal, 3, GL_FLOAT, 0, 0, nullptr);
	}

	{	// colors
		obj.m_pcolorbuf = std::make_shared<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);

		obj.m_pcolorbuf->create();
		obj.m_pcolorbuf->bind();
		BOOST_SCOPE_EXIT(&obj) { obj.m_pcolorbuf->release(); } BOOST_SCOPE_EXIT_END

		std::vector<t_real_gl> vecCols;
		vecCols.reserve(4*triagverts.size());
		for(std::size_t iVert=0; iVert<triagverts.size(); ++iVert)
		{
			for(int icol=0; icol<obj.m_color.size(); ++icol)
				vecCols.push_back(obj.m_color[icol]);
		}

		obj.m_pcolorbuf->allocate(vecCols.data(), vecCols.size()*sizeof(typename decltype(vecCols)::value_type));
		pGl->glVertexAttribPointer(attrVertexColor, 4, GL_FLOAT, 0, 0, nullptr);
	}


	obj.m_vertices = std::move(verts);
	obj.m_triangles = std::move(triagverts);

	return obj;
}


GlPlotObj GlPlot_impl::CreateSphere(t_real_gl rad, t_real_gl x, t_real_gl y, t_real_gl z,
	t_real_gl r, t_real_gl g, t_real_gl b, t_real_gl a)
{
	auto solid = m::create_icosahedron<t_vec3_gl>(1);
	auto [triagverts, norms, uvs] = m::spherify<t_vec3_gl>(
		m::subdivide_triangles<t_vec3_gl>(
			m::create_triangles<t_vec3_gl>(solid), 2), rad);

	auto obj = CreateObject(std::get<0>(solid), triagverts, norms, m::create<t_vec_gl>({r,g,b,a}), true);

	obj.m_mat(0,3) = x;
	obj.m_mat(1,3) = y;
	obj.m_mat(2,3) = z;

	return obj;
}


GlPlotObj GlPlot_impl::CreateCylinder(t_real_gl rad, t_real_gl h, t_real_gl x, t_real_gl y, t_real_gl z,
	t_real_gl r, t_real_gl g, t_real_gl b, t_real_gl a)
{
	auto solid = m::create_cylinder<t_vec3_gl>(rad, h, true);
	auto [triagverts, norms, uvs] = m::create_triangles<t_vec3_gl>(solid);

	auto obj = CreateObject(std::get<0>(solid), triagverts, norms, m::create<t_vec_gl>({r,g,b,a}), false);

	obj.m_mat(0,3) = x;
	obj.m_mat(1,3) = y;
	obj.m_mat(2,3) = z;

	return obj;
}


GlPlotObj GlPlot_impl::CreateCone(t_real_gl rad, t_real_gl h, t_real_gl x, t_real_gl y, t_real_gl z,
	t_real_gl r, t_real_gl g, t_real_gl b, t_real_gl a)
{
	auto solid = m::create_cone<t_vec3_gl>(rad, h);
	auto [triagverts, norms, uvs] = m::create_triangles<t_vec3_gl>(solid);

	auto obj = CreateObject(std::get<0>(solid), triagverts, norms, m::create<t_vec_gl>({r,g,b,a}), false);

	obj.m_mat(0,3) = x;
	obj.m_mat(1,3) = y;
	obj.m_mat(2,3) = z;

	return obj;
}


GlPlotObj GlPlot_impl::CreateArrow(t_real_gl rad, t_real_gl h, t_real_gl x, t_real_gl y, t_real_gl z,
	t_real_gl r, t_real_gl g, t_real_gl b, t_real_gl a)
{
	auto solid = m::create_cylinder<t_vec3_gl>(rad, h, 2, 32, rad, h);
	auto [triagverts, norms, uvs] = m::create_triangles<t_vec3_gl>(solid);

	auto obj = CreateObject(std::get<0>(solid), triagverts, norms, m::create<t_vec_gl>({r,g,b,a}), false);

	obj.m_mat(0,3) = x;
	obj.m_mat(1,3) = y;
	obj.m_mat(2,3) = z;

	return obj;
}



void GlPlot_impl::initialiseGL()
{
	// --------------------------------------------------------------------
	// shaders
	// --------------------------------------------------------------------
	std::string strFragShader = R"RAW(
		#version ${GLSL_VERSION}

		in vec4 fragcolor;
		out vec4 outcolor;

		void main()
		{
			//outcolor = vec4(0,0,0,1);
			outcolor = fragcolor;
		})RAW";
	// --------------------------------------------------------------------


	// --------------------------------------------------------------------
	std::string strVertexShader = R"RAW(
		#version ${GLSL_VERSION}

		in vec4 vertex;
		in vec4 normal;
		in vec4 vertexcolor;
		out vec4 fragcolor;

		uniform mat4 proj = mat4(1.);
		uniform mat4 cam = mat4(1.);
		uniform mat4 obj = mat4(1.);

		//vec4 vertexcolor = vec4(0, 0, 1, 1);
		vec3 light_dir = vec3(2, 2, -1);

		float lighting(vec3 lightdir)
		{
			float I = dot(vec3(cam*normal), normalize(lightdir));
			if(I < 0) I = 0;
			return I;
		}

		void main()
		{
			gl_Position = proj * cam * obj * vertex;

			float I = lighting(light_dir);
			fragcolor = vertexcolor * I;
			fragcolor[3] = 1;
		})RAW";
	// --------------------------------------------------------------------


	// set glsl version
	std::string strGlsl = std::to_string(_GL_MAJ_VER*100 + _GL_MIN_VER*10);
	for(std::string* strSrc : { &strFragShader, &strVertexShader })
		algo::replace_all(*strSrc, std::string("${GLSL_VERSION}"), strGlsl);


	// GL functions
	auto *pGl = GetGlFunctions();
	if(!pGl) return;
	std::cerr << __func__ << ": "
		<< (char*)pGl->glGetString(GL_VERSION) << ", "
		<< (char*)pGl->glGetString(GL_VENDOR) << ", "
		<< (char*)pGl->glGetString(GL_RENDERER) << ", "
		<< "glsl: " << (char*)pGl->glGetString(GL_SHADING_LANGUAGE_VERSION)
		<< std::endl;
	LOGGLERR(pGl);


	// shaders
	{
		static QMutex shadermutex;
		shadermutex.lock();
		BOOST_SCOPE_EXIT(&shadermutex) { shadermutex.unlock(); } BOOST_SCOPE_EXIT_END

		// shader compiler/linker error handler
		auto shader_err = [this](const char* err) -> void
		{
			std::cerr << err << std::endl;

			std::string strLog = m_pShaders->log().toStdString();
			if(strLog.size())
				std::cerr << "Shader log: " << strLog << std::endl;

			std::exit(-1);
		};

		// compile & link shaders
		m_pShaders = std::make_shared<QOpenGLShaderProgram>(this);

		if(!m_pShaders->addShaderFromSourceCode(QOpenGLShader::Fragment, strFragShader.c_str()))
			shader_err("Cannot compile fragment shader.");
		if(!m_pShaders->addShaderFromSourceCode(QOpenGLShader::Vertex, strVertexShader.c_str()))
			shader_err("Cannot compile vertex shader.");

		if(!m_pShaders->link())
			shader_err("Cannot link shaders.");

		m_uniMatrixCam = m_pShaders->uniformLocation("cam");
		m_uniMatrixProj = m_pShaders->uniformLocation("proj");
		m_uniMatrixObj = m_pShaders->uniformLocation("obj");
		m_attrVertex = m_pShaders->attributeLocation("vertex");
		m_attrVertexNormal = m_pShaders->attributeLocation("normal");
		m_attrVertexColor = m_pShaders->attributeLocation("vertexcolor");
	}
	LOGGLERR(pGl);


	// geometries
	{
		GlPlotObj obj1 = CreateSphere(1., 0.,0.,0.,  0.,0.5,0.,1.);
		//GlPlotObj obj1 = CreateArrow(1., 1., 0.,0.,0.,  0.,0.5,0.,1.);
		//GlPlotObj obj1 = CreateCone(1., 1., 0.,0.,0.,  0.,0.5,0.,1.);
		GlPlotObj obj2 = CreateSphere(0.2, 0.,0.,2., 0.,0.,1.,1.);
		GlPlotObj obj3 = CreateCylinder(0.2, 0.5, 0.,0.,-2., 0.,0.,1.,1.);
		m_objs.emplace_back(std::move(obj1));
		m_objs.emplace_back(std::move(obj2));
		m_objs.emplace_back(std::move(obj3));
	}
	LOGGLERR(pGl);


	// options
	pGl->glCullFace(GL_BACK);
	pGl->glEnable(GL_CULL_FACE);
}


void GlPlot_impl::SetScreenDims(int w, int h)
{
	m_iScreenDims[0] = w;
	m_iScreenDims[1] = h;
}


void GlPlot_impl::resizeGL()
{
	const int w = m_iScreenDims[0];
	const int h = m_iScreenDims[1];

	auto *pContext = m_pPlot->context();
	if(!pContext) return;

	std::cerr << std::dec << __func__ << ": w = " << w << ", h = " << h << std::endl;
	m_matViewport = m::hom_viewport<t_mat_gl>(w, h, 0., 1.);
	std::tie(m_matViewport_inv, std::ignore) = m::inv<t_mat_gl>(m_matViewport);

	auto *pGl = GetGlFunctions();
	pGl->glViewport(0, 0, w, h);
	pGl->glDepthRange(0, 1);

	m_matPerspective = m::hom_perspective<t_mat_gl>(0.01, 100., m::pi<t_real_gl>*0.5, t_real_gl(h)/t_real_gl(w));
	std::tie(m_matPerspective_inv, std::ignore) = m::inv<t_mat_gl>(m_matPerspective);


	// bind shaders
	m_pShaders->bind();
	BOOST_SCOPE_EXIT(m_pShaders) { m_pShaders->release(); } BOOST_SCOPE_EXIT_END
	LOGGLERR(pGl);

	// set matrices
	m_pShaders->setUniformValue(m_uniMatrixCam, m_matCam);
	m_pShaders->setUniformValue(m_uniMatrixProj, m_matPerspective);
	LOGGLERR(pGl);

}


void GlPlot_impl::paintGL()
{
	auto *pContext = m_pPlot->context();
	if(!pContext) return;

	BOOST_SCOPE_EXIT(m_pPlot)
	{
// if the frame is not already updated by the timer, directly update it
#if _GL_USE_TIMER == 0
		QMetaObject::invokeMethod(m_pPlot, static_cast<void (QOpenGLWidget::*)()>(&QOpenGLWidget::update),
			Qt::ConnectionType::QueuedConnection);
#endif
	}
	BOOST_SCOPE_EXIT_END

	if(m_bPickerNeedsUpdate)
		updatePicker();


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

	// triangle geometry
	for(auto& obj : m_objs)
	{
		// main vertex array object
		pGl->glBindVertexArray(obj.m_vertexarr);

		m_pShaders->setUniformValue(m_uniMatrixObj, obj.m_mat);

		pGl->glEnableVertexAttribArray(m_attrVertex);
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

		pGl->glDrawArrays(GL_TRIANGLES, 0, obj.m_triangles.size());
		LOGGLERR(pGl);
	}
}


void GlPlot_impl::tick()
{
	tick(std::chrono::milliseconds(1000 / 60));
}

void GlPlot_impl::tick(const std::chrono::milliseconds& ms)
{
	// TODO
	updateCam();
}


void GlPlot_impl::updateCam()
{
	// zoom
	t_mat_gl matZoom = m::unit<t_mat_gl>();
	matZoom(0,0) = matZoom(1,1) = matZoom(2,2) = m_zoom;

	m_matCam = m::create<t_mat_gl>({1,0,0,0,  0,1,0,0,  0,0,1,-5,  0,0,0,1});
	m_matCam *= m_matCamRot;
	m_matCam *= matZoom;
	std::tie(m_matCam_inv, std::ignore) = m::inv<t_mat_gl>(m_matCam);

	m_bPickerNeedsUpdate = true;
	QMetaObject::invokeMethod(m_pPlot, static_cast<void (QOpenGLWidget::*)()>(&QOpenGLWidget::update),
		Qt::ConnectionType::QueuedConnection);
}


QPointF GlPlot_impl::GlToScreenCoords(const t_vec_gl& vec4, bool *pVisible)
{
	auto [ vecPersp, vec ] =
		m::hom_to_screen_coords<t_mat_gl, t_vec_gl>(vec4, m_matCam, m_matPerspective, m_matViewport, true);

	// position not visible -> return a point outside the viewport
	if(vecPersp[2] > 1.)
	{
		if(pVisible) *pVisible = false;
		return QPointF(-1*m_iScreenDims[0], -1*m_iScreenDims[1]);
	}

	if(pVisible) *pVisible = true;
	return QPointF(vec[0], vec[1]);
}


void GlPlot_impl::mouseMoveEvent(const QPointF& pos)
{
	m_posMouse = pos;

	if(m_bInRotation)
	{
		auto diff = (m_posMouse - m_posMouseRotationStart);
		t_real_gl phi = diff.x() + m_phi_saved;
		t_real_gl theta = diff.y() + m_theta_saved;

		m_matCamRot = m::rotation<t_mat_gl, t_vec_gl>(m::create<t_vec_gl>({1.,0.,0.,0.}), theta/180.*M_PI, 0);
		m_matCamRot *= m::rotation<t_mat_gl, t_vec_gl>(m::create<t_vec_gl>({0.,1.,0.,0.}), phi/180.*M_PI, 0);
	}

	updateCam();
}


void GlPlot_impl::zoom(t_real_gl val)
{
	m_zoom *= std::pow(2., val/64.);
	updateCam();
}

void GlPlot_impl::ResetZoom()
{
	m_zoom = 1;
	updateCam();
}


void GlPlot_impl::BeginRotation()
{
	if(!m_bInRotation)
	{
		m_posMouseRotationStart = m_posMouse;
		m_bInRotation = true;
	}
}

void GlPlot_impl::EndRotation()
{
	if(m_bInRotation)
	{
		auto diff = (m_posMouse - m_posMouseRotationStart);
		m_phi_saved += diff.x();
		m_theta_saved += diff.y();

		m_bInRotation = false;
	}
}


void GlPlot_impl::updatePicker()
{
	auto [org, dir] = m::hom_line_from_screen_coords<t_mat_gl, t_vec_gl>(
		m_posMouse.x(), m_posMouse.y(), 0., 1., m_matCam_inv,
		m_matPerspective_inv, m_matViewport_inv, &m_matViewport, true);

	// 3 vertices with rgba color
	t_real_gl red[] = {1.,0.,0.,1., 1.,0.,0.,1., 1.,0.,0.,1.};

	for(auto& obj : m_objs)
	{
		t_real_gl objcol[4*3];
		// TODO: only works if qvector4d stores its data linearly!
		std::copy(&obj.m_color[0], &obj.m_color[0]+4, objcol+0*4);
		std::copy(&obj.m_color[0], &obj.m_color[0]+4, objcol+1*4);
		std::copy(&obj.m_color[0], &obj.m_color[0]+4, objcol+2*4);

		obj.m_pcolorbuf->bind();
		BOOST_SCOPE_EXIT(&obj) { obj.m_pcolorbuf->release(); } BOOST_SCOPE_EXIT_END

		for(std::size_t startidx=0; startidx+2<obj.m_triangles.size(); startidx+=3)
		{
			std::vector<t_vec3_gl> poly{{
				obj.m_triangles[startidx+0],
				obj.m_triangles[startidx+1],
				obj.m_triangles[startidx+2] }};
			auto [vecInters, bInters, lamInters] =
				m::intersect_line_poly<t_vec3_gl, t_mat_gl>(
					t_vec3_gl(org[0], org[1], org[2]), t_vec3_gl(dir[0], dir[1], dir[2]),
					poly, obj.m_mat);

			if(bInters)
				obj.m_pcolorbuf->write(sizeof(red[0])*startidx*4, red, sizeof(red));
			else
				obj.m_pcolorbuf->write(sizeof(objcol[0])*startidx*4, objcol, sizeof(objcol));
		}
	}

	m_bPickerNeedsUpdate = false;
}



// ----------------------------------------------------------------------------
// GLPlot wrapper class

GlPlot::GlPlot(QWidget *pParent) : QOpenGLWidget(pParent),
	m_impl(std::make_unique<GlPlot_impl>(this))
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

// ----------------------------------------------------------------------------
