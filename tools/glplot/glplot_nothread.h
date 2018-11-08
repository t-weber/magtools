/**
 * GL plotter
 * @author Tobias Weber
 * @date Nov-2017 -- 2018
 * @license: see 'LICENSE.GPL' file
 *
 *  * References:
 *  * http://doc.qt.io/qt-5/qopenglwidget.html#details
 *  * http://code.qt.io/cgit/qt/qtbase.git/tree/examples/opengl/threadedqopenglwidget
 */

#ifndef __MAG_GL_PLOT_H__
#define __MAG_GL_PLOT_H__

#include "glplot_common.h"


class GlPlot_impl : public GlPlot_impl_base
{ Q_OBJECT
public:
	GlPlot_impl(GlPlot* pPlot);
	virtual ~GlPlot_impl();

public /*slots*/:
	void paintGL();
};


class GlPlot : public QOpenGLWidget
{ Q_OBJECT
public:
	using QOpenGLWidget::QOpenGLWidget;

	GlPlot(QWidget *pParent);
	virtual ~GlPlot();

public:
	GlPlot_impl* GetImpl() { return m_impl.get(); }

protected:
	virtual void paintGL() override;
	virtual void initializeGL() override;
	virtual void resizeGL(int w, int h) override;

	virtual void mouseMoveEvent(QMouseEvent *pEvt) override;
	virtual void mousePressEvent(QMouseEvent *Evt) override;
	virtual void mouseReleaseEvent(QMouseEvent *Evt) override;
	virtual void wheelEvent(QWheelEvent *pEvt) override;

private:
	std::unique_ptr<GlPlot_impl> m_impl;

signals:
	void AfterGLInitialisation();
	void MouseDown(bool left, bool mid, bool right);
	void MouseUp(bool left, bool mid, bool right);
};


#endif
