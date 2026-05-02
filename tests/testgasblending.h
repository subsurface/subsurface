// SPDX-License-Identifier: GPL-2.0
#ifndef TESTGASBLENDER_H
#define TESTGASBLENDER_H

#include "testbase.h"

class TestGasBlender : public TestBase {
    Q_OBJECT
private slots:
    void setup();
    void testCalculateCylinderVolumes();
    void testBlendStruct();
    void testGenerateExhaustiveBlends();
    
    // Engine Logic Tests
    void testBlendValidate_PhysicalLimits();
    void testSimplexMatrixGeneration_Constraints();
    void testRunSimplexSolver_Feasible();
    void testRunSimplexSolver_Infeasible();
    void testFindCheapestBlend_ExactVsBestEffort();
};

#endif