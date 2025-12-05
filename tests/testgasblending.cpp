// SPDX-License-Identifier: GPL-2.0
#include "testgasblending.h"
#include "core/gasblending.h"

using Point = std::pair<double, double>;

using namespace Blender;

void TestGasBlender::setup()
{

}

void TestGasBlender::testIntersection()
{

    double out_x, out_y;

	//Base case
    QCOMPARE(calculateIntersection(0.0, 0.0, 5.0, 5.0, 0.0, 5.0, 5.0, 0.0, out_x, out_y), true);
    QCOMPARE(out_x, 2.5);
    QCOMPARE(out_y, 2.5);    

	//Parallel case
    QCOMPARE(calculateIntersection(1.0, 1.0, 5.0, 5.0, 1.0, 0.0, 5.0, 4.0, out_x, out_y), false);

	//Intersection outside of segments
	QCOMPARE(calculateIntersection(1.0, 1.0, 5.0, 5.0, 0.5, 0.5, 0.6, 7.0, out_x, out_y), false);

	//Vertical line, with intersection
	QCOMPARE(calculateIntersection(0.0, 0.0, 5.0, 5.0, 0.5, 0.0, 0.5, 7.0, out_x, out_y), true);
    QCOMPARE(out_x, 0.5);
    QCOMPARE(out_y, 0.5); 

	//Vertical line, no intersection
	QCOMPARE(calculateIntersection(1.0, 1.0, 5.0, 5.0, 0.5, 5.0, 0.5, 7.0, out_x, out_y), false);

	//Horizontal line, with intersection
	QCOMPARE(calculateIntersection(0.0, 0.0, 5.0, 5.0, 0.0, 0.5, 7.0, 0.5, out_x, out_y), true);
	QCOMPARE(out_x, 0.5);
	QCOMPARE(out_y, 0.5);

	//Horizontal line, no intersection
	QCOMPARE(calculateIntersection(1.0, 1.0, 5.0, 5.0, 0.0, 0.5, 7.0, 0.5, out_x, out_y), false);

	//Point on line, intersection
	QCOMPARE(calculateIntersection(0.0, 0.0, 5.0, 5.0, 0.5, 0.5, 0.5, 0.5, out_x, out_y), true);
	QCOMPARE(out_x, 0.5);
	QCOMPARE(out_y, 0.5);

	//Point on line, no intersection
	QCOMPARE(calculateIntersection(1.0, 1.0, 5.0, 5.0, 0.5, 0.5, 0.5, 0.5, out_x, out_y), false);

}

void TestGasBlender::test2DInterpolation()
{

	//Base case, all at 5.0
	QCOMPARE(linear_interpolate_2d(0.0, 0.0, 5.0, 5.0, 0.0, 5.0, 2.5, 0.0), 5.0);

	//Case with varying values
	QCOMPARE(linear_interpolate_2d(0.0, 0.0, 0.0, 5.0, 0.0, 5.0, 2.5, 0.0), 2.5);
	QCOMPARE(linear_interpolate_2d(0.0, 0.0, 0.0, 5.0, 5.0, 5.0, 2.0, 2.0), 2.0);

	//Y values being 0, test 1D linear interpolation
	QCOMPARE(linear_interpolate_2d(0.0, 0.0, 5.0, 5.0, 0.0, 10.0, 2.0, 0.0), 7.0);
	QCOMPARE(linear_interpolate_2d(0.0, 0.0, 0.0, 5.0, 0.0, 5.0, 2.5, 0.0), 2.5);

	//Test value equals a sample value
	QCOMPARE(linear_interpolate_2d(0.0, 0.0, 5.0, 5.0, 0.0, 0.0, 0.0, 0.0), 5.0);
	QCOMPARE(linear_interpolate_2d(0.0, 0.0, 0.0, 50.0, 25.0, 5.0, 50.0, 25.0), 5.0);

	//Test point is not within the two points
	QCOMPARE(linear_interpolate_2d(0.0, 0.0, 5.0, 0.0, 5.0, 10.0, 0.0, 10.0), 15.0);
}

