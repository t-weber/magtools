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

//
// !! DOES NOT YET WORK !!
//
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


GlPlot_impl::GlPlot_impl(GlPlot *pPlot) : m_pPlot(pPlot)
{
	connect(&m_timer, &QTimer::timeout, this, static_cast<void (GlPlot_impl::*)()>(&GlPlot_impl::tick));
	m_timer.start(std::chrono::milliseconds(1000 / 60));
}


GlPlot_impl::~GlPlot_impl()
{
	m_timer.stop();
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


void GlPlot_impl::initializeGL()
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
			gl_Position = proj * cam * vertex;

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
		m_attrVertex = m_pShaders->attributeLocation("vertex");
		m_attrVertexNormal = m_pShaders->attributeLocation("normal");
		m_attrVertexColor = m_pShaders->attributeLocation("vertexcolor");
	}
	LOGGLERR(pGl);

	// flatten vertex array into raw float array
	auto to_float_array = [](const std::vector<t_vec3_gl>& verts, int iRepeat=1, int iElems=3)
		-> std::vector<GLfloat>
	{
		std::vector<GLfloat> vecRet;
		vecRet.reserve(iRepeat*verts.size()*iElems);

		for(const t_vec3_gl& vert : verts)
			for(int i=0; i<iRepeat; ++i)
				for(int iElem=0; iElem<iElems; ++iElem)
					vecRet.push_back(vert[iElem]);
		return vecRet;
	};

	// geometries
	{
		auto solid = m::create_icosahedron<t_vec3_gl>(0.75);
		auto [verts, norms, uvs] =
			m::spherify<t_vec3_gl>(
				m::subdivide_triangles<t_vec3_gl>(
					m::create_triangles<t_vec3_gl>(solid), 2), 1.);
		m_lines = m::create_lines<t_vec3_gl>(std::get<0>(solid), std::get<1>(solid));

		// main vertex array object
		pGl->glGenVertexArrays(2, m_vertexarr.data());
		pGl->glBindVertexArray(m_vertexarr[0]);

		{	// vertices
			m_pvertexbuf = std::make_shared<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);

			m_pvertexbuf->create();
			m_pvertexbuf->bind();
			BOOST_SCOPE_EXIT(&m_pvertexbuf) { m_pvertexbuf->release(); } BOOST_SCOPE_EXIT_END

			auto vecVerts = to_float_array(verts);
			m_pvertexbuf->allocate(vecVerts.data(), vecVerts.size()*sizeof(typename decltype(vecVerts)::value_type));
			pGl->glVertexAttribPointer(m_attrVertex, 3, GL_FLOAT, 0, 0, (void*)(0*sizeof(typename decltype(vecVerts)::value_type)));
		}

		{	// normals
			m_pnormalsbuf = std::make_shared<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);

			m_pnormalsbuf->create();
			m_pnormalsbuf->bind();
			BOOST_SCOPE_EXIT(&m_pnormalsbuf)
			{ m_pnormalsbuf->release(); }
			BOOST_SCOPE_EXIT_END

			auto vecNorms = to_float_array(norms, 3);
			m_pnormalsbuf->allocate(vecNorms.data(), vecNorms.size()*sizeof(typename decltype(vecNorms)::value_type));
			pGl->glVertexAttribPointer(m_attrVertexNormal, 3, GL_FLOAT, 0, 0, (void*)(0*sizeof(typename decltype(vecNorms)::value_type)));
		}

		{	// colors
			m_pcolorbuf = std::make_shared<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);

			m_pcolorbuf->create();
			m_pcolorbuf->bind();
			BOOST_SCOPE_EXIT(&m_pcolorbuf)
			{ m_pcolorbuf->release(); }
			BOOST_SCOPE_EXIT_END

			std::vector<GLfloat> vecCols;
			vecCols.reserve(4*verts.size());
			for(std::size_t iVert=0; iVert<verts.size(); ++iVert)
			{
				vecCols.push_back(0); vecCols.push_back(0);
				vecCols.push_back(1); vecCols.push_back(1);
			}

			m_pcolorbuf->allocate(vecCols.data(), vecCols.size()*sizeof(typename decltype(vecCols)::value_type));
			pGl->glVertexAttribPointer(m_attrVertexColor, 4, GL_FLOAT, 0, 0, (void*)(0*sizeof(typename decltype(vecCols)::value_type)));
		}


		pGl->glBindVertexArray(m_vertexarr[1]);

		{	// lines
			m_plinebuf = std::make_shared<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);

			m_plinebuf->create();
			m_plinebuf->bind();
			BOOST_SCOPE_EXIT(&m_plinebuf)
			{ m_plinebuf->release(); }
			BOOST_SCOPE_EXIT_END

			auto vecVerts = to_float_array(m_lines);
			m_plinebuf->allocate(vecVerts.data(), vecVerts.size()*sizeof(typename decltype(vecVerts)::value_type));
			pGl->glVertexAttribPointer(m_attrVertex, 3, GL_FLOAT, 0, 0, (void*)(0*sizeof(typename decltype(vecVerts)::value_type)));
		}

		m_vertices = std::move(std::get<0>(solid));
		m_triangles = std::move(verts);
	}
	LOGGLERR(pGl);


	// options
	pGl->glCullFace(GL_BACK);
	pGl->glEnable(GL_CULL_FACE);

	m_bInitialised = true;
}


