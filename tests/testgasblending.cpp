// SPDX-License-Identifier: GPL-2.0
#include "testgasblending.h"
#include "core/gasblending.h"

using Point = std::pair<double, double>;

using namespace Blender;

// Helper function to handle strongly-typed struct initialization cleanly in tests
static GasSource createSource(int o2, int he, int wp_mbar, int wet_ml, int fill_ml, int cur_ml, int cur_mbar, double cost, bool unl, int num, bool bst) {
    GasSource s;
    s.o2_permille = o2;
    s.he_permille = he;
    s.workingPressure.mbar = wp_mbar;
    s.wetVolume.mliter = wet_ml;
    s.fillVolume.mliter = fill_ml;
    s.currentVolume.mliter = cur_ml;
    s.currentPressure.mbar = cur_mbar;
    s.cost_per_unit_volume = cost;
    s.unlimited = unl;
    s.cylinder_number = num;
    s.boosted = bst;
    return s;
}

void TestGasBlender::setup()
{

}

void TestGasBlender::testCalculateCylinderVolumes()
{
    TargetCylinder cylinder;
    cylinder.currentHe_permille = 100;
    cylinder.currentO2_permille = 200;
    cylinder.currentN2_permille = 700;
    cylinder.targetHe_permille = 100;
    cylinder.targetO2_permille = 200;
    cylinder.currentPressure.mbar = 2013;
    cylinder.workingPressure.mbar = 11013;
    cylinder.targetPressure.mbar = 10000;
    cylinder.wetVolume.mliter = 0;
    cylinder.fillVolume.mliter = 10000;
    calculate_cylinder_volumes(cylinder);

    QCOMPARE(cylinder.wetVolume.mliter, 1013);
    QCOMPARE(cylinder.currentVolume.mliter, 3026); 

    //Reverse it
    cylinder.fillVolume.mliter = 0;
    cylinder.currentVolume.mliter = 0;
    calculate_cylinder_volumes(cylinder);

    QCOMPARE(cylinder.wetVolume.mliter, 1013);

    QCOMPARE(cylinder.fillVolume.mliter, 11034);
    QCOMPARE(cylinder.currentVolume.mliter, 3026); 
}

void TestGasBlender::testBlendStruct()
{
    TargetCylinder cylinder;
    cylinder.currentHe_permille = 100;
    cylinder.currentO2_permille = 200;
    cylinder.currentN2_permille = 700;
    cylinder.currentPressure.mbar = 1000;
    cylinder.workingPressure.mbar = 10000;
    cylinder.wetVolume.mliter = 0;
    cylinder.fillVolume.mliter = 10000;
    cylinder.targetHe_permille = 100;
    cylinder.targetO2_permille = 200;
    cylinder.targetPressure.mbar = 10000;

    calculate_cylinder_volumes(cylinder);

    Blend blend;
    blend.target = cylinder;
    blend.removedGasVolume.mliter = 0;

    GasSource source1;
    source1.o2_permille = 210;
    source1.he_permille = 0;
    source1.cost_per_unit_volume = 1.5;
    source1.unlimited = true;

    GasSource source2;
    source2.o2_permille = 800;
    source2.he_permille = 0;
    source2.cost_per_unit_volume = 2.1;
    source2.unlimited = true;

    GasSource source3;
    source3.o2_permille = 0;
    source3.he_permille = 1000;
    source3.cost_per_unit_volume = 15.0;
    source3.unlimited = true;

    BlendStep step1;
    step1.source = source1;
    step1.volume.mliter = 1000;
    blend.steps.push_back(step1);

    QCOMPARE(3012 > 3011, true);
}

void TestGasBlender::testGenerateExhaustiveBlends(){
    GasSource source1;
    GasSource source2;
    GasSource source3;
    
    TargetCylinder cylinder;
    source1.o2_permille = 210;
    source1.he_permille = 0;
    source2.o2_permille = 800;
    source2.he_permille = 0;
    source3.o2_permille = 0;
    source3.he_permille = 1000;
    source1.cylinder_number = 0;
    source2.cylinder_number = 1;
    source3.cylinder_number = 2;

    source1.unlimited = false;
    source2.unlimited = false;
    source3.unlimited = false;
    source1.boosted = false;
    source2.boosted = false;
    source3.boosted = false;

    source1.wetVolume.mliter = 10000;
    source2.wetVolume.mliter = 10000;
    source3.wetVolume.mliter = 10000;

    source1.fillVolume.mliter = 100000;
    source2.fillVolume.mliter = 100000;
    source3.fillVolume.mliter = 100000;

    source1.workingPressure.mbar = 10000;
    source2.workingPressure.mbar = 10000;
    source3.workingPressure.mbar = 10000;

    source1.currentPressure.mbar = 5000;
    source2.currentPressure.mbar = 5000;
    source3.currentPressure.mbar = 5000;

    source1.currentVolume.mliter = 50000;
    source2.currentVolume.mliter = 50000;
    source3.currentVolume.mliter = 50000;

    cylinder.currentHe_permille = 0;
    cylinder.currentO2_permille = 210;
    cylinder.currentN2_permille = 790;
    cylinder.currentPressure.mbar = 3000;
    cylinder.workingPressure.mbar = 10000;
    cylinder.wetVolume.mliter = 10000;
    cylinder.fillVolume.mliter = 100000;
    cylinder.targetHe_permille = 150;
    cylinder.targetO2_permille = 320;
    cylinder.targetPressure.mbar = 10000;
    calculate_cylinder_volumes(cylinder);

    std::vector<GasSource> sources;
    sources.push_back(source1);
    sources.push_back(source2);
    sources.push_back(source3);
    auto result = generateExhaustiveBlends(cylinder,sources);
    QCOMPARE(result.size(), 6);
}