void TestGasBlender::testGetCompressibilityFactor()
{

	pressure_t pressure;
	pressure.mbar = 5000;
	QCOMPARE(get_compressibility_factor(1000,0,0,pressure), 0.9968);
	QCOMPARE(get_compressibility_factor(500,250,250,pressure), 0.99875);
	QCOMPARE(get_compressibility_factor(200,0,800,pressure), 0.99856);

	pressure.mbar = 120000;
	QCOMPARE(get_compressibility_factor(450,250,300,pressure), 0.998389);

}

void TestGasBlender::testCalculateCylinderVolumes()
{
	TargetCylinder cylinder;
	cylinder.currentHe_permille = 100;
	cylinder.currentO2_permille = 200;
	cylinder.currentN2_permille = 700;
	cylinder.targetHe_permille = 100;
	cylinder.targetO2_permille = 200;
	cylinder.targetN2_permille = 700;
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

	QCOMPARE(cylinder.fillVolume.mliter, 11031);
	QCOMPARE(cylinder.currentVolume.mliter, 3026); 

}

void TestGasBlender::testSign()
{
	QCOMPARE(sign(2.0,2.0,2.0,5.0,1.0,2.0) > 0, true);
	QCOMPARE(sign(2.0,2.0,2.0,5.0,3.0,2.0) < 0, true);
}

void TestGasBlender::testPointInTriangle()
{


	QCOMPARE(pointInTriangle(2.0,2.0,2.0,5.0,1.0,2.0,3.0,2.0), false);
	QCOMPARE(pointInTriangle(2.0,2.0,2.0,5.0,3.0,2.0,2.5,2.0), true); //On the line
	QCOMPARE(pointInTriangle(2.0,2.0,2.0,5.0,3.0,2.0,3.0,2.0), true); //On the corner
	QCOMPARE(pointInTriangle(2.0,2.0,2.0,5.0,3.0,2.0,2.5,3.0), true); //Inside
	//Out far right
	QCOMPARE(pointInTriangle(2.0,2.0,2.0,5.0,3.0,2.0,15.0,2.5), false);
	//Out far left
	QCOMPARE(pointInTriangle(2.0,2.0,2.0,5.0,3.0,2.0,-10.0,2.5), false);
	//Out far up
	QCOMPARE(pointInTriangle(2.0,2.0,2.0,5.0,3.0,2.0,2.5,7.0), false);
	//Out far down
	QCOMPARE(pointInTriangle(2.0,2.0,2.0,5.0,3.0,2.0,2.0,-5.0), false);
}

void TestGasBlender::testTriangularInterpolation()
{
	//Neat option
	QCOMPARE(triangular_interpolate(0.0,0.0,0.0,0.0,10.0,0.0,10.0,5.0,100.0, 5.0, 5.0), 50.0);
	QCOMPARE(triangular_interpolate(0.0,0.0,100.0,0.0,10.0,0.0,10.0,5.0,0.0, 5.0, 5.0), 25.0);
	QCOMPARE(triangular_interpolate(0.0,0.0,0.0,0.0,10.0,100.0,10.0,5.0,0.0, 5.0, 5.0), 25.0);

	//Messier option
	QCOMPARE(triangular_interpolate(0.0,0.0,0.0,0.0,10.0,0.0,10.0,5.0,100.0, 3.0, 7.0), 30.0);
	QCOMPARE(triangular_interpolate(0.0,0.0,100.0,0.0,10.0,0.0,10.0,5.0,0.0, 3.0, 7.0), 15.0);
	QCOMPARE(triangular_interpolate(0.0,0.0,0.0,0.0,10.0,100.0,10.0,5.0,0.0, 3.0, 7.0), 55.0);

	//On a point
	QCOMPARE(triangular_interpolate(0.0,0.0,0.0,0.0,10.0,0.0,10.0,0.0,100.0, 0.0, 0.0), 0.0);
	QCOMPARE(triangular_interpolate(0.0,0.0,100.0,0.0,10.0,0.0,10.0,0.0,0.0, 0.0, 0.0), 100.0);
	QCOMPARE(triangular_interpolate(0.0,0.0,0.0,0.0,10.0,100.0,10.0,0.0,0.0, 0.0, 0.0), 0.0);

	//On a line
	QCOMPARE(triangular_interpolate(0.0,0.0,0.0,0.0,10.0,0.0,10.0,0.0,100.0, 0.0, 5.0), 0.0);
	QCOMPARE(triangular_interpolate(0.0,0.0,100.0,0.0,10.0,0.0,10.0,0.0,0.0, 0.0, 5.0), 50.0);
	QCOMPARE(triangular_interpolate(0.0,0.0,0.0,0.0,10.0,100.0,10.0,0.0,0.0, 0.0, 5.0), 50.0);

}

