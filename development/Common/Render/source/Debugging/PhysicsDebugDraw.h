/////////////////////////////////////////////////////////////////////////
// PhysicsDebugDraw.h
//
//  Copyright 7/30/2016 Edgar Glowacki.
//
// NOTES:
//      Render debug information for physics system
//
//
// #include "Debug/PhysicsDebugDraw.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#ifndef YAGET_PHYSISCS_DEBUG_DRAW_H
#define YAGET_PHYSISCS_DEBUG_DRAW_H
#pragma once

#include "LinearMath/btIDebugDraw.h"
#include <Components/LineComponent.h>

namespace yaget
{
    namespace render
    {
        class PhysicsDebugDraw : public btIDebugDraw
        {
        public:
            PhysicsDebugDraw(Device& device, LineComponent& lineComponent)
                : mDevice(device)
                , mLineComponent(lineComponent)
                , mLines(std::make_shared<Lines_t>())
                , mCurrentLineColor(-1, -1, -1)
                , mDebugMode(btIDebugDraw::DBG_DrawWireframe | btIDebugDraw::DBG_DrawAabb)
            {}

            virtual DefaultColors getDefaultColors() const override
            {
                return mOurColors;
            }
            ///the default implementation for setDefaultColors has no effect. A derived class can implement it and store the colors.
            virtual void setDefaultColors(const DefaultColors& colors)
            {
                mOurColors = colors;
            }


            virtual void drawLine(const btVector3& from1, const btVector3& to1, const btVector3& color1) override
            {
                Line line;
                line.p0.x = from1.x();
                line.p0.y = from1.y();
                line.p0.z = from1.z();

                line.p1.x = to1.x();
                line.p1.y = to1.y();
                line.p1.z = to1.z();

                line.c0.x = color1.x();
                line.c0.y = color1.y();
                line.c0.z = color1.z();
                line.c1 = line.c0;
                mLines->emplace_back(line);
            }

            virtual void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar /*distance*/, int /*lifeTime*/, const btVector3& color) override
            {
                drawLine(PointOnB, PointOnB + normalOnB, color);
            }


            virtual void reportErrorWarning(const char* /*warningString*/) override
            {}

            virtual void draw3dText(const btVector3& /*location*/, const char* /*textString*/) override
            {}

            virtual void setDebugMode(int debugMode) override
            {
                mDebugMode = debugMode;
            }

            virtual int getDebugMode() const override
            {
                return mDebugMode;
            }

            virtual void flushLines() override
            {
                if (mLines)
                {
                    mLineComponent.Draw(mLines);
                }
                mLines = std::make_shared<Lines_t>();

                // farm this to next step (another thread,) and in this example, rander should pick it up
                // the lock pointe,r shpoiuld be only read form render, but form here replace when needed,
                // just like text
                //int sz = m_linePoints.size();
                //if (sz)
                //{
                //    float debugColor[4];
                //    debugColor[0] = m_currentLineColor.x();
                //    debugColor[1] = m_currentLineColor.y();
                //    debugColor[2] = m_currentLineColor.z();
                //    debugColor[3] = 1.f;
                //    m_glApp->m_renderer->drawLines(&m_linePoints[0].x, debugColor,
                //        m_linePoints.size(), sizeof(MyDebugVec3),
                //        &m_lineIndices[0],
                //        m_lineIndices.size(),
                //        1);
                //    m_linePoints.clear();
                //    m_lineIndices.clear();
                //}
            }
        private:
            Device& mDevice;
            LineComponent& mLineComponent;
            int mDebugMode;

            //btAlignedObjectArray<MyDebugVec3> m_linePoints;
            //btAlignedObjectArray<unsigned int> m_lineIndices;
            btVector3 mCurrentLineColor;
            DefaultColors mOurColors;

            typedef LineComponent::Line Line;
            typedef LineComponent::Lines_t Lines_t;
            typedef LineComponent::LinesPtr_t LinesPtr_t;
            LinesPtr_t mLines;
        };

    } // namespace render
} // namespace yaget

#endif // YAGET_PHYSISCS_DEBUG_DRAW_H