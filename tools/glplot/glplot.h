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

#include <QtCore/QThread>
#include <QtCore/QMutex>

#include "glplot_common.h"


class GlPlot_impl : public GlPlot_impl_base
{ Q_OBJECT
public:
	GlPlot_impl(GlPlot* pPlot);
	virtual ~GlPlot_impl();

public slots:
	void paintGL();

	void startedThread();
	void stoppedThread();
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
	virtual void mousePressEvent(QMouseEvent *Evt) override;
	virtual void mouseReleaseEvent(QMouseEvent *Evt) override;
	virtual void wheelEvent(QWheelEvent *pEvt) override;

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