void TestGasBlender::testCalculateSourceMix()
{
	//NOTE: ALL VALIDATION MUST BE DONE BEFORE THIS FUNCTION. THIS FUNCTION ASSUMES THE CALCULATION IS POSSIBLE

	GasSource source1;
	GasSource source2;
	GasSource source3;
	double targetO2 = 0.32;
	double targetHe = 0.1;
	double targetVolume = 10000.0;

	source1.o2_permille = 210;
	source1.he_permille = 0;
	source2.o2_permille = 800;
	source2.he_permille = 0;
	source3.o2_permille = 0;
	source3.he_permille = 1000;


	std::vector<BlendStep> steps = calculateSourceMix(source1, source2, &source3, targetO2, targetHe, targetVolume);

	QCOMPARE(steps.size(), 3);
	QCOMPARE(steps[0].source.o2_permille, source1.o2_permille);
	QCOMPARE(steps[0].source.he_permille, source1.he_permille);
	QCOMPARE(steps[0].volume.mliter < 6785, true);
	QCOMPARE(steps[0].volume.mliter > 6775, true);
	QCOMPARE(steps[1].source.o2_permille, source2.o2_permille);
	QCOMPARE(steps[1].source.he_permille, source2.he_permille);
	QCOMPARE(steps[1].volume.mliter > 2219, true);
	QCOMPARE(steps[1].volume.mliter < 2221, true);
	QCOMPARE(steps[2].source.o2_permille, source3.o2_permille);
	QCOMPARE(steps[2].source.he_permille, source3.he_permille);
	QCOMPARE(steps[2].volume.mliter > 999, true);
	QCOMPARE(steps[2].volume.mliter < 1001, true);

	targetO2 = 0.4;
	targetHe = 0.0;

	steps = calculateSourceMix(source1, source2, nullptr, targetO2, targetHe, targetVolume);

	QCOMPARE(steps.size(), 2);
	QCOMPARE(steps[0].source.o2_permille, source1.o2_permille);
	QCOMPARE(steps[0].source.he_permille, source1.he_permille);
	QCOMPARE(steps[0].volume.mliter < 6781, true);
	QCOMPARE(steps[0].volume.mliter > 6778, true);
	QCOMPARE(steps[1].source.o2_permille, source2.o2_permille);
	QCOMPARE(steps[1].source.he_permille, source2.he_permille);
	QCOMPARE(steps[1].volume.mliter > 3219, true);
	QCOMPARE(steps[1].volume.mliter < 3221, true);

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
	cylinder.targetN2_permille = 700;
	cylinder.targetPressure.mbar = 10000;

	calculate_cylinder_volumes(cylinder);


	Blend blend;
	blend.target = cylinder;
	blend.removedGasVolume = 0;

	QCOMPARE(blend.currentHe(), 100.0);
	QCOMPARE(blend.currentO2(), 200.0);
	QCOMPARE(blend.neededHe(), 100.0);
	QCOMPARE(blend.neededO2(), 200.0);

	QCOMPARE(blend.currentVolume() > 2011, true);
	QCOMPARE(blend.currentVolume() < 2013, true);

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

	QCOMPARE(blend.currentVolume() > 3011, true);
	QCOMPARE(blend.currentVolume() < 3014, true);


	QCOMPARE(blend.currentHe() > 66.7, true); 
	QCOMPARE(blend.currentHe() < 66.9, true);

	QCOMPARE(blend.currentO2() > 202.9, true); 
	QCOMPARE(blend.currentO2() < 204.1, true);

	
	QCOMPARE(blend.neededHe() > 114, true);
	QCOMPARE(blend.neededHe() < 115, true);

	QCOMPARE(blend.neededO2() > 198.4, true);
	QCOMPARE(blend.neededO2() <  198.6, true);

	BlendStep step2;
	step2.source = source3;
	step2.volume.mliter = 900;
	blend.steps.push_back(step2);


	QCOMPARE(blend.currentVolume() > 3911, true);
	QCOMPARE(blend.currentVolume() < 3913, true);

	QCOMPARE(blend.currentHe() > 281.0, true); 
	QCOMPARE(blend.currentHe() < 282.0, true);

	QCOMPARE(blend.currentO2() > 156.0, true); 
	QCOMPARE(blend.currentO2() < 157.0, true);

	
	QCOMPARE(blend.neededHe() < -16.0, true);
	QCOMPARE(blend.neededHe() > -17.0, true);

	QCOMPARE(blend.neededO2() > 227.0, true);
	QCOMPARE(blend.neededO2() < 228.0, true);

	blend.steps.clear();

	blend.removedGasVolume = 500;
	QCOMPARE(blend.currentHe(), 100.0);
	QCOMPARE(blend.currentO2(), 200.0);
	QCOMPARE(blend.neededHe(), 100.0);
	QCOMPARE(blend.neededO2(), 200.0);

	QCOMPARE(blend.currentVolume() > 1511, true); //Within 1 mliter
	QCOMPARE(blend.currentVolume() < 1513, true);


	blend.steps.push_back(step1);

	QCOMPARE(blend.currentVolume() > 2511, true);
	QCOMPARE(blend.currentVolume() < 2513, true);

	QCOMPARE(blend.currentHe() > 60, true); 
	QCOMPARE(blend.currentHe() < 61, true);

	QCOMPARE(blend.currentO2() > 203, true); 
	QCOMPARE(blend.currentO2() < 204, true);

	
	QCOMPARE(blend.neededHe() > 113, true);
	QCOMPARE(blend.neededHe() < 114, true);

	QCOMPARE(blend.neededO2() > 198, true);
	QCOMPARE(blend.neededO2() <  199, true);

	blend.steps.clear();

	cylinder.wetVolume.mliter = 0;
	cylinder.currentPressure.mbar = 1500;
	cylinder.targetO2_permille = 400;
	calculate_cylinder_volumes(cylinder);
	blend.target = cylinder;
	blend.removedGasVolume = 0;

	auto steps = calculateSourceMix(source1, source2, &source3, blend.neededO2()/1000.0, blend.neededHe()/1000.0, cylinder.fillVolume.mliter - blend.currentVolume());
	for (auto step : steps)
	{
		blend.steps.push_back(step);
	}
	QCOMPARE(blend.validate(false), true);

	GasSource source4;
	source4.o2_permille = 0;
	source4.he_permille = 1000;
	source4.cost_per_unit_volume = 7.0;
	source4.wetVolume.mliter = 1013;
	source4.fillVolume.mliter = 10130;
	source4.currentPressure.mbar = 10130;
	source4.workingPressure.mbar = 10130;
	source4.currentVolume.mliter = 10130;
	source4.unlimited = false;
	source4.boosted = false;

	blend.steps.clear();
	BlendStep step3;
	step3.source = source4;
	step3.volume.mliter = 700;
	blend.steps.push_back(step3);


	QCOMPARE(blend.validate(true), true);

	steps = calculateSourceMix(source1, source2, &source3, blend.neededO2()/1000.0, blend.neededHe()/1000.0, cylinder.fillVolume.mliter - blend.currentVolume());
	for (auto step : steps)
	{	
		blend.steps.push_back(step);
	}
	QCOMPARE(blend.validate(false), true);
	QCOMPARE(blend.cost() > 17.84, true);
	QCOMPARE(blend.cost() < 17.85, true);
	
}

