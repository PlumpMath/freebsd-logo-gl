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
#include <QEasingCurve>

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


void Renderer::createVbo(QOpenGLBuffer &vbo, const QVector<QVector3D> &vertices, const QVector<QVector3D> &normals, const QVector<QVector2D> &texcoords)
{
    vbo.create();
    vbo.bind();
    const int verticesSize = vertices.count() * 3 * sizeof(GLfloat);
    const int textcoordsSize = vertices.count() * 2 * sizeof(GLfloat);
    vbo.allocate(verticesSize * 2 + textcoordsSize);
    vbo.write(0, vertices.constData(), verticesSize);
    vbo.write(verticesSize, normals.constData(), verticesSize);
    vbo.write(verticesSize*2, texcoords.constData(), textcoordsSize);
    vbo.release();
}

void Renderer::initialize()
{
    m_program = new QOpenGLShaderProgram(this);
    m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/vshader.glsl");
    m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/fshader.glsl");
    m_program->link();
    m_program->bind();

    m_piTexture = new QOpenGLTexture(QImage(":/pi.png"));

    m_piTexture->setMinificationFilter(QOpenGLTexture::Linear);
    m_piTexture->setMagnificationFilter(QOpenGLTexture::Linear);
    m_piTexture->setWrapMode(QOpenGLTexture::ClampToBorder);

    m_qtTexture = new QOpenGLTexture(QImage(":/qt.png").mirrored());

    m_qtTexture->setMinificationFilter(QOpenGLTexture::Linear);
    m_qtTexture->setMagnificationFilter(QOpenGLTexture::Linear);
    m_qtTexture->setWrapMode(QOpenGLTexture::ClampToBorder);

    m_vertexAttr = m_program->attributeLocation("vertex");
    m_normalAttr = m_program->attributeLocation("normal");
    m_texcoordAttr = m_program->attributeLocation("texcoord");
    m_matrixUniform = m_program->uniformLocation("matrix");
    m_colorUniform = m_program->uniformLocation("sourceColor");
    m_textureUniform = m_program->uniformLocation("texture");
    m_alphaUniform = m_program->uniformLocation("alpha");

    createGeometry();
    createVbo(m_bsdVbo, m_bsdVertices, m_bsdNormals, m_bsdTexcoords);
    createVbo(m_qtVbo, m_qtVertices, m_qtNormals, m_qtTexcoords);

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
    f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    f->glEnable( GL_BLEND );

    m_program->bind();

    m_program->enableAttributeArray(m_vertexAttr);
    m_program->enableAttributeArray(m_normalAttr);
    m_program->enableAttributeArray(m_texcoordAttr);

    QMatrix4x4 modelview;
    modelview.rotate(90, -1.0f, 0.0f, 0.0f);
    modelview.rotate((float)m_frame, 0.0f, 0.0f, -1.0f);
    m_program->setUniformValue(m_matrixUniform, modelview);
    m_program->setUniformValue(m_colorUniform, QColor(200, 0, 0, 255));

    m_piTexture->bind();
    m_bsdVbo.bind();
    m_program->setUniformValue(m_textureUniform, 0);
    m_program->setUniformValue(m_alphaUniform, 1.0f);
    int verticesSize = m_bsdVertices.count() * 3 * sizeof(GLfloat);
    m_program->setAttributeBuffer(m_vertexAttr, GL_FLOAT, 0, 3);
    m_program->setAttributeBuffer(m_normalAttr, GL_FLOAT, verticesSize, 3);
    m_program->setAttributeBuffer(m_texcoordAttr, GL_FLOAT, verticesSize*2, 2);
    f->glDrawArrays(GL_TRIANGLES, 0, m_bsdVertices.size());

    m_qtTexture->bind();
    m_qtVbo.bind();
    modelview.setToIdentity();
    modelview.translate(0.7, -0.7, 0);
    m_program->setUniformValue(m_matrixUniform, modelview);
    m_program->setUniformValue(m_colorUniform, QColor(0, 0, 0, 0));
    m_program->setUniformValue(m_textureUniform, 0);
    // blink Qt logo 3 times during last 60 degrees of turn
    const int flickerPart = 60;
    const int period = 457;
    int degree = m_frame % period;
    GLfloat qtAlpha = 1.0;
    QEasingCurve easing(QEasingCurve::OutInBounce);

    if (degree < flickerPart) {
        qtAlpha = easing.valueForProgress(degree/(float)flickerPart);
    }

    if (degree >= period - flickerPart) {
        qtAlpha = easing.valueForProgress((degree + flickerPart - period)/(float)flickerPart);
        qtAlpha = 1 - qtAlpha;
    }

    if (qtAlpha > 1)
        qtAlpha = 1;
    if (qtAlpha < 0)
        qtAlpha = 0;

    m_program->setUniformValue(m_alphaUniform, qtAlpha);
    verticesSize = m_qtVertices.count() * 3 * sizeof(GLfloat);
    m_program->setAttributeBuffer(m_vertexAttr, GL_FLOAT, 0, 3);
    m_program->setAttributeBuffer(m_normalAttr, GL_FLOAT, verticesSize, 3);
    m_program->setAttributeBuffer(m_texcoordAttr, GL_FLOAT, verticesSize*2, 2);
    f->glDrawArrays(GL_TRIANGLES, 0, m_qtVertices.size());

    m_context->swapBuffers(m_surface);

    ++m_frame;

    QTimer::singleShot(0, this, SLOT(render()));
}