void GlPlot_impl::resizeGL()
{
	const int w = m_iScreenDims[0];
	const int h = m_iScreenDims[1];

	auto *pContext = m_pPlot->context();
	if(!pContext) return;

	QMetaObject::invokeMethod(m_pPlot, &GlPlot::MoveContextToThread, Qt::ConnectionType::BlockingQueuedConnection);
	m_pPlot->GetMutex()->lock();
	m_pPlot->makeCurrent();
	BOOST_SCOPE_EXIT(m_pPlot, pContext)
	{
		m_pPlot->doneCurrent();
		pContext->moveToThread(qGuiApp->thread());
		m_pPlot->GetMutex()->unlock();

		//QMetaObject::invokeMethod(m_pPlot, static_cast<void (QOpenGLWidget::*)()>(&QOpenGLWidget::update),
		//	Qt::ConnectionType::QueuedConnection);
	}
	BOOST_SCOPE_EXIT_END

	if(!m_bInitialised) initializeGL();


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
	if(!m_bRunning) return;
	auto *pContext = m_pPlot->context();
	if(!pContext) return;

	QMetaObject::invokeMethod(m_pPlot, &GlPlot::MoveContextToThread, Qt::ConnectionType::BlockingQueuedConnection);
	m_pPlot->GetMutex()->lock();
	m_pPlot->makeCurrent();
	BOOST_SCOPE_EXIT(m_pPlot, pContext)
	{
		m_pPlot->doneCurrent();
		pContext->moveToThread(qGuiApp->thread());
		m_pPlot->GetMutex()->unlock();

		QMetaObject::invokeMethod(m_pPlot, static_cast<void (QOpenGLWidget::*)()>(&QOpenGLWidget::update),
			Qt::ConnectionType::QueuedConnection);
	}
	BOOST_SCOPE_EXIT_END

	if(!m_bInitialised) initializeGL();


	auto *pGl = GetGlFunctions();
	//QPainter painter(m_pPlot);


	// gl painting
	{
		/*painter.beginNativePainting();
		BOOST_SCOPE_EXIT(&painter) { painter.endNativePainting(); } BOOST_SCOPE_EXIT_END*/

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
		if(m_pvertexbuf)
		{
			// main vertex array object
			pGl->glBindVertexArray(m_vertexarr[0]);

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

			pGl->glDrawArrays(GL_TRIANGLES, 0, m_triangles.size());
			LOGGLERR(pGl);
		}

		// lines
		if(m_plinebuf)
		{
			// auxiliary vertex array object
			pGl->glBindVertexArray(m_vertexarr[1]);

			pGl->glEnableVertexAttribArray(m_attrVertex);
			BOOST_SCOPE_EXIT(pGl, &m_attrVertex)
			{ pGl->glDisableVertexAttribArray(m_attrVertex); }
			BOOST_SCOPE_EXIT_END
			LOGGLERR(pGl);

			pGl->glDrawArrays(GL_LINES, 0, m_lines.size());
			LOGGLERR(pGl);
		}
	}

	// classic painting
	/*{
		pGl->glDisable(GL_DEPTH_TEST);

		std::size_t i = 0;
		for(const auto& vert : m_vertices)
		{
			std::string strName = "* " + std::to_string(i);
			painter.drawText(GlToScreenCoords(t_vec_gl(vert[0], vert[1], vert[2], 1)), strName.c_str());
			++i;
		}
	}*/

	//std::cout << "paintGL" << std::endl;
}