void TestGasBlender::testCorrectPressures()
{
	Blend blend;

	GasSource source1;
	GasSource source2;
	GasSource source3;

	TargetCylinder cylinder;


	cylinder.currentHe_permille = 0;
	cylinder.currentO2_permille = 210;
	cylinder.currentN2_permille = 790;
	cylinder.currentPressure.mbar = 5000;
	cylinder.workingPressure.mbar = 10000;
	cylinder.wetVolume.mliter = 10000;
	cylinder.fillVolume.mliter = 100000;
	cylinder.targetHe_permille = 150;
	cylinder.targetO2_permille = 320;
	cylinder.targetPressure.mbar = 10000;
	calculate_cylinder_volumes(cylinder);

	blend.target = cylinder;
	blend.removedGasVolume = 0;

	source1.o2_permille = 210;
	source1.he_permille = 0;
	source2.o2_permille = 800;
	source2.he_permille = 0;
	source3.o2_permille = 0;
	source3.he_permille = 1000;

	source1.currentVolume.mliter = 50000;
	source1.wetVolume.mliter = 10000;

	source2.currentVolume.mliter = 20000;
	source2.wetVolume.mliter = 10000;

	source3.currentVolume.mliter = 15000;
	source3.wetVolume.mliter = 10000;
	
	source1.boosted = false;
	source2.boosted = false;
	source3.boosted = false;

	BlendStep step1;
	step1.source = source1;
	step1.volume.mliter = 50000;
	blend.steps.push_back(step1);

	BlendStep step2;
	step2.source = source2;
	step2.volume.mliter = 20000;
	blend.steps.push_back(step2);

	BlendStep step3;
	step3.source = source3;
	step3.volume.mliter = 15000;
	blend.steps.push_back(step3);

	QCOMPARE(blend.correctPressures(), false);

	source1.currentVolume.mliter = 40000;
	source2.currentVolume.mliter = 70000;
	source3.currentVolume.mliter = 90000;

	step1.source = source1;
	step2.source = source2;
	step3.source = source3;
	
	blend.steps.clear();
	blend.steps.push_back(step1);
	blend.steps.push_back(step2);
	blend.steps.push_back(step3);
	blend.removedGasVolume = 25000;

	QCOMPARE(blend.correctPressures(), true);

	QCOMPARE(blend.validate(true), true);	


	source1.currentVolume.mliter = 500000;
	source2.currentVolume.mliter = 500000;
	source3.currentVolume.mliter = 250000;

	step1.source = source1;
	step2.source = source2;
	step3.source = source3;
	blend.steps.clear();
	blend.steps.push_back(step1);
	blend.steps.push_back(step2);
	blend.steps.push_back(step3);
	QCOMPARE(blend.correctPressures(), false);

	blend.steps.clear();
	source3.boosted = true;
	step3.source = source3;
	blend.steps.push_back(step1);
	blend.steps.push_back(step2);
	blend.steps.push_back(step3);
	QCOMPARE(blend.correctPressures(), true);


	QCOMPARE(blend.validate(true), true);	
	
}

