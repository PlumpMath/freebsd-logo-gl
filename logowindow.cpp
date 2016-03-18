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

#include "logowindow.h"

#include <QtGui/QMatrix4x4>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QScreen>

#include <QtCore/qmath.h>
#include <QTimer>

Renderer::Renderer(const QSurfaceFormat &format)
    : 
    m_initialized(false)
    , m_format(format)
    , m_program(0)
    , m_frame(0)
    , m_surface(0)
{
    m_context = new QOpenGLContext(this);
    m_context->setFormat(format);
    m_context->create();

    m_backgroundColor = QColor::fromRgbF(0.1f, 0.1f, 0.2f, 1.0f);
    m_detalizationLevel = 200;
}

void Renderer::setAnimating(LogoWindow *window, bool animating)
{
    if (animating) {
        m_surface = window;
        QTimer::singleShot(0, this, SLOT(render()));
    }
    else {
        m_surface = NULL;
    }
}

void Renderer::initialize()
{
    m_program = new QOpenGLShaderProgram(this);
    m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/vshader.glsl");
    m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/fshader.glsl");
    m_program->link();
    m_program->bind();

    m_texture = new QOpenGLTexture(QImage(":/pi.png"));

    m_texture->setMinificationFilter(QOpenGLTexture::Linear);
    m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
    m_texture->setWrapMode(QOpenGLTexture::ClampToBorder);

    vertexAttr = m_program->attributeLocation("vertex");
    normalAttr = m_program->attributeLocation("normal");
    texcoordAttr = m_program->attributeLocation("texcoord");
    matrixUniform = m_program->uniformLocation("matrix");
    colorUniform = m_program->uniformLocation("sourceColor");

    createGeometry();

    m_vbo.create();
    m_vbo.bind();
    const int verticesSize = vertices.count() * 3 * sizeof(GLfloat);
    const int textcoordsSize = vertices.count() * 2 * sizeof(GLfloat);
    m_vbo.allocate(verticesSize * 2 + textcoordsSize);
    m_vbo.write(0, vertices.constData(), verticesSize);
    m_vbo.write(verticesSize, normals.constData(), verticesSize);
    m_vbo.write(verticesSize*2, texcoords.constData(), textcoordsSize);
    m_vbo.release();

    m_program->release();
}

void Renderer::render()
{
    if (!m_surface)
        return;

    if (!m_context->makeCurrent(m_surface))
        return;

    if (!m_initialized) {
        initialize();
        m_initialized = true;
    }

    QSize viewSize = m_surface->size();

    QOpenGLFunctions *f = m_context->functions();
    f->glViewport(0, 0, viewSize.width() * m_surface->devicePixelRatio(), viewSize.height() * m_surface->devicePixelRatio());
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    f->glClearColor(m_backgroundColor.redF(), m_backgroundColor.greenF(), m_backgroundColor.blueF(), m_backgroundColor.alphaF());
    f->glEnable(GL_DEPTH_TEST);

    m_texture->bind();
    m_program->bind();
    m_vbo.bind();

    m_program->enableAttributeArray(vertexAttr);
    m_program->enableAttributeArray(normalAttr);
    m_program->enableAttributeArray(texcoordAttr);
    m_program->setAttributeBuffer(vertexAttr, GL_FLOAT, 0, 3);
    const int verticesSize = vertices.count() * 3 * sizeof(GLfloat);
    m_program->setAttributeBuffer(normalAttr, GL_FLOAT, verticesSize, 3);
    m_program->setAttributeBuffer(texcoordAttr, GL_FLOAT, verticesSize*2, 2);

    QMatrix4x4 modelview;
    modelview.rotate(90, -1.0f, 0.0f, 0.0f);
    modelview.rotate((float)m_frame, 0.0f, 0.0f, -1.0f);

    m_program->setUniformValue(matrixUniform, modelview);
    m_program->setUniformValue(colorUniform, QColor(200, 0, 0, 255));
    m_program->setUniformValue("texture", 0);

    f->glDrawArrays(GL_TRIANGLES, 0, vertices.size());

    m_context->swapBuffers(m_surface);

    ++m_frame;

    QTimer::singleShot(0, this, SLOT(render()));
}

void Renderer::createGeometry()
{
    vertices.clear();
    normals.clear();
    texcoords.clear();

    createSphere();
    createHorns();

    for (int i = 0;i < vertices.size();i++)
        vertices[i] *= 2.0f;
}