void Renderer::createGeometry()
{
    /* Sphere */
    m_bsdVertices.clear();
    m_bsdNormals.clear();
    m_bsdTexcoords.clear();
    createSphere();
    createHorns();
    for (int i = 0;i < m_bsdVertices.size();i++)
        m_bsdVertices[i] *= 2.0f;

    /* Qt logo square */
    m_qtVertices.clear();
    m_qtNormals.clear();
    m_qtTexcoords.clear();
    createSquare();
}

void Renderer::createSquare()
{
    const qreal f = 0.15;
    QVector3D p1(-f, -f, -1);
    QVector3D p2(-f, f, -1);
    QVector3D p3(f, f, -1);
    QVector3D p4(f, -f, -1);

    QVector3D n = normalForTriangle(p1, p2, p3);

    m_qtVertices << p1;
    m_qtVertices << p2;
    m_qtVertices << p3;

    m_qtVertices << p3;
    m_qtVertices << p4;
    m_qtVertices << p1;

    m_qtNormals << n;
    m_qtNormals << n;
    m_qtNormals << n;

    m_qtNormals << n;
    m_qtNormals << n;
    m_qtNormals << n;

    m_qtTexcoords << QVector2D(0, 0);
    m_qtTexcoords << QVector2D(0, 1);
    m_qtTexcoords << QVector2D(1, 1);

    m_qtTexcoords << QVector2D(1, 1);
    m_qtTexcoords << QVector2D(1, 0);
    m_qtTexcoords << QVector2D(0, 0);
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

            QVector3D n = normalForTriangle(p1, p2, p3);

            m_bsdVertices << p1;
            m_bsdVertices << p2;
            m_bsdVertices << p3;

            m_bsdTexcoords << t1;
            m_bsdTexcoords << t2;
            m_bsdTexcoords << t3;

            m_bsdVertices << p3;
            m_bsdVertices << p4;
            m_bsdVertices << p1;

            m_bsdTexcoords << t3;
            m_bsdTexcoords << t4;
            m_bsdTexcoords << t1;

            m_bsdNormals << n;
            m_bsdNormals << n;
            m_bsdNormals << n;

            m_bsdNormals << n;
            m_bsdNormals << n;
            m_bsdNormals << n;
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

            QVector3D n = normalForTriangle(p1, p2, p3);

            m_bsdVertices << p1;
            m_bsdVertices << p2;
            m_bsdVertices << p3;

            m_bsdVertices << p3;
            m_bsdVertices << p4;
            m_bsdVertices << p1;

            m_bsdNormals << n;
            m_bsdNormals << n;
            m_bsdNormals << n;

            m_bsdNormals << n;
            m_bsdNormals << n;
            m_bsdNormals << n;

            // Trick: pint 0, 0 should be transparent in texture
            m_bsdTexcoords << QVector2D(0, 0);
            m_bsdTexcoords << QVector2D(0, 0);
            m_bsdTexcoords << QVector2D(0, 0);

            m_bsdTexcoords << QVector2D(0, 0);
            m_bsdTexcoords << QVector2D(0, 0);
            m_bsdTexcoords << QVector2D(0, 0);
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