void TestGasBlender::testCalculatePressure()
{
	int o2_permille = 200;
	int he_permille = 100;
	int wet_volume = 10000;
	int gas_volume = 20000;
	double pressure = calculatePressure(o2_permille, he_permille, wet_volume, gas_volume);

	QCOMPARE(pressure > 2026.0, true);
	QCOMPARE(pressure < 2026.1, true);

	gas_volume = 100000;
	pressure = calculatePressure(o2_permille, he_permille, wet_volume, gas_volume);
	QCOMPARE(pressure > 10110.0, true);
	QCOMPARE(pressure < 10120.0, true);

}

void TestGasBlender::testInterpolateBlends()
{
	Blend blend1;
	TargetCylinder cylinder1;
	cylinder1.currentHe_permille = 0;
	cylinder1.currentO2_permille = 200;
	cylinder1.currentN2_permille = 800;
	cylinder1.currentPressure.mbar = 5000;
	cylinder1.workingPressure.mbar = 10000;
	cylinder1.wetVolume.mliter = 0;
	cylinder1.fillVolume.mliter = 10000;
	cylinder1.targetHe_permille = 0;
	cylinder1.targetO2_permille = 500;
	cylinder1.targetN2_permille = 500;
	cylinder1.targetPressure.mbar = 10000;
	calculate_cylinder_volumes(cylinder1);
	blend1.target = cylinder1;
	blend1.removedGasVolume = 1500;

	Blend blend2;
	blend2.target = cylinder1;
	blend2.removedGasVolume = 0;

	Blend *result = interpolateBlends(&blend1, &blend2, nullptr, 731, 0);
	QCOMPARE(result->neededO2() > 730, true);
	QCOMPARE(result->neededO2() < 732, true);
	//This will need expanding once complex blending is implemented
}