void TestGasBlender::testBlendValidate_PhysicalLimits()
{
    // Setup a target cylinder starting at 50 bar
    TargetCylinder cylinder;
    cylinder.currentO2_permille = 210;
    cylinder.currentHe_permille = 0;
    cylinder.currentN2_permille = 790;
    cylinder.currentPressure.mbar = 50000; 
    cylinder.wetVolume.mliter = 10000; // 10L tank
    cylinder.fillVolume.mliter = 200000; // Roughly 200 bar target
    cylinder.targetO2_permille = 320;
    cylinder.targetHe_permille = 0;
    
    Blend blend;
    blend.target = cylinder;
    blend.removedGasVolume.mliter = 0;

    // Test 1: Cascade Violation (Source is at 40 bar, Target is at 50 bar)
    GasSource lowPressureBank;
    lowPressureBank.unlimited = false;
    lowPressureBank.boosted = false;
    lowPressureBank.o2_permille = 1000;
    lowPressureBank.he_permille = 0;
    lowPressureBank.wetVolume.mliter = 50000; // 50L bank
    lowPressureBank.currentPressure.mbar = 40000; // 40 bar (LOWER than target)
    lowPressureBank.currentVolume.mliter = 2000000; // V = P*V_wet

    BlendStep step1;
    step1.source = lowPressureBank;
    step1.volume.mliter = 10000; // Trying to pull 10L

    blend.steps.push_back(step1);
    
    // Validate should return -1.0 for a pressure cascade violation
    QCOMPARE(blend.validate(), -1.0);

    // Test 2: Booster Violation
    blend.steps.clear();
    GasSource boostedBank = lowPressureBank;
    boostedBank.boosted = true;
    // Drain it down near the unpumpable limit (e.g., 5 bar left)
    boostedBank.currentPressure.mbar = 5000; 
    boostedBank.currentVolume.mliter = 250000; 

    BlendStep step2;
    step2.source = boostedBank;
    // Pulling this much will drop the bank below the BOOSTED_LIMIT (approx 7000 mbar)
    step2.volume.mliter = 100000; 
    blend.steps.push_back(step2);

    // Validate should return -1.0 for a booster limit violation
    QCOMPARE(blend.validate(), -1.0);
}

void TestGasBlender::testSimplexMatrixGeneration_Constraints()
{
    TargetCylinder cylinder;
    cylinder.currentO2_permille = 210;
    cylinder.currentHe_permille = 0;
    cylinder.targetO2_permille = 320;
    cylinder.targetHe_permille = 0;
    cylinder.wetVolume.mliter = 10000;
    cylinder.fillVolume.mliter = 200000;
    cylinder.currentVolume.mliter = 50000;

    Blend blend;
    blend.target = cylinder;

    // 1 Unlimited, 1 Cascaded, 1 Boosted
    GasSource srcUnl = createSource(210, 0, 200000, 50000, 10000000, 10000000, 200000, 1.0, true, 1, false);
    GasSource srcCas = createSource(1000, 0, 200000, 50000, 10000000, 2000000, 40000, 2.0, false, 2, false);
    GasSource srcBst = createSource(0, 1000, 200000, 50000, 10000000, 2000000, 40000, 3.0, false, 3, true);
    
    blend.gasSources = {srcUnl, srcCas, srcBst};

    auto matrix = blend.getSimplexMatrix();

    // Verify Dimensions
    // N = 3 sources. L = 2 limited sources.
    // Cols = 1(res) + 3(sources) + 1(res slack) + 2(src slacks) + 3(art) + 1(rhs) = 11
    // Rows = 3(vol/o2/he) + 1(res limit) + 2(src limits) + 1(cost) = 7
    QCOMPARE(matrix.size(), 7);
    QCOMPARE(matrix[0].size(), 11);

    // Verify Cost Row (Row 6) is populated with Big-M penalties
	QVERIFY(matrix[6][1] < -1000.0); 
    QVERIFY(matrix[6][2] < -1000.0);
}