void Renderer::createSphere()
{
    const qreal r = 0.30;

    for (int i = 0; i < m_detalizationLevel; ++i) {
        qreal angle1 = i * 360. / m_detalizationLevel;
        qreal angle2 = (i + 1) * 360. / m_detalizationLevel;

        for (int j = 0; j < m_detalizationLevel/2; ++j) {
            qreal angle3 = j * 360. / m_detalizationLevel;
            qreal angle4 = (j + 1) * 360. / m_detalizationLevel;

            QVector3D p1 = fromSph(r, angle1, angle3);
            QVector3D p2 = fromSph(r, angle1, angle4);
            QVector3D p3 = fromSph(r, angle2, angle4);
            QVector3D p4 = fromSph(r, angle2, angle3);

            QVector2D t1;
            QVector2D t2;
            QVector2D t3;
            QVector2D t4;

            // we want texture to be on the square 60 degrees by 60 degrees
            int fromLevel = m_detalizationLevel / 6;
            int toLevel = 2 * m_detalizationLevel / 6;
            int totalLevels = toLevel - fromLevel;
            QVector2D origin(fromLevel, fromLevel);
            if ((i >= fromLevel) && (i < toLevel)
                    && (j >= fromLevel) && (j < toLevel)) {

                t1 = (QVector2D(i, j) - origin) / totalLevels;
                t2 = (QVector2D(i, j + 1) - origin) / totalLevels;
                t3 = (QVector2D(i + 1, j + 1) - origin) / totalLevels;
                t4 = (QVector2D(i + 1, j) - origin)  /totalLevels;

                // Flip x coordinate because direction we build 
                // sphere is right to left
                t1.setX(1 - t1.x());
                t2.setX(1 - t2.x());
                t3.setX(1 - t3.x());
                t4.setX(1 - t4.x());
            }

            QVector3D d1 = p1 - p2;
            QVector3D d2 = p3 - p2;
            d1.normalize();
            d2.normalize();
            QVector3D n = QVector3D::normal(d1, d2);

            vertices << p1;
            vertices << p2;
            vertices << p3;

            texcoords << t1;
            texcoords << t2;
            texcoords << t3;

            vertices << p3;
            vertices << p4;
            vertices << p1;

            texcoords << t3;
            texcoords << t4;
            texcoords << t1;

            normals << n;
            normals << n;
            normals << n;

            normals << n;
            normals << n;
            normals << n;
        }
    }
}

void Renderer::createHorns()
{
    QMatrix4x4 transform1;
    transform1.translate(-0.3, 0, 0.3);
    transform1.rotate(135, 0.0f, 1.0f, 0.0f);
    transform1.scale(0.3, 0.3, 0.3);

    createHorn(transform1);

    QMatrix4x4 transform2;
    transform2.translate(0.3, 0, 0.3);
    transform2.rotate(225, 0.0f, 1.0f, 0.0f);
    transform2.scale(0.3, 0.3, 0.3);

    createHorn(transform2);
}

void Renderer::createHorn(QMatrix4x4 transform)
{
    const qreal a = 7;

    for (int i = 0; i < m_detalizationLevel; ++i) {
        qreal angle1 = qDegreesToRadians((i * 360.0f) / m_detalizationLevel);
        qreal angle2 = qDegreesToRadians(((i + 1) * 360.0f) / m_detalizationLevel);

        qreal r = 0;
        const qreal r_step = 0.01;
        while ( r*r*a < 0.5) {
            qreal r1 = r;
            qreal r2 = r + r_step;
            r += r_step;

            QVector3D p1(r1*qSin(angle1), r1*qCos(angle1), r1*r1*a);
            QVector3D p2(r2*qSin(angle1), r2*qCos(angle1), r2*r2*a);
            QVector3D p3(r2*qSin(angle2), r2*qCos(angle2), r2*r2*a);
            QVector3D p4(r1*qSin(angle2), r1*qCos(angle2), r1*r1*a);

            p1 = transform.map(p1);
            p2 = transform.map(p2);
            p3 = transform.map(p3);
            p4 = transform.map(p4);

            QVector3D d1 = p1 - p2;
            QVector3D d2 = p3 - p1;
            d1.normalize();
            d2.normalize();
            QVector3D n = QVector3D::normal(d1, d2);

            vertices << p1;
            vertices << p2;
            vertices << p3;

            vertices << p3;
            vertices << p4;
            vertices << p1;

            normals << n;
            normals << n;
            normals << n;

            normals << n;
            normals << n;
            normals << n;

            // Trick: pint 0, 0 should be transparent in texture
            texcoords << QVector2D(0, 0);
            texcoords << QVector2D(0, 0);
            texcoords << QVector2D(0, 0);

            texcoords << QVector2D(0, 0);
            texcoords << QVector2D(0, 0);
            texcoords << QVector2D(0, 0);
        }
    }
}

QVector3D Renderer::fromSph(qreal r, qreal theta, qreal phi)
{
    theta = qDegreesToRadians(theta);
    phi = qDegreesToRadians(phi);

    return QVector3D(r*qCos(theta)*qSin(phi),
        r*qSin(theta)*qSin(phi), r*qCos(phi));
}

LogoWindow::LogoWindow(Renderer *renderer)
    : m_renderer(renderer)
{
    setSurfaceType(QWindow::OpenGLSurface);
    setFlags(Qt::Window | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);

    setGeometry(QRect(10, 10, 640, 640));

    setFormat(renderer->format());
    create();
}

void LogoWindow::exposeEvent(QExposeEvent *)
{
    m_renderer->setAnimating(this, isExposed());
}