void TestGasBlender::testCalculateBlends()
{
	//Blend* calculate_blend(const Blend* blend1, const Blend* blend2, const Blend* blend3, const GasSource* source1, const GasSource* source2, const GasSource* source3)
	//Up to 2 Blends, up to 3 GasSources

	//Build the test cases
	TargetCylinder cylinder;
	cylinder.currentHe_permille = 0;
	cylinder.currentO2_permille = 210;
	cylinder.currentN2_permille = 790;
	cylinder.currentPressure.mbar = 5000;
	cylinder.workingPressure.mbar = 10000;
	cylinder.wetVolume.mliter = 0;
	cylinder.fillVolume.mliter = 100000;
	cylinder.targetHe_permille = 150;
	cylinder.targetO2_permille = 320;
	cylinder.targetPressure.mbar = 10000;
	calculate_cylinder_volumes(cylinder);

	Blend blend1;
	blend1.target = cylinder;
	blend1.removedGasVolume = 0;

	GasSource source1;
	GasSource source2;
	GasSource source3;

	source1.o2_permille = 210;
	source1.he_permille = 0;
	source2.o2_permille = 800;
	source2.he_permille = 0;
	source3.o2_permille = 0;
	source3.he_permille = 1000;
	source1.unlimited = true;
	source2.unlimited = true;
	source3.unlimited = true;
	
	Blend* result = calculate_blend(&blend1, nullptr, nullptr, &source1, &source2, &source3);
	QCOMPARE(result == nullptr, false); 
	QCOMPARE(result->validate(false), true);

	blend1.target.targetHe_permille = 0;
	Blend* result2 = calculate_blend(&blend1, nullptr, nullptr, &source1, &source2, nullptr);
	QCOMPARE(result2 == nullptr, false); 
	QCOMPARE(result2->validate(false), true);
	
	blend1.target.targetO2_permille = 210;
	Blend* result3 = calculate_blend(&blend1, nullptr, nullptr, &source1, nullptr, nullptr);
	QCOMPARE(result3 == nullptr, false); 
	QCOMPARE(result3->validate(false), true);

	Blend blend2;
	blend2.target = cylinder;
	blend2.removedGasVolume = 35000;

	blend1.target.targetO2_permille = 600;
	blend2.target.targetO2_permille = 600;
	blend1.target.targetHe_permille = 0;
	blend2.target.targetHe_permille = 0;

	Blend* result4 = calculate_blend(&blend1, &blend2, nullptr, &source2, nullptr, nullptr);
	QCOMPARE(result4 == nullptr, false); 
	QCOMPARE(result4->validate(false), true);
	
	blend1.target.targetO2_permille = 600;
	blend2.target.targetO2_permille = 600;
	blend2.target.targetHe_permille = 50;
	blend1.target.targetHe_permille = 50;
	blend1.steps.clear();
	blend2.steps.clear();

	Blend* result5 = calculate_blend(&blend1, &blend2, nullptr, &source2, &source3, nullptr);
	QCOMPARE(result5 == nullptr, false); 
	QCOMPARE(result5->validate(false), true);

	blend1.target.targetO2_permille = 400;
	blend2.target.targetO2_permille = 400;
	blend2.target.targetHe_permille = 50;
	blend1.target.targetHe_permille = 50;
	blend1.steps.clear();
	blend2.steps.clear();

	GasSource source4;
	source4.o2_permille = 0;
	source4.he_permille = 0;
	source4.unlimited = true;

	Blend blend3;
	blend3.target = cylinder;
	blend3.target.targetO2_permille = 400;
	blend3.target.targetHe_permille = 50;
	blend3.removedGasVolume = 25000;

	BlendStep step1;
	step1.source = source4;
	step1.volume.mliter = 45000;
	blend3.steps.push_back(step1);
	

	Blend* result6 = calculate_blend(&blend1, &blend3, nullptr, &source1, &source2, &source3);
	QCOMPARE(result6 == nullptr, false); 
	QCOMPARE(result6->validate(false), true);

	blend1.target.targetO2_permille = 600;
	blend2.target.targetO2_permille = 600;
	blend2.target.targetHe_permille = 0;
	blend1.target.targetHe_permille = 0;
	blend3.target.targetO2_permille = 600;
	blend3.target.targetHe_permille = 0;

	step1.source.he_permille = 500;
	blend1.steps.clear();
	blend2.steps.clear();
	blend3.steps.clear();
	blend3.steps.push_back(step1);

	Blend* result7 = calculate_blend(&blend1, &blend2, &blend3, &source2, nullptr, nullptr);
	QCOMPARE(result7 == nullptr, false); 
	QCOMPARE(result7->validate(false), true);

	blend1.steps.clear();
	blend2.steps.clear();
	blend3.steps.clear();
	blend3.steps.push_back(step1);

	Blend* result8 = calculate_blend(&blend1, &blend2, &blend3, &source2, &source3, nullptr);
	QCOMPARE(result8 == nullptr, false); 
	QCOMPARE(result8->validate(false), true);

	blend1.steps.clear();
	blend2.steps.clear();
	blend3.steps.clear();
	blend3.steps.push_back(step1);

	blend1.target.targetHe_permille = 50;
	blend2.target.targetHe_permille = 50;
	blend3.target.targetHe_permille = 50;

	Blend* result9 = calculate_blend(&blend1, &blend2, &blend3, &source1, &source2, &source3);
	QCOMPARE(result9 == nullptr, false); 
	QCOMPARE(result9->validate(false), true);
}