void GlPlot_impl::tick()
{
	tick(std::chrono::milliseconds(1000 / 60));
}

void GlPlot_impl::tick(const std::chrono::milliseconds& ms)
{
	static t_real_gl fAngle = 0.f;
	fAngle += 0.5f;

	m_matCam = m::create<t_mat_gl>({1,0,0,0,  0,1,0,0,  0,0,1,-3,  0,0,0,1});
	m_matCam *= m::rotation<t_mat_gl, t_vec_gl>(m::create<t_vec_gl>({1.,1.,0.,0.}), fAngle/180.*M_PI, 0);
	std::tie(m_matCam_inv, std::ignore) = m::inv<t_mat_gl>(m_matCam);

	updatePicker();
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
	//std::cerr << "(" << pos.x() << ", " << pos.y() << ")" << std::endl;
	m_posMouse = pos;
	updatePicker();
}


void GlPlot_impl::updatePicker()
{
	if(!m_bInitialised || !m_bRunning || !m_pcolorbuf) return;

	m_pcolorbuf->bind();
	BOOST_SCOPE_EXIT(&m_pcolorbuf) { m_pcolorbuf->release(); } BOOST_SCOPE_EXIT_END
	LOGGLERR(GetGlFunctions());

	auto [org, dir] = m::hom_line_from_screen_coords<t_mat_gl, t_vec_gl>(m_posMouse.x(), m_posMouse.y(), 0., 1.,
		m_matCam_inv, m_matPerspective_inv, m_matViewport_inv, &m_matViewport, true);

	GLfloat red[] = {1.,0.,0.,1., 1.,0.,0.,1., 1.,0.,0.,1.};
	GLfloat blue[] = {0.,0.,1.,1., 0.,0.,1.,1., 0.,0.,1.,1.};

	for(std::size_t startidx=0; startidx+2<m_triangles.size(); startidx+=3)
	{
		std::vector<t_vec3_gl> poly{{ m_triangles[startidx+0], m_triangles[startidx+1], m_triangles[startidx+2] }};
		auto [vecInters, bInters, lamInters] =
			m::intersect_line_poly<t_vec3_gl>(
				t_vec3_gl(org[0], org[1], org[2]), t_vec3_gl(dir[0], dir[1], dir[2]), poly);
		//if(bInters)
		//	std::cout << "Intersection with polygon " << startidx/3 << std::endl;

		if(bInters)
			m_pcolorbuf->write(sizeof(red[0])*startidx*4, red, sizeof(red));
		else
			m_pcolorbuf->write(sizeof(blue[0])*startidx*4, blue, sizeof(blue));
	}
}

void GlPlot_impl::SetScreenDims(int w, int h)
{
	m_iScreenDims[0] = w;
	m_iScreenDims[1] = h;
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

	m_thread_impl->start();
	setMouseTracking(true);
	m_impl->SetRunning(true);
}

GlPlot::~GlPlot()
{
	m_impl->SetRunning(false);
	m_thread_impl->quit();
	m_thread_impl->wait();
}


void GlPlot::mouseMoveEvent(QMouseEvent *pEvt)
{
	m_impl->mouseMoveEvent(pEvt->localPos());
}


void GlPlot::MoveContextToThread()
{
	auto *pContext = context();
	if(pContext)
		pContext->moveToThread(m_thread_impl.get());
}


void GlPlot::paintEvent(QPaintEvent *pEvt)
{
	// empty for threaded painting
	//QOpenGLWidget::paintEvent(pEvt);
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

	QMetaObject::invokeMethod(m_impl.get(), &GlPlot_impl::resizeGL, Qt::ConnectionType::QueuedConnection);
}

// ----------------------------------------------------------------------------