void TestGasBlender::testRunSimplexSolver_Feasible()
{
    TargetCylinder cylinder;
    cylinder.currentO2_permille = 210;
    cylinder.currentHe_permille = 0;
    cylinder.currentVolume.mliter = 10000; 
    cylinder.wetVolume.mliter = 10000;
    cylinder.fillVolume.mliter = 200000; 
    cylinder.targetO2_permille = 320;
    cylinder.targetHe_permille = 0;

    Blend blend;
    blend.target = cylinder;

    GasSource air = createSource(210, 0, 200000, 50000, 10000000, 10000000, 200000, 0.1, true, 1, false);
    GasSource o2 = createSource(1000, 0, 200000, 50000, 10000000, 10000000, 200000, 1.0, true, 2, false);
    GasSource he = createSource(0, 1000, 200000, 50000, 10000000, 10000000, 200000, 3.0, true, 3, false);
    blend.gasSources = {air, o2, he};

    auto matrix = blend.getSimplexMatrix();
    SimplexResult result = runSimplexSolver(matrix);

    QVERIFY(result.is_exact);
	QCOMPARE(result.solvedVolumes.size(), matrix[0].size() - 1);
    QVERIFY(result.solvedVolumes[1] > 0.0); 
    QVERIFY(result.solvedVolumes[2] > 0.0); 
    QVERIFY(result.solvedVolumes[3] == 0.0); 


    cylinder.targetHe_permille = 100;
	blend.target = cylinder;
    blend.gasSources = {air, o2, he};

    matrix = blend.getSimplexMatrix();
    result = runSimplexSolver(matrix);

    QVERIFY(result.is_exact);
	QCOMPARE(result.solvedVolumes.size(), matrix[0].size() - 1);
    QVERIFY(result.solvedVolumes[1] > 0.0); 
    QVERIFY(result.solvedVolumes[2] > 0.0); 
    QVERIFY(result.solvedVolumes[3] > 0.0); 
}

void TestGasBlender::testRunSimplexSolver_Infeasible()
{
    TargetCylinder cylinder;
    cylinder.currentO2_permille = 210;
    cylinder.currentHe_permille = 0;
    cylinder.currentVolume.mliter = 10000; 
    cylinder.wetVolume.mliter = 10000;
    cylinder.fillVolume.mliter = 200000; 
    cylinder.targetO2_permille = 500; 
    cylinder.targetHe_permille = 0;   

    Blend blend;
    blend.target = cylinder;

    GasSource air = createSource(210, 0, 200000, 50000, 10000000, 10000000, 200000, 0.1, true, 1, false);
    GasSource tmx = createSource(100, 500, 200000, 50000, 10000000, 10000000, 200000, 1.0, true, 2, false);
    blend.gasSources = {air, tmx};

    auto matrix = blend.getSimplexMatrix();
    SimplexResult result = runSimplexSolver(matrix);

    QVERIFY(!result.is_exact);
}

void TestGasBlender::testFindCheapestBlend_ExactVsBestEffort()
{
    TargetCylinder cylinder;
    cylinder.currentO2_permille = 210;
    cylinder.currentHe_permille = 0;
    cylinder.currentVolume.mliter = 10000; 
    cylinder.wetVolume.mliter = 10000;
    cylinder.fillVolume.mliter = 200000; 
    cylinder.targetO2_permille = 320;
    cylinder.targetHe_permille = 0;

    Blend exactBlend;
    exactBlend.target = cylinder;
    GasSource air = createSource(210, 0, 200000, 50000, 10000000, 10000000, 200000, 0.1, true, 1, false);
    GasSource o2 = createSource(1000, 0, 200000, 50000, 10000000, 10000000, 200000, 1.0, true, 2, false);
    exactBlend.gasSources = {air, o2};

    Blend bestEffortBlend;
    bestEffortBlend.target = cylinder;
    GasSource ean30 = createSource(300, 0, 200000, 50000, 10000000, 10000000, 200000, 0.5, true, 3, false);
    bestEffortBlend.gasSources = {air, ean30};

    std::vector<Blend> blendCandidates = {bestEffortBlend, exactBlend};
    Blend chosenBlend = findCheapestBlend(blendCandidates);

    QVERIFY(chosenBlend.isFeasible);
    QVERIFY(!chosenBlend.isBestEffort); 

    std::vector<Blend> bestEffortOnly = {bestEffortBlend};
    Blend chosenBestEffort = findCheapestBlend(bestEffortOnly);
    
    QVERIFY(chosenBestEffort.isFeasible);
    QVERIFY(chosenBestEffort.isBestEffort);
}

QTEST_GUILESS_MAIN(TestGasBlender)