void TestGasBlender::testConvexHull(){
	std::vector<Point> points;
	Point point1;
	Point point2;
	Point point3;
	Point point4;

	point1.first = 0;
	point1.second = 0;
	point2.first = 1;
	point2.second = 0;
	point3.first = 1;
	point3.second = 1;
	point4.first = 0;
	point4.second = 1;

	points.push_back(point1);
	points.push_back(point2);
	points.push_back(point3);
	points.push_back(point4);
	auto result = get_convex_hull(points);

	QCOMPARE(result.size(), 4);

	point1.first = .75;
	point1.second = .75;

	points.clear();

	points.push_back(point1);
	points.push_back(point2);
	points.push_back(point3);
	points.push_back(point4);
	auto result2 = get_convex_hull(points);

	QCOMPARE(result2.size(), 3);

	for (auto p : result2){
		QCOMPARE(p.first == .75, false);
	}
}

void TestGasBlender::testGasSourceTriangle(){

	GasSource source1;
	GasSource source2;
	GasSource source3;
	GasSource source4;

	source1.cost_per_unit_volume = 0;
	source1.cylinder_number = 0;

	source2.cost_per_unit_volume = .5;
	source2.cylinder_number = 1;

	source3.cost_per_unit_volume = .25;
	source3.cylinder_number = 2;
	
	source4.cost_per_unit_volume = .33;
	source4.cylinder_number = 3;

	source1.unlimited = true;
	source2.unlimited = true;
	source3.unlimited = true;
	source4.unlimited = true;


	std::vector<GasSource> sources;
	sources.push_back(source1);


	auto result = getSourceTriangles(sources);
	QCOMPARE(result.size(), 1);
	QCOMPARE(result[0].source1.cylinder_number,0);
	QCOMPARE(result[0].itemCount, 1);

	sources.push_back(source2);
	result = getSourceTriangles(sources);
	QCOMPARE(result.size(), 1);
	QCOMPARE(result[0].source1.cylinder_number,0);
	QCOMPARE(result[0].source2.cylinder_number,1);
	QCOMPARE(result[0].itemCount, 2);

	sources.push_back(source3);
	result = getSourceTriangles(sources);
	QCOMPARE(result.size(), 1);
	QCOMPARE(result[0].source1.cylinder_number,0);
	QCOMPARE(result[0].source2.cylinder_number,1);
	QCOMPARE(result[0].source3.cylinder_number,2);
	QCOMPARE(result[0].itemCount, 3);

	sources.push_back(source4);
	result = getSourceTriangles(sources);
	QCOMPARE(result.size(), 4);
	auto firstTriangle = result[0];
	double firstCost = firstTriangle.source1.cost_per_unit_volume + firstTriangle.source2.cost_per_unit_volume + firstTriangle.source3.cost_per_unit_volume;
	QCOMPARE(firstCost, source1.cost_per_unit_volume + source4.cost_per_unit_volume + source3.cost_per_unit_volume);

	auto secondTriangle = result[1];
	double secondCost = secondTriangle.source1.cost_per_unit_volume + secondTriangle.source2.cost_per_unit_volume + secondTriangle.source3.cost_per_unit_volume;
	QCOMPARE(secondCost, source1.cost_per_unit_volume + source2.cost_per_unit_volume + source3.cost_per_unit_volume);

	auto thirdTriangle = result[2];
	double thirdCost = thirdTriangle.source1.cost_per_unit_volume + thirdTriangle.source2.cost_per_unit_volume + thirdTriangle.source3.cost_per_unit_volume;
	QCOMPARE(thirdCost, source1.cost_per_unit_volume + source2.cost_per_unit_volume + source4.cost_per_unit_volume);

	auto fourthTriangle = result[3];
	double fourthCost = fourthTriangle.source1.cost_per_unit_volume + fourthTriangle.source2.cost_per_unit_volume + fourthTriangle.source3.cost_per_unit_volume;
	QCOMPARE(fourthCost, source4.cost_per_unit_volume + source3.cost_per_unit_volume + source2.cost_per_unit_volume);
	
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
	QCOMPARE(result.size(), 3360);

	//These blend triangles are difficult to test specifically.. I manually checked the outputs, which tend to
	//be halfing the volume of the previous step in the blend. These blend objects are NOT necessarily "valid"
	// because they can go above the cylinder volume or max O2/He values. This is ok.

}

QTEST_GUILESS_MAIN(TestGasBlender)
