/*-
 * Copyright (c) 2012 Oleksandr Tymoshenko <gonzo@bluezbox.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <QtGui/QMatrix4x4>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLBuffer>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOpenGLTexture>
#include <QtGui/QWindow>

#include <QtCore/qmath.h>

class LogoWindow;

class Renderer : public QObject
{
    Q_OBJECT

public:
    explicit Renderer(const QSurfaceFormat &format);
    QSurfaceFormat format() const { return m_format; }

    void setAnimating(LogoWindow *window, bool animating);

private slots:
    void render();

private:
    void initialize();

    void createGeometry();
    void createSphere();
    void createHorns();
    void createHorn(QMatrix4x4 transform);

    void createVbo(QOpenGLBuffer &vbo, const QVector<QVector3D> &vertices, const QVector<QVector3D> &normals, const QVector<QVector2D> &texcoords);

    QVector3D fromSph(qreal r, qreal theta, qreal phi);

    QOpenGLBuffer m_bsdVbo;
    QVector<QVector3D> m_bsdVertices;
    QVector<QVector3D> m_bsdNormals;
    QVector<QVector2D> m_bsdTexcoords;

    QOpenGLBuffer m_qtVbo;
    QVector<QVector3D> m_qtVertices;
    QVector<QVector3D> m_qtNormals;
    QVector<QVector2D> m_qtTexcoords;

    int vertexAttr;
    int normalAttr;
    int texcoordAttr;
    int matrixUniform;
    int colorUniform;

    bool m_initialized;
    QSurfaceFormat m_format;
    QOpenGLContext *m_context;
    QOpenGLShaderProgram *m_program;
    int m_frame;
    LogoWindow *m_surface;
    QColor m_backgroundColor;
    int m_detalizationLevel;

    QOpenGLTexture *m_piTexture;
    QOpenGLTexture *m_qtTexture;
};

class LogoWindow : public QWindow
{
public:
    LogoWindow(Renderer *renderer);
    void exposeEvent(QExposeEvent *event);

private:
    Renderer *m_renderer;
};
