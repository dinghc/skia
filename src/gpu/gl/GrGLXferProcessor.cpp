/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gl/GrGLXferProcessor.h"

#include "GrXferProcessor.h"
#include "gl/builders/GrGLFragmentShaderBuilder.h"
#include "gl/builders/GrGLProgramBuilder.h"

void GrGLXferProcessor::emitCode(const EmitArgs& args) {
    if (args.fXP.getDstTexture()) {
        bool topDown = kTopLeft_GrSurfaceOrigin == args.fXP.getDstTexture()->origin();

        GrGLXPFragmentBuilder* fsBuilder = args.fPB->getFragmentShaderBuilder();

        if (args.fXP.readsCoverage()) {
            // We don't think any shaders actually output negative coverage, but just as a safety
            // check for floating point precision errors we compare with <= here
            fsBuilder->codeAppendf("if (all(lessThanEqual(%s, vec4(0)))) {"
                                   "    discard;"
                                   "}", args.fInputCoverage);
        }

        const char* dstColor = fsBuilder->dstColor();

        const char* dstTopLeftName;
        const char* dstCoordScaleName;

        fDstTopLeftUni = args.fPB->addUniform(GrGLProgramBuilder::kFragment_Visibility,
                                              kVec2f_GrSLType,
                                              kDefault_GrSLPrecision,
                                              "DstTextureUpperLeft",
                                              &dstTopLeftName);
        fDstScaleUni = args.fPB->addUniform(GrGLProgramBuilder::kFragment_Visibility,
                                            kVec2f_GrSLType,
                                            kDefault_GrSLPrecision,
                                            "DstTextureCoordScale",
                                            &dstCoordScaleName);
        const char* fragPos = fsBuilder->fragmentPosition();

        fsBuilder->codeAppend("// Read color from copy of the destination.\n");
        fsBuilder->codeAppendf("vec2 _dstTexCoord = (%s.xy - %s) * %s;",
                               fragPos, dstTopLeftName, dstCoordScaleName);

        if (!topDown) {
            fsBuilder->codeAppend("_dstTexCoord.y = 1.0 - _dstTexCoord.y;");
        }

        fsBuilder->codeAppendf("vec4 %s = ", dstColor);
        fsBuilder->appendTextureLookup(args.fSamplers[0], "_dstTexCoord", kVec2f_GrSLType);
        fsBuilder->codeAppend(";");
    }

    this->onEmitCode(args);
}

void GrGLXferProcessor::setData(const GrGLProgramDataManager& pdm, const GrXferProcessor& xp) {
    if (xp.getDstTexture()) {
        if (fDstTopLeftUni.isValid()) {
            pdm.set2f(fDstTopLeftUni, static_cast<GrGLfloat>(xp.dstTextureOffset().fX),
                      static_cast<GrGLfloat>(xp.dstTextureOffset().fY));
            pdm.set2f(fDstScaleUni, 1.f / xp.getDstTexture()->width(),
                      1.f / xp.getDstTexture()->height());
        } else {
            SkASSERT(!fDstScaleUni.isValid());
        }
    } else {
        SkASSERT(!fDstTopLeftUni.isValid());
        SkASSERT(!fDstScaleUni.isValid());
    }
    this->onSetData(pdm, xp);
}

