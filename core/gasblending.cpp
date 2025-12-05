    #include "gasblending.h"
    #include "units.h"
    #include <cmath>      
    #include <algorithm>
    #include <vector>
    #include "errorhelper.h"


    using namespace std;
namespace Blender {

    // ============================================================================
    // Blend Member Implementations
    // ============================================================================

    /**
     * @brief: Get the series of OutputSteps representing this blend object
     */
    std::vector<OutputStep> Blend::getOutputSteps() const {
        std::vector<OutputStep> outputSteps;
        double currentVolume = target.currentVolume.mliter - removedGasVolume;
        double stepEndPressure = target.currentPressure.mbar;
        double currentO2Vol = (target.currentVolume.mliter - removedGasVolume) / 1000.0 * target.currentO2_permille;
        double currentHeVol = (target.currentVolume.mliter - removedGasVolume) / 1000.0 * target.currentHe_permille;
        if (removedGasVolume > 0) {
            OutputStep os;
            os.add = false;
            os.volume.mliter = removedGasVolume;
            os.weight =  removedGasVolume * target.currentO2_permille / 1000.0 * O2_g + removedGasVolume * target.currentHe_permille / 1000.0 * He_g + removedGasVolume * target.currentN2_permille / 1000.0 * N2_g;
            //Get the pressure of the cylinder AFTER this step
            stepEndPressure = calculatePressure(target.currentO2_permille, target.currentHe_permille, target.wetVolume.mliter, currentVolume);
            os.gaugePressure.mbar = stepEndPressure - ATM;
            os.cost_per_unit_volume = 0.0;
            os.mix = "N/A";
            os.limited_pressure = -1;
            os.cylinder_number = -1;
            outputSteps.push_back(os);
        }
        for (const BlendStep& step : steps) {
            if (step.volume.mliter < 1000) { //Some margin of error. This is ~.035 cuft. This will not have a huge effect. It is < 1.5 grams of gas 
                continue;
            }
            currentVolume += step.volume.mliter;
            currentO2Vol += step.volume.mliter * (step.source.o2_permille / 1000.0);
            currentHeVol += step.volume.mliter * (step.source.he_permille / 1000.0);
            OutputStep os;
            os.add = true;
            os.volume.mliter = step.volume.mliter;
            int stepO2Permille = currentO2Vol * 1000.0 / currentVolume;
            int stepHePermille = currentHeVol * 1000.0 / currentVolume;
            int stepN2Permille = 1000 - stepO2Permille - stepHePermille;
            os.weight = step.volume.mliter * (step.source.o2_permille / 1000.0) * O2_g + step.volume.mliter * (step.source.he_permille / 1000.0) * He_g + step.volume.mliter * (stepN2Permille / 1000.0) * N2_g;
            //Get the pressure of the cylinder AFTER this step. To do this, we need the permille of the gas after the step

            stepEndPressure = calculatePressure(stepO2Permille, stepHePermille, target.wetVolume.mliter, currentVolume);
            os.gaugePressure.mbar = stepEndPressure - ATM;
            os.limited_pressure = -1;
            if (!step.source.unlimited){
                os.limited_pressure = calculatePressure(step.source.o2_permille, step.source.he_permille, step.source.wetVolume.mliter, step.source.currentVolume.mliter - step.volume.mliter) - ATM;
            }
            os.cost_per_unit_volume = step.source.cost_per_unit_volume;
            os.mix = std::to_string(step.source.o2_permille / 10) + "/" + std::to_string(step.source.he_permille / 10);
            os.cylinder_number = step.source.cylinder_number;
            outputSteps.push_back(os);
        }
        return outputSteps;
    }

    /**
     * @brief Returns the current percentage of O2 in the blend, in permille
     * * Start by removing the gas from the target cylinder
     * Then for each step, add the amount of gas from the GasSource
     * Then calculate the final percentage of O2 in the gas, in permille
     */
    double Blend::currentO2() const {
        double currentVol = target.currentVolume.mliter - removedGasVolume;
        double volumeO2 = target.currentO2_permille / 1000.0 * currentVol;
        for (const BlendStep& step : steps) {
            currentVol += step.volume.mliter;
            volumeO2 += step.volume.mliter / 1000.0 * step.source.o2_permille;
        }
        return (volumeO2 * 1000.0/ currentVol);
    }

    /**
     * @brief Returns the current percentage of He in the blend, in permille
     * * Start by removing the gas from the target cylinder
     * Then for each step, add the amount of gas from the GasSource
     * Then calculate the final percentage of He in the gas, in permille
     */
    double Blend::currentHe() const {
        double currentVol = target.currentVolume.mliter - removedGasVolume;
        double volumeHe = target.currentHe_permille / 1000.0 * currentVol;
        for (const BlendStep& step : steps) {
            currentVol += step.volume.mliter;
            volumeHe += step.volume.mliter / 1000.0 * step.source.he_permille;

        }
        return (volumeHe * 1000.0/ currentVol);
    }

    double Blend::neededO2() const {
        double neededVol = target.fillVolume.mliter - currentVolume();

        double totalO2Vol = target.fillVolume.mliter / 1000.0 * target.targetO2_permille;
        double currentO2Vol = (currentVolume()) * (currentO2() / 1000.0);
        double volumeO2Needed = totalO2Vol - currentO2Vol;

        return (volumeO2Needed * 1000.0 / neededVol);
    }

    /**
     * @brief: Interpolating on neededO2 is a hyperbolic problem. This normalizes it by taking in
     * a target capacity and O2 percentage for our target
     */
    double Blend::neededO2BlendInterpolation(int o2_permille_blend, int target_volume) const {
        double neededVol = target.fillVolume.mliter - target_volume;

        double totalO2Vol = target.fillVolume.mliter * (target.targetO2_permille / 1000.0);
        double currentO2Vol = (currentVolume()) * currentO2() / 1000.0 + (target_volume - currentVolume()) * o2_permille_blend / 1000.0;
        double volumeO2Needed = totalO2Vol - currentO2Vol;

        return (volumeO2Needed * 1000.0 / neededVol);
    }

    double Blend::neededHe() const {
        double neededVol = target.fillVolume.mliter - currentVolume();

        double totalHeVol = target.fillVolume.mliter * (target.targetHe_permille / 1000.0);
        double currentHeVol = (currentVolume()) * (currentHe() / 1000.0);
        double volumeHeNeeded = totalHeVol - currentHeVol;
        
        return (volumeHeNeeded * 1000.0 / neededVol);
    }

    double Blend::neededHeBlendInterpolation(int he_permille_blend, int target_volume) const {
        double neededVol = target.fillVolume.mliter - target_volume;

        double totalHeVol = target.fillVolume.mliter * (target.targetHe_permille / 1000.0);
        double currentHeVol = (currentVolume()) * currentHe() / 1000.0 + (target_volume - currentVolume()) * he_permille_blend / 1000.0;
        double volumeHeNeeded = totalHeVol - currentHeVol;
        
        return (volumeHeNeeded * 1000.0 / neededVol);
    }

    double Blend::currentVolume() const {
        double currentVol = target.currentVolume.mliter - removedGasVolume;
        for (const BlendStep& step : steps) {
            currentVol += step.volume.mliter;
        }
        return currentVol;
    }

    /**
     * @brief Checks if the blend is valid and fills the target cylinder correctly depending on partial
     * * If partial is true, we only want to make sure the current blend steps can be accomplished
     * If partial is false, we want to make sure the current blend steps can be accomplished and the target cylinder is full
     * */
    bool Blend::validate(bool partial) const {
        double round_factor = 1.0; //Allow for slightly more or less than the targets, this is 1 permille
        double neededO2Vol_max = target.fillVolume.mliter / 1000.0 * (target.targetO2_permille + round_factor);
        double neededHeVol_max = target.fillVolume.mliter / 1000.0 * (target.targetHe_permille + round_factor);
        double neededO2Vol_min = target.fillVolume.mliter / 1000.0 * (target.targetO2_permille - round_factor);
        double neededHeVol_min = target.fillVolume.mliter / 1000.0 * (target.targetHe_permille - round_factor);
        if(target.currentVolume.mliter - removedGasVolume < target.wetVolume.mliter){
            return false; //Cannot remove gas such that the pressure is less than the atmospheric pressure
        }
        double currentO2Vol = (target.currentVolume.mliter - removedGasVolume) / 1000.0 * target.currentO2_permille;
        double currentHeVol = (target.currentVolume.mliter - removedGasVolume) / 1000.0 * target.currentHe_permille;
        double currentVol = target.currentVolume.mliter - removedGasVolume;

        double currentPressure = calculatePressure(target.currentO2_permille, target.currentHe_permille, target.wetVolume.mliter, currentVol) - ATM;
        
        //We now know that we are in a valid state. We need to iterate over the blend steps and check if they are valid
        for(const BlendStep& step : steps) {
            double stepO2Vol = step.volume.mliter / 1000.0 * step.source.o2_permille;
            double stepHeVol = step.volume.mliter / 1000.0 * step.source.he_permille;
            currentO2Vol += stepO2Vol;
            currentHeVol += stepHeVol;
            currentVol += step.volume.mliter;
            currentPressure = calculatePressure(currentO2Vol * 1000 / currentVol, currentHeVol * 1000 / currentVol, target.wetVolume.mliter, currentVol) - ATM;

            if (!step.source.unlimited)
            {
                double stepEndPressure = calculatePressure(step.source.o2_permille, step.source.he_permille, step.source.wetVolume.mliter ,step.source.currentVolume.mliter - step.volume.mliter) - ATM;
                if(currentPressure > stepEndPressure && !step.source.boosted){
                    return false; //Cannot add gas such that the pressure is greater than the end pressure of the step
                }
                if(step.source.boosted){
                    if(stepEndPressure < BOOSTED_LIMIT){
                        return false;
                    }
                }
            }
            if ((currentO2Vol > neededO2Vol_max || currentHeVol > neededHeVol_max) && !partial){
                return false; //We have added too much. This only matters if this is a final validation check
            }
        }

        if (!partial){
            if (currentO2Vol < neededO2Vol_min || currentHeVol < neededHeVol_min){
                return false; //We have not added enough
            }
        }
        return true; //Validation Complete
    }

    /**
     * @brief Iterate over all of the presures for the sources in the steps, changing them so we get the most out of each source
     *          that we can. Return false IF ANY PRESSURES ARE REDUCED TO 0
     */
    bool Blend::correctPressures() {

        int currentVolume = target.currentVolume.mliter - removedGasVolume;
        int currentStepO2 = target.currentO2_permille / 1000.0 * currentVolume;
        int currentStepHe = target.currentHe_permille / 1000.0 * currentVolume;
        int currentPressure = calculatePressure(target.currentO2_permille, target.currentHe_permille, target.wetVolume.mliter, currentVolume) - ATM; // get gauge pressure
        vector<BlendStep> newSteps;
        for (const BlendStep& step : steps) {
            int sourcePressure = calculatePressure(step.source.o2_permille, step.source.he_permille, step.source.wetVolume.mliter, step.source.currentVolume.mliter) - ATM; //Get gauge pressure
            //Do a simple estimate on how much volume we can use as a proportion of the source pressure
            // Remove 10 millibar to adjust for inaccuracies in the calculation

            //Middle pressure is = (P1V1 + P2V2) / (V1 + V2). This is doable with a single calculatePressure call if I can get the permilles

            int totalVolume = step.source.currentVolume.mliter + currentVolume;
            int totalO2permille = (step.source.currentVolume.mliter / 1000.0 * step.source.o2_permille + currentStepO2) / totalVolume * 1000;
            int totalHepermille = (step.source.currentVolume.mliter / 1000.0 * step.source.he_permille + currentStepHe) / totalVolume * 1000;

            double midPressure = calculatePressure(totalO2permille, totalHepermille, target.wetVolume.mliter + step.source.wetVolume.mliter, totalVolume) - ATM;
            if(step.source.boosted && midPressure > (BOOSTED_LIMIT + 250)){
                midPressure = BOOSTED_LIMIT + 250; //Add a little bit of leaway to help ensure validation
            }
            double midVolume = (step.source.currentVolume.mliter) * (midPressure + ATM) / (sourcePressure + ATM);
            double newVolume = step.source.currentVolume.mliter - midVolume - 250; //250 to ensure we do not equilize the tanks
            double new_source_pressure = calculatePressure(step.source.o2_permille, step.source.he_permille, step.source.wetVolume.mliter, step.source.currentVolume.mliter - newVolume) - ATM;
            if ((sourcePressure <= currentPressure && !step.source.boosted) || newVolume <= 0){
                return false; //Ignore this gas source in this order
            }
            currentVolume += newVolume;
            currentStepO2 += newVolume / 1000.0 * step.source.o2_permille;
            currentStepHe += newVolume / 1000.0 * step.source.he_permille;
            currentPressure = calculatePressure(currentStepO2 / currentVolume, currentStepHe / currentVolume, target.wetVolume.mliter, currentVolume) - ATM;

            BlendStep newStep;
            newStep.source = step.source;
            newStep.volume.mliter = newVolume;
            newSteps.push_back(newStep);
        }
        steps = newSteps;
        return true;

    }

    double Blend::cost() const { 
        double cost = 0.0;
        for (const BlendStep& step : steps) {
            cost += step.volume.mliter / 1000.0 * step.source.cost_per_unit_volume;
        }
        return cost; 
    }

    // ============================================================================
    // Logic Function Implementations
    // ============================================================================

    /**
     * @brief: Calculate the pressure of a mix in a given wet volume
     * 
     * This uses an approximation on the compressibility, then uses that approximation to calculate the pressure
     * (In theory, there is a way to not use this approximation, but this should be good enough)
     */
    double calculatePressure(int o2_permille, int he_permille, double wetVolume, double gasVolume) {
        int n2_permille = 1000 - o2_permille - he_permille;
        pressure_t atm_pressure, approx_pressure;
        atm_pressure.mbar = ATM;
        double atm_factor = get_compressibility_factor(o2_permille, he_permille, n2_permille,atm_pressure);
        approx_pressure.mbar = ATM * gasVolume / wetVolume;
        double approx_factor = get_compressibility_factor(o2_permille, he_permille, n2_permille, approx_pressure);
        //pv/z = pv/z
        double final_pressure = approx_factor / atm_factor * ATM * gasVolume / wetVolume;
        return final_pressure;    
    }

    /**
     * @brief: 2 dimensional LINEAR interpolation. This performs linear interpolation between 2 points in a 2D plane.
     * The function returns the interpolated z value on the x-y-z plane with a target x,y pair.
     */
    double linear_interpolate_2d(double x1, double y1, double z1, double x2, double y2, double z2, double target_x, double target_y){
        if (x1 == x2 && y1 == y2){
            return z1;
        }
        if (x1 == x2){
            //Linear interpolate between the two y values, which we know are not identical
            return z1 + (target_y - y1) * (z2 - z1) / (y2 - y1);
        }     
        //Linear interpolate between the two x values, which we know are not identical
        return z1 + (target_x - x1) * (z2 - z1) / (x2 - x1);
    }

    /**
     * Performs linear interpolation for a point within a 3D triangle.
     * * This function calculates the z-value of a target (x, y) point by determining
     * its barycentric coordinates relative to the 2D triangle defined by
     * (x1, y1), (x2, y2), and (x3, y3). It then applies these same coordinates
     * (weights) to the z-values (z1, z2, z3) to find the interpolated z-value.
     */
    double triangular_interpolate(double x1, double y1, double z1,
                                        double x2, double y2, double z2,
                                        double x3, double y3, double z3,
                                        double target_x, double target_y) {

        // Calculate the determinant (which is 2x the signed area of the triangle)
        // This is used as the denominator for calculating the weights.
        double det = (y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3);

        // Handle the degenerate case: points are collinear (form a line, not a triangle).
        // We check against a small epsilon for floating-point stability.
        if (std::abs(det) < 1e-9) {
            // Cannot interpolate on a 0-area triangle.
            // Check if the target point is on the line defined by the first 2 points, if so, do 2D linear interpolation.
            double tmpx, tmpy;
            if (calculateIntersection(x1, y1, x2, y2, target_x, target_y, target_x, target_y, tmpx, tmpy)){
                return linear_interpolate_2d(x1, y1, z1, x2, y2, z2, target_x, target_y);
            } else if(calculateIntersection(x2, y2, x3, y3, target_x, target_y, target_x, target_y, tmpx, tmpy)){
                return linear_interpolate_2d(x2, y2, z2, x3, y3, z3, target_x, target_y);
            }
            return -50000;
        }
        
        // Calculate the barycentric coordinates (weights) for the target point.
        // w1 corresponds to point 1, w2 to point 2, and w3 to point 3.
        double w1 = ((y2 - y3) * (target_x - x3) + (x3 - x2) * (target_y - y3)) / det;
        double w2 = ((y3 - y1) * (target_x - x3) + (x1 - x3) * (target_y - y3)) / det;
        double w3 = 1.0 - w1 - w2; // The weights must sum to 1.0

        // Apply the weights to the z-values to get the interpolated result.
        return w1 * z1 + w2 * z2 + w3 * z3;
    }

    /**
     * @brief Calculates the compressibility factor (Z-factor) for a gas mix at a given pressure.
     * This uses lookup tables and linear interpolation. The Z-factors are for gases at ~20°C.
     */
    double get_compressibility_factor(int o2_permille, int he_permille, int n2_permille, pressure_t pressure)
    {
        // Lookup tables for Z-factors at different pressures (in bar), from compressibility.r
        static const double pressure_bar_table[] = { 1, 5, 10, 20, 40, 60, 80, 100, 200, 300, 400, 500 };
        static const double z_o2_table[] = { 0.9994, 0.9968, 0.9941, 0.9884, 0.9771, 0.9676, 0.9597, 0.9542, 0.9560, 0.9972, 1.0689, 1.1572 };
        static const double z_n2_table[] = { 0.9998, 0.9990, 0.9983, 0.9971, 0.9964, 0.9973, 1.0000, 1.0052, 1.0559, 1.1422, 1.2480, 1.3629 };
        static const double z_he_table[] = { 1.0005, 1.0024, 1.0048, 1.0096, 1.0191, 1.0286, 1.0381, 1.0476, 1.0943, 1.1402, 1.1854, 1.2297 };
        static const int table_size = sizeof(pressure_bar_table) / sizeof(double);

        // Assuming pressure_t has an 'mbar' member. Convert to double in bar.
        double pressure_bar = pressure.mbar / 1000.0;

        // Find the pressure interval in the table
        int i = 0;
        while (i < table_size - 1 && pressure_bar_table[i + 1] < pressure_bar) {
            i++;
        }

        // Handle cases where pressure is outside the table range by clamping
        if (i >= table_size - 1) {
            i = table_size - 2;
        }

        // Interpolate the Z-factor for each gas, x1, y1, x2, y2, target_x
                        //                        x1, y1, z1, x2, y2, z2, target_x, target_y
        double z_he = linear_interpolate_2d(pressure_bar_table[i], 0, z_he_table[i], pressure_bar_table[i + 1], 0, z_he_table[i + 1], pressure_bar, 0);
        double z_n2 = linear_interpolate_2d(pressure_bar_table[i], 0, z_n2_table[i], pressure_bar_table[i + 1], 0, z_n2_table[i + 1], pressure_bar, 0);
        double z_o2 = linear_interpolate_2d(pressure_bar_table[i], 0, z_o2_table[i], pressure_bar_table[i + 1], 0, z_o2_table[i + 1], pressure_bar, 0);

        // Calculate the weighted average based on gas fractions
        double he_frac = he_permille / 1000.0;
        double n2_frac = n2_permille / 1000.0;
        double o2_frac = o2_permille / 1000.0;

        return (z_he * he_frac) + (z_n2 * n2_frac) + (z_o2 * o2_frac);
    }

    /**
     * @brief Calculates the unknown volume properties of a target cylinder.
     * Assumes currentPressure, workingPressure, and either wetVolume or fillVolume are provided.
     * It uses the cylinder's *current* gas mix for compressibility calculations.
     */
    void calculate_cylinder_volumes(TargetCylinder &cylinder)
    {
        // For calculating the relationship between wet volume and fill volume,
        // assume ideal gas behavior (Z=1.0) as these are often rated under ideal conditions.
        const double z_ideal = 1.0;

        // If wetVolume is unknown, calculate it from fillVolume and target pressure.
        // The relationship is: fillVolume = (workingPressure / P_atm) * wetVolume
        if (cylinder.wetVolume.mliter <= 0 && cylinder.fillVolume.mliter > 0) {
            cylinder.wetVolume.mliter = (cylinder.fillVolume.mliter / cylinder.targetPressure.mbar * z_ideal * ATM) ;
        }
        // If fillVolume is unknown, calculate it from wetVolume.
        else if (cylinder.fillVolume.mliter <= 0 && cylinder.wetVolume.mliter > 0) {
            double real_z = get_compressibility_factor(
                cylinder.targetO2_permille,
                cylinder.targetHe_permille,
                1000 - cylinder.targetO2_permille - cylinder.targetHe_permille,
                cylinder.targetPressure
            );
            //Add one ATM to account for the atmospheric pressure, to convert to gauge pressure
            cylinder.fillVolume.mliter = ((cylinder.targetPressure.mbar + ATM) / ATM) * (cylinder.wetVolume.mliter / real_z);       
        }

        // Now, calculate the actual current volume of gas using the REAL compressibility at the current pressure.
        cylinder.currentN2_permille = 1000 - cylinder.currentHe_permille - cylinder.currentO2_permille;
        pressure_t current_pres_abs;
        current_pres_abs.mbar = cylinder.currentPressure.mbar + ATM;
        pressure_t atm_pressure;
        atm_pressure.mbar = ATM;
        double z_atm = get_compressibility_factor(
            cylinder.currentO2_permille,
            cylinder.currentHe_permille,
            cylinder.currentN2_permille,
            atm_pressure
        );
        double z_current = get_compressibility_factor(
            cylinder.currentO2_permille,
            cylinder.currentHe_permille,
            cylinder.currentN2_permille,
            current_pres_abs);

        // The relationship is: currentVolume = (currentPressure / P_atm) * wetVolume / z_current
        if (cylinder.wetVolume.mliter > 0) {
            cylinder.currentVolume.mliter = z_atm * (current_pres_abs.mbar / ATM) * (cylinder.wetVolume.mliter / z_current);
            if (cylinder.currentVolume.mliter < cylinder.wetVolume.mliter) {
                cylinder.currentVolume.mliter = cylinder.wetVolume.mliter;
            }
        }
    }

    /**
     * @brief Calculates the intersection of two lines, with x1, y1, x2, and y2 being the endpoints of line 1, and x3, y3, x4, and y4 being the endpoints of line 2
     * Return true if there is an intersection, false otherwise. Allow X3, x4, Y3, and Y4 to be equal points to determine if that point is on the first line
     * */
    bool calculateIntersection(double x1, double y1, double x2, double y2,
                            double x3, double y3, double x4, double y4,
                            double& out_x, double& out_y) {

        // A small epsilon value for floating-point comparisons
        const double EPS = 1e-9;

        // --- Handle the special case: Line 2 is a single point ---
        // Check if (x3, y3) and (x4, y4) are effectively the same point.
        if (std::abs(x3 - x4) < EPS && std::abs(y3 - y4) < EPS) {
            
            // Calculate the cross product for (p1, p2, p3)
            // (y2 - y1) * (x3 - x1) - (x2 - x1) * (y3 - y2)
            double cross_product = (y2 - y1) * (x3 - x1) - (x2 - x1) * (y3 - y1);

            // 1. Check if the point (x3, y3) is collinear with segment 1
            if (std::abs(cross_product) > EPS) {
                // Point is not on the infinite line, so it can't be on the segment
                return false;
            }

            // 2. Check if the point is within the bounding box of segment 1
            // This ensures the point is on the segment, not just the infinite line.
            double min_x = std::min(x1, x2);
            double max_x = std::max(x1, x2);
            double min_y = std::min(y1, y2);
            double max_y = std::max(y1, y2);

            bool on_segment = (x3 >= min_x - EPS && x3 <= max_x + EPS &&
                            y3 >= min_y - EPS && y3 <= max_y + EPS);

            if (on_segment) {
                out_x = x3;
                out_y = y3;
                return true;
            } else {
                // Point is collinear but outside the segment
                return false;
            }
        }

        // --- General Case: Two line segments ---
        // Based on solving a system of 2 linear equations:
        // P1 + t * (P2 - P1) = P3 + u * (P4 - P3)
        
        // Calculate deltas for line 1
        double dx1 = x2 - x1;
        double dy1 = y2 - y1;

        // Calculate deltas for line 2
        double dx2 = x4 - x3;
        double dy2 = y4 - y3;

        // Calculate the denominator. If 0, lines are parallel.
        // D = (dy1 * dx2) - (dx1 * dy2)
        double D = (dy1 * dx2) - (dx1 * dy2);

        // Check if lines are parallel (denominator is close to 0)
        if (std::abs(D) < EPS) {
            // This also handles collinear, non-overlapping segments.
            // Note: Collinear, *overlapping* segments are not fully handled
            // as they have infinite intersection points, and this function
            // is designed to return one.
            return false;
        }

        // Calculate the numerators for t and u
        double t_num = (y3 - y1) * dx2 - (x3 - x1) * dy2;
        double u_num = (y3 - y1) * dx1 - (x3 - x1) * dy1;

        // Calculate t and u. These are parameters [0, 1] indicating
        // where the intersection lies on each line segment.
        double t = t_num / D;
        double u = u_num / D;

        // Check if the intersection point lies on *both* segments
        // (i.e., t and u are both between 0 and 1, inclusive)
        if (t >= 0 - EPS && t <= 1 + EPS && u >= 0 - EPS && u <= 1 + EPS) {
            // Intersection found. Calculate the point.
            out_x = x1 + t * dx1;
            out_y = y1 + t * dy1;
            return true;
        }

        // The infinite lines intersect, but the segments do not.
        return false;
    }

    /**
     * @brief handle the blend objects that need to be 2D/3D interpolated to a particular point
     */
    Blend* interpolateBlends(const Blend* b1, const Blend* b2, const Blend* b3, double x, double y) {
        
        //It is required for this function to function for these blends to have steps in the same order
        std::vector<GasSource> sources;
        for (auto step : b1->steps) {
            sources.push_back(step.source);
        }
        for (auto step : b2->steps) {
            bool found = false;
            for (auto source : sources) {
                if (source.cylinder_number == step.source.cylinder_number) {
                    found = true;
                    break;
                }
            }
            if(!found) {
                sources.push_back(step.source);

            }
        }
        if (b3 != nullptr)
        {
            for (auto step : b3->steps) {
                bool found = false;
                for (auto source : sources) {
                    if (source.cylinder_number == step.source.cylinder_number) {
                        found = true;
                        break;
                    }
                }
                if(!found) {
                    sources.push_back(step.source);

                }
            }
        }
        //Create the final blend object to return
        Blend *finalBlend = new Blend();
        finalBlend->target = b1->target;

        int max_cap = b1->currentVolume();
        if (b2->currentVolume() > max_cap) {
            max_cap = b2->currentVolume();
        }
        if (b3 != nullptr && b3->currentVolume() > max_cap) {
            max_cap = b3->currentVolume();
        }

        double blend1_x = b1->neededO2BlendInterpolation(x, max_cap);
        double blend1_y = b1->neededHeBlendInterpolation(y, max_cap);
        double blend2_x = b2->neededO2BlendInterpolation(x, max_cap);
        double blend2_y = b2->neededHeBlendInterpolation(y, max_cap);
        double blend3_x = 0.0;
        double blend3_y = 0.0;
        if (b3 != nullptr) {
            blend3_x = b3->neededO2BlendInterpolation(x, max_cap);
            blend3_y = b3->neededHeBlendInterpolation(y, max_cap);
        }


        double value1 = b1->removedGasVolume;
        double value2 = b2->removedGasVolume;
        double value3 = 0.0;
        if (b3 != nullptr) 
        {
            value3 = b3->removedGasVolume;
            finalBlend->removedGasVolume = triangular_interpolate(blend1_x, blend1_y, value1, blend2_x, blend2_y, value2, blend3_x, blend3_y, value3, x, y);

        }
        else
        {
            finalBlend->removedGasVolume = linear_interpolate_2d(blend1_x, blend1_y, value1, blend2_x, blend2_y, value2, x, y);
        }

        for (auto source : sources) {
            double value1 = 0.0;
            double value2 = 0.0;
            double value3 = 0.0;
            double value = 0.0;
            for (auto step : b1->steps) {
                if (step.source.cylinder_number == source.cylinder_number) {
                    value1 = step.volume.mliter;
                }
            }
            for (auto step : b2->steps) {
                if (step.source.cylinder_number == source.cylinder_number) {
                    value2 = step.volume.mliter;
                }
            }
            if (b3 != nullptr)
            {
                for (auto step : b3->steps) {
                    if (step.source.cylinder_number == source.cylinder_number) {
                        value3 = step.volume.mliter;
                    }
                }
                value = triangular_interpolate(blend1_x, blend1_y, value1, blend2_x, blend2_y, value2, blend3_x, blend3_y, value3, x, y);
            }
            else
            {
                value = linear_interpolate_2d(blend1_x, blend1_y, value1, blend2_x, blend2_y, value2, x, y);
            }
            BlendStep step;
            step.source = source;
            step.volume.mliter = value;
            finalBlend->steps.push_back(step);
        }

        return finalBlend;
    }

    /**
     * @brief Calculate the steps needed to blend the sources to a target mix and volume
     * NOTE: Hitting exactly on an edge is difficult to do, so we assume we are within the line or triangle
     *       before entering this function. We validate at the end of blend calculation, so this is ok
     */
    std::vector<BlendStep> calculateSourceMix(GasSource source1, GasSource source2, const GasSource* source3, double targetO2, double targetHe, double targetVolume) {
        std::vector<BlendStep> steps;// Inside calculateSourceMix (2 source case)

        //If there are only 2 sources, we can just balance the two
        if (source3 == nullptr) {
            
            double source1_x = source1.o2_permille / 1000.0;
            double source1_y = source1.he_permille / 1000.0;

            double source2_x = source2.o2_permille / 1000.0;
            double source2_y = source2.he_permille / 1000.0;

            double percent_1 = linear_interpolate_2d(source1_x, source1_y, 100.0, source2_x, source2_y, 0.0, targetO2, targetHe);
            double percent_2 = 100.0 - percent_1;

            BlendStep step1;
            step1.source = source1;
            step1.volume.mliter = percent_1 * targetVolume / 100.0;  
            steps.push_back(step1);

            BlendStep step2;
            step2.source = source2;
            step2.volume.mliter = percent_2 * targetVolume / 100.0;
            steps.push_back(step2);
            return steps;
        }
        bool validPoint = pointInTriangle(source1.o2_permille, source1.he_permille, source2.o2_permille, source2.he_permille, source3->o2_permille, source3->he_permille, targetO2*1000, targetHe*1000);
        
        //Now we need to use triangular interpolation to handle this, since we have 3 sources
        double source1_x = source1.o2_permille / 1000.0;
        double source1_y = source1.he_permille / 1000.0;

        double source2_x = source2.o2_permille / 1000.0;
        double source2_y = source2.he_permille / 1000.0;

        double source3_x = source3->o2_permille / 1000.0;
        double source3_y = source3->he_permille / 1000.0;

        double percent_1 = triangular_interpolate(source1_x, source1_y, 100.0, source2_x, source2_y, 0.0, source3_x, source3_y, 0.0, targetO2, targetHe);
        double percent_2 = triangular_interpolate(source1_x, source1_y, 0.0, source2_x, source2_y, 100.0, source3_x, source3_y, 0.0, targetO2, targetHe);
        double percent_3 = 100.0 - percent_1 - percent_2;

        BlendStep step1;
        step1.source = source1;
        step1.volume.mliter = percent_1 * targetVolume / 100.0;  
        steps.push_back(step1);

        BlendStep step2;
        step2.source = source2;
        step2.volume.mliter = percent_2 * targetVolume / 100.0;
        steps.push_back(step2);

        BlendStep step3;
        step3.source = *source3; // Dereferenced source3
        step3.volume.mliter = percent_3 * targetVolume / 100.0;
        steps.push_back(step3);

        return steps;
    }

    /**
     * @brief Helper function to determine the "sign" of a point relative to a line segment.
     *
     * This calculates a value related to the cross product. The sign of the result
     * indicates which side of the line (p1 -> p2) the point (p3) is on.
     */
    double sign(double x1, double y1, double x2, double y2, double x3, double y3) {
        return (x1 - x3) * (y2 - y3) - (x2 - x3) * (y1 - y3);
    }

    /**
     * @brief Determine if a point (tx, ty) is within a triangle formed by the points (x1, y1), (x2, y2), (x3, y3).
     *
     * This check is inclusive, meaning points lying exactly on the edges of the triangle
     * will also return true.
     */
    bool pointInTriangle(double x1, double y1, double x2, double y2, double x3, double y3, double tx, double ty) {
        
        // 1. Standard Half-Plane / Barycentric check
        double s1 = sign(x1, y1, x2, y2, tx, ty);
        double s2 = sign(x2, y2, x3, y3, tx, ty);
        double s3 = sign(x3, y3, x1, y1, tx, ty);

        bool all_positive_or_zero = (s1 >= 0) && (s2 >= 0) && (s3 >= 0);
        bool all_negative_or_zero = (s1 <= 0) && (s2 <= 0) && (s3 <= 0);

        // If the point is outside the half-planes, return false immediately
        if (!(all_positive_or_zero || all_negative_or_zero)) {
            return false;
        }

        // 2. EDGE CASE: Degenerate Triangle (Collinear points)
        // Calculate 2x the Area of the triangle
        double area2 = std::abs((x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1));

        // If area is effectively zero, the "triangle" is a line segment.
        // The previous check only confirmed the point is on the INFINITE line.
        // We must now verify it is within the bounds of the segments.
        if (area2 < 1e-9) {
            double min_x = std::min({x1, x2, x3});
            double max_x = std::max({x1, x2, x3});
            double min_y = std::min({y1, y2, y3});
            double max_y = std::max({y1, y2, y3});

            const double EPS = 1e-9;
            bool inside_x = tx >= min_x - EPS && tx <= max_x + EPS;
            bool inside_y = ty >= min_y - EPS && ty <= max_y + EPS;

            return inside_x && inside_y;
        }

        return true;
    }

    std::vector<Blend> handleDegenerateTriangle(const Blend& blend1, const Blend& blend2, const Blend& blend3, const GasSource& source) {
        std::vector<Blend> blends;
        double x1 = blend1.neededO2() / 1000.0;
        double y1 = blend1.neededHe() / 1000.0;
        double x2 = blend2.neededO2() / 1000.0;
        double y2 = blend2.neededHe() / 1000.0;
        double x3 = blend3.neededO2() / 1000.0;
        double y3 = blend3.neededHe() / 1000.0;

        double area2 = std::abs((x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1));

        // If area is effectively zero, the "triangle" is a line segment.
        // The previous check only confirmed the point is on the INFINITE line.
        // We must now verify it is within the bounds of the segments.
        if (!(area2 < 1e-9)) {
            return blends; // Not a degenerate triangle
        }
        Blend *b1 = calculate_blend(&blend1, &blend2, nullptr, &source, nullptr, nullptr);
        Blend *b2 = calculate_blend(&blend1, &blend3, nullptr, &source, nullptr, nullptr);
        Blend *b3 = calculate_blend(&blend3, &blend2, nullptr, &source, nullptr, nullptr);
        if(b1 != nullptr) {
            blends.push_back(*b1);
        }
        if(b2 != nullptr) {
            blends.push_back(*b2);
        }
        if(b3 != nullptr) {
            blends.push_back(*b3);
        }
        return blends;
    }

    /**
     * @brief Calculate the blend. This is the primary operating function, taking in multiple Gas Sources and Blend objects
     * * This function takes in up to 3 Blend objects and 3 Gas Sources. It then changes what it does based on how many items are passed into it
     * To start, for simple blending, only 2 blend objects can be passed in. The blend operation proceeds as follows:
     * 0. Build a point, line, or triangle depending on the number of Gas Sources passed in, based on gas mix. All sources here are unlimited 
     * (limited ones are passed in only as part of the blend object)
     * 1. If only one blend object is passed in, we have our "point of interest"
     * 2. If two blend objects are passed in, we need to find our points of interest
     * These points are: the intersection of the line formed with the triangle or line formed in 0 or the actual Blend point (neededO2 and neededHe) if that is inside the triangle/line
     * 3. If there is a point of interest that is NOT already a blend, we need to use triangular or linear interpolation to build that blend object.
     * 4. If the point is within the triangle, use triangular interpolation to get the volume of each of the gasses needed to fill the tank to the needed mix
     * 5. If the point is on an edge in the triangle, use linear interpolation instead
     * 6. If the point is a point of the triangle, top off with that gas
     * * Return the blend object after these operations are added to it
     */
    Blend* calculate_blend(const Blend* blend1, const Blend* blend2, const Blend* blend3, const GasSource* source1, const GasSource* source2, const GasSource* source3){

        int numBlends = 0;
        //determine how many of blend1-3 we actually have
        if (blend1 != nullptr) {
            numBlends++;
        }
        if (blend2 != nullptr) {
            numBlends++;
        }
        if (blend3 != nullptr) {
            numBlends++;
        }

        //Make an empty vector of points of interest
        std::vector<Blend> pointsOfInterest;
        
        int numSources = 0;
        //determine how many of source1-3 we actually have
        if (source1 != nullptr) {
            numSources++;
        }
        if (source2 != nullptr) {
            numSources++;
        }
        if (source3 != nullptr) {
            numSources++;
        }

        //We need to account for 1 source 1 blend, 1 source 2 blends, 2 sources 1 blend, 2 sources 2 blends, and 3 sources 2 blends
        if (numSources == 1 && numBlends == 1) {
            //We only have a single option, so see if this works and return it if it does
            //Create the blend step based on needed volume in the target cylinder and the source available
            Blend* result = new Blend(*blend1);

            BlendStep step;
            step.source = *source1;
            step.volume.mliter = result->target.fillVolume.mliter - result->target.currentVolume.mliter + result->removedGasVolume;
            result->steps.push_back(step);

            if(result->validate(false)) {
                return result;
            } else {
                delete result;
                return nullptr;//Return null if this does not work, since it was the only option
            }
        } else if(numSources == 2 && numBlends == 1) {
            //We need to see if the blend is on the line formed by the two sources
            double blend_x = blend1->neededO2(); 
            double blend_y = blend1->neededHe();

            double source1_x = source1->o2_permille / 1000.0;
            double source1_y = source1->he_permille / 1000.0;

            double source2_x = source2->o2_permille / 1000.0;
            double source2_y = source2->he_permille / 1000.0;

            double intersection_x;
            double intersection_y;

            if(calculateIntersection(source1_x, source1_y, source2_x, source2_y, blend_x/1000.0, blend_y/1000.0, blend_x/1000.0, blend_y/1000.0, intersection_x, intersection_y)) {
                //The blend is on the line formed by the two sources
                //Linear interpolate between the two sources to get % how much of each gas to add to the target cylinder
                //This is going to go from 0 to 100 of one gas, which the remainder would be the other gas
                Blend* result = new Blend(*blend1);

                std::vector<BlendStep> steps;
                steps = calculateSourceMix(*source1, *source2, nullptr, blend_x/1000.0, blend_y/1000.0, result->target.fillVolume.mliter - result->target.currentVolume.mliter + result->removedGasVolume);

                for(const BlendStep& step : steps) {
                    result->steps.push_back(step);
                }
                if(result->validate(false)) {
                    return result;
                } else {
                    delete result;
                    return nullptr;//Return null if this does not work, since it was the only option
                }
            } else {
                return nullptr; //We cannot make this blend work, so return null
            }
        } else if (numSources == 3 && numBlends == 1) {
            double blend1_x = blend1->neededO2()/1000.0;
            double blend1_y = blend1->neededHe()/1000.0;

            double source1_x = source1->o2_permille / 1000.0;
            double source1_y = source1->he_permille / 1000.0;

            double source2_x = source2->o2_permille / 1000.0;
            double source2_y = source2->he_permille / 1000.0;

            double source3_x = source3->o2_permille / 1000.0;
            double source3_y = source3->he_permille / 1000.0; 

            if(!pointInTriangle(source1_x, source1_y, source2_x, source2_y, source3_x, source3_y, blend1_x, blend1_y)) {
                return nullptr;
            }
            vector<BlendStep> stepsToAdd;
            Blend *finalBlend = new Blend(*blend1);
            stepsToAdd = calculateSourceMix(*source1, *source2, source3, blend1->neededO2()/1000.0, blend1->neededHe()/1000.0, blend1->target.fillVolume.mliter - blend1->currentVolume());
            for(auto step : stepsToAdd) {
                finalBlend->steps.push_back(step);
            }
            if(finalBlend->validate(false)) {
                //return blend1;
                return finalBlend;
            }
            return finalBlend;
            
        
        } else if (numSources == 1 && numBlends == 2) {
            //We need to see if the source is on the line formed by the two blends
            double source_x = source1->o2_permille / 1000.0;
            double source_y = source1->he_permille / 1000.0;

            double blend1_x = blend1->neededO2() / 1000.0;
            double blend1_y = blend1->neededHe() / 1000.0;

            double blend2_x = blend2->neededO2() / 1000.0;
            double blend2_y = blend2->neededHe() / 1000.0;

            double intersection_x;
            double intersection_y;
                      
            if(calculateIntersection(blend1_x, blend1_y, blend2_x, blend2_y, source_x, source_y, source_x, source_y, intersection_x, intersection_y)) {
                //The source is on the line formed by the two blends
                //Need to figure out which item in the blend is different between the two
                Blend* targetBlend = interpolateBlends(blend1, blend2, nullptr, source1->o2_permille, source1->he_permille);
                if (!targetBlend) return nullptr;
                BlendStep step;
                step.source = *source1;
                //Check the following
                step.volume.mliter = targetBlend->target.fillVolume.mliter - targetBlend->currentVolume();
                targetBlend->steps.push_back(step);

                if(targetBlend->validate(false)) {
                    return targetBlend;
                } else {
                    delete targetBlend;
                    return nullptr;
                }
            } else {
                return nullptr;
            }
        } else if (numSources == 2 && numBlends == 2){
            //Determine if the two sources and 2 blends are intersecting
            double source1_x = source1->o2_permille / 1000.0;
            double source1_y = source1->he_permille / 1000.0;

            double source2_x = source2->o2_permille / 1000.0;
            double source2_y = source2->he_permille / 1000.0;

            double blend1_x = blend1->neededO2() / 1000.0;
            double blend1_y = blend1->neededHe() / 1000.0;

            double blend2_x = blend2->neededO2() / 1000.0;
            double blend2_y = blend2->neededHe() / 1000.0;

            double intersection_x;
            double intersection_y;

            Blend *finalBlend;
            if(calculateIntersection(blend1_x, blend1_y, blend2_x, blend2_y, source1_x, source1_y, source1_x, source1_y, intersection_x, intersection_y)) {
                finalBlend = interpolateBlends(blend1, blend2, nullptr, intersection_x * 1000.0, intersection_y * 1000.0);
                vector<BlendStep> stepsToAdd;
                stepsToAdd = calculateSourceMix(*source1, *source2, nullptr, intersection_x, intersection_y, finalBlend->target.fillVolume.mliter - finalBlend->currentVolume());
                for (auto step : stepsToAdd) {

                    finalBlend->steps.push_back(step);
                }
                if(!finalBlend->validate(false)) {
                    delete finalBlend;
                }
            }
            if(calculateIntersection(blend1_x, blend1_y, blend2_x, blend2_y, source2_x, source2_y, source2_x, source2_y, intersection_x, intersection_y)) {
                Blend* finalBlend2 = interpolateBlends(blend1, blend2, nullptr, intersection_x * 1000.0, intersection_y * 1000.0);
                vector<BlendStep> stepsToAdd;
                stepsToAdd = calculateSourceMix(*source1, *source2, nullptr, intersection_x, intersection_y, finalBlend2->target.fillVolume.mliter - finalBlend2->currentVolume());
                for (auto step : stepsToAdd) {

                    finalBlend2->steps.push_back(step);
                }
                if(!finalBlend2->validate(false)) {
                    delete finalBlend2;
                }
                if (finalBlend && finalBlend2) {
                    if (finalBlend2->cost() < finalBlend->cost()) {
                        delete finalBlend2;
                        return finalBlend;
                    }
                    else {
                        delete finalBlend;
                        return finalBlend2;
                    }
                }
                if (finalBlend) {
                    return finalBlend;
                }
                return finalBlend2;
            }
            if(calculateIntersection(blend1_x, blend1_y, blend2_x, blend2_y, source1_x, source1_y, source2_x, source2_y, intersection_x, intersection_y)) {
                //The line between the two sources intersect the line formed by the two blends
                //Need to figure out which item in the blend is different between the two
                finalBlend = interpolateBlends(blend1, blend2, nullptr, intersection_x * 1000.0, intersection_y * 1000.0);
                if (!finalBlend) return nullptr;
                // Now determine how much of each source is needed, and add that vector<BlendSteps> to the final blend
                vector<BlendStep> stepsToAdd;
                stepsToAdd = calculateSourceMix(*source1, *source2, nullptr, intersection_x, intersection_y, finalBlend->target.fillVolume.mliter - finalBlend->currentVolume());
                for (auto step : stepsToAdd) {

                    finalBlend->steps.push_back(step);
                }
                if(finalBlend->validate(false)) {
                    return finalBlend;
                } else {
                    delete finalBlend;
                    return nullptr;
                }
            } else {
                return nullptr;
            }
        } else if (numSources == 3 && numBlends == 2) {
            //Find the points of interest. This is any intersection between the two blends and the triangle formed by the three sources
            // or the two blends themselves, if they are inside the triangle.

            //For each blend, if the neededO2,neededHe point is in the triangle formed by the sources, add that blend to points of interest

            //First blend
            double blend1_x = blend1->neededO2() / 1000.0;
            double blend1_y = blend1->neededHe() / 1000.0;

            double source1_x = source1->o2_permille / 1000.0;
            double source1_y = source1->he_permille / 1000.0;

            double source2_x = source2->o2_permille / 1000.0;
            double source2_y = source2->he_permille / 1000.0;

            double source3_x = source3->o2_permille / 1000.0;
            double source3_y = source3->he_permille / 1000.0; 

            if(pointInTriangle(source1_x, source1_y, source2_x, source2_y, source3_x, source3_y, blend1_x, blend1_y)) {
                pointsOfInterest.push_back(*blend1);
            }
            //Second blend
            double blend2_x = blend2->neededO2() / 1000.0;
            double blend2_y = blend2->neededHe() / 1000.0;

            if(pointInTriangle(source1_x, source1_y, source2_x, source2_y, source3_x, source3_y, blend2_x, blend2_y)) {
                pointsOfInterest.push_back(*blend2);
            }
            //Now find the intersection between the two blends and each of the three lines
            double intersection_x;
            double intersection_y;
            
            if(calculateIntersection(blend1_x, blend1_y, blend2_x, blend2_y, source1_x, source1_y, source2_x, source2_y, intersection_x, intersection_y)) {
                Blend* temp = interpolateBlends(blend1, blend2, nullptr, intersection_x * 1000.0, intersection_y * 1000.0);
                if (temp) {
                    pointsOfInterest.push_back(*temp);
                    delete temp;
                }
            }
            if(calculateIntersection(blend1_x, blend1_y, blend2_x, blend2_y, source2_x, source2_y, source3_x, source3_y, intersection_x, intersection_y)) {
                Blend* temp = interpolateBlends(blend1, blend2, nullptr, intersection_x * 1000.0, intersection_y * 1000.0);
                if (temp) {
                    pointsOfInterest.push_back(*temp);
                    delete temp;
                }
            }
            if(calculateIntersection(blend1_x, blend1_y, blend2_x, blend2_y, source3_x, source3_y, source1_x, source1_y, intersection_x, intersection_y)) {
                Blend* temp = interpolateBlends(blend1, blend2, nullptr, intersection_x * 1000.0, intersection_y * 1000.0);
                if (temp) {
                    pointsOfInterest.push_back(*temp);
                    delete temp;
                }
            }
            //For each point of interest, finish the blend calculation and find the cost of the blend. Keep the cheapest blend
            if (pointsOfInterest.empty()) return nullptr;
            Blend bestBlend = pointsOfInterest[0];
            bool valid = false;
            for(auto &point : pointsOfInterest) {
                vector<BlendStep> stepsToAdd;
                stepsToAdd = calculateSourceMix(*source1, *source2, source3, point.neededO2() / 1000.0, point.neededHe() / 1000.0, point.target.fillVolume.mliter - point.currentVolume());
                for (auto step : stepsToAdd) {
                    point.steps.push_back(step);
                }
                if(point.validate(false)) {
                    if(!valid ||point.cost() < bestBlend.cost()) {
                        bestBlend = point;
                        valid = true;
                    }
                }
            }
            
            Blend* finalResult = new Blend(bestBlend);
            if(finalResult->validate(false)) {
                return finalResult;
            } else {
                delete finalResult;
                return nullptr;
            }
        } else if (numSources == 1 && numBlends == 3) {
            double blend1_x = blend1->neededO2() / 1000.0;
            double blend1_y = blend1->neededHe() / 1000.0;

            double blend2_x = blend2->neededO2() / 1000.0;
            double blend2_y = blend2->neededHe() / 1000.0;

            double blend3_x = blend3->neededO2() / 1000.0;
            double blend3_y = blend3->neededHe() / 1000.0;

            double source1_x = source1->o2_permille / 1000.0;
            double source1_y = source1->he_permille / 1000.0;

            std::vector<Blend> degPoints = handleDegenerateTriangle(*blend1, *blend2, *blend3, *source1);
            if(!degPoints.empty()) {
                vector<Blend> results;
                for (Blend point : degPoints) {

                    BlendStep step;
                    step.source = *source1;
                    step.volume.mliter = point.target.fillVolume.mliter - point.currentVolume();
                    point.steps.push_back(step);
                    if(point.validate(false)) {
                        //save blend;
                        results.push_back(point);
                    }
                }
                if (results.size() > 0) {
                    Blend *cheapest = &results[0];
                    for (int i = 1; i < results.size(); i++) {
                        if (results[i].cost() < cheapest->cost()) {
                            cheapest = &results[i];
                        }
                    }
                    return new Blend(*cheapest);
                }
            } 

            if(pointInTriangle(blend1_x, blend1_y, blend2_x, blend2_y, blend3_x, blend3_y, source1_x, source1_y)) {
                Blend* temp = interpolateBlends(blend1, blend2, blend3, source1_x * 1000.0, source1_y * 1000.0);
                if (temp) {
                    BlendStep step;
                    step.source = *source1;
                    step.volume.mliter = temp->target.fillVolume.mliter - temp->currentVolume();
                    temp->steps.push_back(step);
                    if (temp->validate(false)) {
                        return temp;
                    } else {
                        delete temp;
                        return nullptr;
                    }
                }
            }
            return nullptr;
        }
        else if (numSources == 2 && numBlends == 3) {
            double blend1_x = blend1->neededO2() / 1000.0;
            double blend1_y = blend1->neededHe() / 1000.0;

            double blend2_x = blend2->neededO2() / 1000.0;
            double blend2_y = blend2->neededHe() / 1000.0;

            double blend3_x = blend3->neededO2() / 1000.0;
            double blend3_y = blend3->neededHe() / 1000.0;

            double source1_x = source1->o2_permille / 1000.0;
            double source1_y = source1->he_permille / 1000.0;

            double source2_x = source2->o2_permille / 1000.0;
            double source2_y = source2->he_permille / 1000.0;

            //Get the points of interest, which are the source coordinates, if inside the blend triangle
            // or any intersection between the source line and any blend lines
            std::vector<Blend> pointsOfInterest;

            if(pointInTriangle(blend1_x, blend1_y, blend2_x, blend2_y, blend3_x, blend3_y, source1_x, source1_y)) {
                Blend* temp = interpolateBlends(blend1, blend2, blend3, source1_x * 1000.0, source1_y * 1000.0);
                if (temp) {
                    pointsOfInterest.push_back(*temp);
                    delete temp;
                }
            }
            if(pointInTriangle(blend1_x, blend1_y, blend2_x, blend2_y, blend3_x, blend3_y, source2_x, source2_y)) {
                Blend* temp = interpolateBlends(blend1, blend2, blend3, source2_x * 1000.0, source2_y * 1000.0);
                if (temp) {

                    pointsOfInterest.push_back(*temp);
                    delete temp;
                }
            }

            //Handle the case where the triangle is degenerate

            double intersection_x = 0.0;
            double intersection_y = 0.0;
            if(calculateIntersection(blend1_x, blend1_y, blend2_x, blend2_y, source1_x, source1_y, source2_x, source2_y, intersection_x, intersection_y)) {
                Blend* temp = interpolateBlends(blend1, blend2, blend3, intersection_x * 1000.0, intersection_y * 1000.0);
                if (temp) {
                    pointsOfInterest.push_back(*temp);
                    delete temp;
                }
            }
            if(calculateIntersection(blend1_x, blend1_y, blend3_x, blend3_y, source1_x, source1_y, source2_x, source2_y, intersection_x, intersection_y)) {
                Blend* temp = interpolateBlends(blend1, blend2, blend3, intersection_x * 1000.0, intersection_y * 1000.0);
                if (temp) {
                    pointsOfInterest.push_back(*temp);
                    delete temp;
                }
            }
            if(calculateIntersection(blend3_x, blend3_y, blend2_x, blend2_y, source1_x, source1_y, source2_x, source2_y, intersection_x, intersection_y)) {
                Blend* temp = interpolateBlends(blend1, blend2, blend3, intersection_x * 1000.0, intersection_y * 1000.0);
                if (temp) {
                    pointsOfInterest.push_back(*temp);
                    delete temp;
                }
            }
            vector<Blend> results;
            std::vector<Blend> degPoints = handleDegenerateTriangle(*blend1, *blend2, *blend3, *source1);

            for (Blend point : degPoints) {
                results.push_back(point);
            }
            degPoints = handleDegenerateTriangle(*blend1, *blend2, *blend3, *source2);

            for (Blend point : degPoints) {
                results.push_back(point);
            }

            for (Blend point : pointsOfInterest) {
                Blend *finalBlend = new Blend(point);
                vector<BlendStep> stepsToAdd = calculateSourceMix(*source1, *source2, source3, point.neededO2()/1000.0, point.neededHe()/1000.0, point.target.fillVolume.mliter - point.currentVolume());
                for(auto step : stepsToAdd) {
                    finalBlend->steps.push_back(step);
                }
                if(finalBlend->validate(false)) {
                    //save blend;
                    results.push_back(*finalBlend);
                }
            }

            if (results.size() > 0) {
                Blend *cheapest = &results[0];
                for (int i = 1; i < results.size(); i++) {

                    if (results[i].cost() < cheapest->cost()) {
                        cheapest = &results[i];
                    }
                }
                return new Blend(*cheapest);
            }

            return nullptr;
        
        } else if (numSources == 3 && numBlends == 3) {
            //Get the points of interest, which should be the blends, if they are within the source triangle,
            //The source triangle points, if they are within the blend triangle, or any intersection between 
            //the two triangles.
            double source1_x = source1->o2_permille/1000.0;
            double source1_y = source1->he_permille/1000.0;
            double source2_x = source2->o2_permille/1000.0;
            double source2_y = source2->he_permille/1000.0;
            double source3_x = source3->o2_permille/1000.0;
            double source3_y = source3->he_permille/1000.0;

            double blend1_x = blend1->neededO2() / 1000.0;
            double blend1_y = blend1->neededHe() / 1000.0;

            double blend2_x = blend2->neededO2() / 1000.0;
            double blend2_y = blend2->neededHe() / 1000.0;

            double blend3_x = blend3->neededO2() / 1000.0;
            double blend3_y = blend3->neededHe() / 1000.0;

            std::vector<Blend> pointsOfInterest;

            if (pointInTriangle(source1_x, source1_y, source2_x, source2_y, source3_x, source3_y, blend1_x, blend1_y)) {
                pointsOfInterest.push_back(*blend1);
            }
            if (pointInTriangle(source1_x, source1_y, source2_x, source2_y, source3_x, source3_y, blend2_x, blend2_y)) {
                pointsOfInterest.push_back(*blend2);
            }
            if (pointInTriangle(source1_x, source1_y, source2_x, source2_y, source3_x, source3_y, blend3_x, blend3_y)) {
                pointsOfInterest.push_back(*blend3);
            }

            if (pointInTriangle(blend1_x, blend1_y, blend2_x, blend2_y, blend3_x, blend3_y, source1_x, source1_y)) {
                Blend* temp = interpolateBlends(blend1, blend2, blend3, source1_x * 1000.0, source1_y * 1000.0);
                if (temp) {
                    pointsOfInterest.push_back(*temp);
                    delete temp;
                }
            }

            if (pointInTriangle(blend1_x, blend1_y, blend2_x, blend2_y, blend3_x, blend3_y, source2_x, source2_y)) {
                Blend* temp = interpolateBlends(blend1, blend2, blend3, source2_x * 1000.0, source2_y * 1000.0);
                if (temp) {
                    pointsOfInterest.push_back(*temp);
                    delete temp;
                }
            }

            if (pointInTriangle(blend1_x, blend1_y, blend2_x, blend2_y, blend3_x, blend3_y, source3_x, source3_y)) {
                Blend* temp = interpolateBlends(blend1, blend2, blend3, source3_x * 1000.0, source3_y * 1000.0);
                if (temp) {
                    pointsOfInterest.push_back(*temp);
                    delete temp;
                }
            }

            //Find the intersections of each line to find points of interest

            double temp_x = 0.0;
            double temp_y = 0.0;

            if (calculateIntersection(blend1_x, blend1_y, blend2_x, blend2_y, source1_x, source1_y, source2_x, source2_y, temp_x, temp_y)) {
                Blend* temp = interpolateBlends(blend1, blend2, blend3, temp_x * 1000.0, temp_y * 1000.0);
                if (temp) {
                    pointsOfInterest.push_back(*temp);
                    delete temp;
                }
            }

            if (calculateIntersection(blend1_x, blend1_y, blend2_x, blend2_y, source1_x, source1_y, source3_x, source3_y, temp_x, temp_y)) {
                Blend* temp = interpolateBlends(blend1, blend2, blend3, temp_x * 1000.0, temp_y * 1000.0);
                if (temp) {
                    pointsOfInterest.push_back(*temp);
                    delete temp;
                }
            }

            if (calculateIntersection(blend1_x, blend1_y, blend2_x, blend2_y, source2_x, source2_y, source3_x, source3_y, temp_x, temp_y)) {
                Blend* temp = interpolateBlends(blend1, blend2, blend3, temp_x * 1000.0, temp_y * 1000.0);
                if (temp) {
                    pointsOfInterest.push_back(*temp);
                    delete temp;
                }
            }

            if (calculateIntersection(blend2_x, blend2_y, blend3_x, blend3_y, source2_x, source2_y, source1_x, source1_y, temp_x, temp_y)) {
                Blend* temp = interpolateBlends(blend1, blend2, blend3, temp_x * 1000.0, temp_y * 1000.0);
                if (temp) {
                    pointsOfInterest.push_back(*temp);
                    delete temp;
                }
            }
            if (calculateIntersection(blend2_x, blend2_y, blend3_x, blend3_y, source1_x, source1_y, source3_x, source3_y, temp_x, temp_y)) {
                Blend* temp = interpolateBlends(blend1, blend2, blend3, temp_x * 1000.0, temp_y * 1000.0);
                if (temp) {
                    pointsOfInterest.push_back(*temp);
                    delete temp;
                }
            }
            if (calculateIntersection(blend2_x, blend2_y, blend3_x, blend3_y, source2_x, source2_y, source3_x, source3_y, temp_x, temp_y)) {
                Blend* temp = interpolateBlends(blend1, blend2, blend3, temp_x * 1000.0, temp_y * 1000.0);
                if (temp) {
                    pointsOfInterest.push_back(*temp);
                    delete temp;
                }
            }

            if (calculateIntersection(blend1_x, blend1_y, blend3_x, blend3_y, source1_x, source1_y, source3_x, source3_y, temp_x, temp_y)) {
                Blend* temp = interpolateBlends(blend1, blend2, blend3, temp_x * 1000.0, temp_y * 1000.0);
                if (temp) {
                    pointsOfInterest.push_back(*temp);
                    delete temp;
                }
            }

            if (calculateIntersection(blend1_x, blend1_y, blend3_x, blend3_y, source1_x, source1_y, source2_x, source2_y, temp_x, temp_y)) {
                Blend* temp = interpolateBlends(blend1, blend2, blend3, temp_x * 1000.0, temp_y * 1000.0);
                if (temp) {
                    pointsOfInterest.push_back(*temp);
                    delete temp;
                }
            }

            if (calculateIntersection(blend1_x, blend1_y, blend3_x, blend3_y, source2_x, source2_y, source3_x, source3_y, temp_x, temp_y)) {
                Blend* temp = interpolateBlends(blend1, blend2, blend3, temp_x * 1000.0, temp_y * 1000.0);
                if (temp) {
                    pointsOfInterest.push_back(*temp);
                    delete temp;
                }
            }

            vector<Blend> results;
            std::vector<Blend> degBlends = handleDegenerateTriangle(*blend1, *blend2, *blend3, *source1);

            for (Blend blend : degBlends) {
                results.push_back(blend);
            }

            degBlends = handleDegenerateTriangle(*blend1, *blend2, *blend3, *source2);

            for (Blend blend : degBlends) {
                results.push_back(blend);
            }

            degBlends = handleDegenerateTriangle(*blend1, *blend2, *blend3, *source3);

            for (Blend blend : degBlends) {
                results.push_back(blend);
            }
            for (Blend point : pointsOfInterest) {
                Blend *finalBlend = new Blend(point);
                vector<BlendStep> stepsToAdd = calculateSourceMix(*source1, *source2, source3, point.neededO2()/1000.0, point.neededHe()/1000.0, point.target.fillVolume.mliter - point.currentVolume());
                for(auto step : stepsToAdd) {
                    finalBlend->steps.push_back(step);
                }
                if(finalBlend->validate(false)) {
                    //save blend;
                    results.push_back(*finalBlend);
                }
            }
            if (results.size() > 0) {
                Blend *cheapest = &results[0];
                for (int i = 1; i < results.size(); i++) {
                    if (results[i].cost() < cheapest->cost()) {
                        cheapest = &results[i];
                    }
                }
                return new Blend(*cheapest);
            }

            return nullptr;
            
            
        } else {
            return nullptr; //This should never happen
        }
                    
    }


    /**
     * @brief Creates a vector of 3 blend objects per entry in the vector. This converts a vector of GasSources that
     *        are limited in pressure/volume into the correct exhaustive set of blend objects to pass into calculateBlends
     * 
     *      This works by iterating over all of the GasSources and adding each one in all places to the held vector of 
     *      blends steps. For example: The base case has 2 Blend Objects, one at the starting pressure, one with all
     *      gas removed (down to wet volume), assuming it was not already at wet volume (if it were, only 1 case exists in the base).
     * 
     *      Then for the first gas source, another two Blend objects are added, each one with the max pressure that can be
     *      reached with that gas source. Then for the second gas source, another six Blend objects are added, one to each of the
     *      existing blend objects at the end (as a new step) and two more as the step before source 1. The next gas now needs 2 + 6 + 6 + 8 = 22 cases added 
     */
    std::vector<BlendTriangle> generateExhaustiveBlends(const TargetCylinder& target, const std::vector<GasSource>& limitedSources) {
        std::vector<Blend> allBlends;
        std::vector<BlendTriangle> allTriangles;
        Blend baseBlend;
        baseBlend.target = target;
        baseBlend.removedGasVolume = 0.0;

        // --- 1. Establish Base Cases ---
        
        // Case A: The blend as it currently exists (start with current pressure)
        allBlends.push_back(baseBlend);
        bool noRemovedGas = true;
        // Case B: The blend assuming we drain it first (start with wet volume / 1 atm)
        // We only add this if the tank isn't already empty, to avoid duplicates
        if (baseBlend.target.currentVolume.mliter > baseBlend.target.wetVolume.mliter + 1.0) { // +1.0 for float epsilon safety
            Blend emptyBlend = baseBlend;
            // We set the removed gas to equal the current volume minus the wet volume
            // effectively simulating a drain down to 1 ATM
            emptyBlend.removedGasVolume = baseBlend.target.currentVolume.mliter - baseBlend.target.wetVolume.mliter;
            allBlends.push_back(emptyBlend);
            noRemovedGas = false;
        }

        // --- 2. Iterate over limited sources ---
        int numLimitedSources = 0;
        for (const auto& source : limitedSources) {
            if (source.unlimited) {
                continue;
            }
            numLimitedSources++;
            // We need a temporary vector to store the new permutations generated in this pass.
            // We cannot modify 'allBlends' while iterating over it.
            std::vector<Blend> newPermutations;

            // For every blend configuration we have generated so far...
            for (const auto& existingBlend : allBlends) {
                
                // ...Try inserting the new source at every possible position in the steps.
                // If there are N steps, there are N+1 positions (Index 0 to N).
                // Example: Steps [A, B]. We can insert C at:
                // 0: [C, A, B]
                // 1: [A, C, B]
                // 2: [A, B, C]

                for (size_t i = 0; i <= existingBlend.steps.size(); ++i) {
                    Blend newBlend = existingBlend; // Copy the existing blend
                    
                    BlendStep newStep;
                    newStep.source = source;
                    
                    // Since this is a limited source, we default to trying to add 
                    // the entire available volume of the source.
                    // The validate() function or solver will later check if this overfills the target.
                    newStep.volume.mliter = source.currentVolume.mliter;

                    // Insert the step at the calculated position
                    newBlend.steps.insert(newBlend.steps.begin() + i, newStep);
                    if (newBlend.correctPressures()) {
                        newPermutations.push_back(newBlend);
                    }
                }
            }
            // Add the newly generated permutations to our master list
            // as the prompt implies "adding" to the set for the next gas to iterate over
            allBlends.insert(allBlends.end(), newPermutations.begin(), newPermutations.end());
        }

        if (allBlends.size() < 4) {
            BlendTriangle baseTriangle;
            if(allBlends.size() > 0) {
                baseTriangle.blend1 = allBlends[0];
                baseTriangle.itemCount = 1;
            }
            if(allBlends.size() > 1) {
                baseTriangle.blend2 = allBlends[1];
                baseTriangle.itemCount = 2;
            }
            if(allBlends.size() > 2) {
                baseTriangle.blend3 = allBlends[2];
                baseTriangle.itemCount = 3;
            }
            allTriangles.push_back(baseTriangle);
            return allTriangles;
        }
        std::vector<Blend> uniqueOrdering;
        for (const auto& blend : allBlends) {
            if (blend.steps.size() == numLimitedSources && (blend.removedGasVolume > 0 || noRemovedGas)) {
                uniqueOrdering.push_back(blend);
            }
        }

        std::vector<Blend> currentGroup;
        for (const auto& blend : uniqueOrdering) {
            currentGroup.clear();
            for (const auto& otherBlend : allBlends) {
                //If the gasSource ordering is the same as in blend, add this to current group
                int index = 0;
                bool valid = true;
                for (const auto& step : otherBlend.steps) {
                    if(!valid) {
                        break;
                    }
                    valid = false;
                    for(int i = index; i < blend.steps.size(); i++) {
                        if (blend.steps[i].source.cylinder_number == step.source.cylinder_number) {
                            valid = true;
                            index = i;
                            break;
                        }
                    }
                }
                if (valid) {
                    currentGroup.push_back(otherBlend);
                }
            }
            //For the current group, generate the pairs of coordinates from NeededO2 and He
            //This has been removed because this can result in missing lower cost solutions. This adds
            //a lot of computation time.

            /*
            std::vector<Point> points;
            for (const auto& blend : currentGroup) {
                Point point;
                point.first = blend.neededO2();
                point.second = blend.neededHe();
                points.push_back(point);
            }
            std::vector<Point> convexHull = get_convex_hull(points);
            std::vector<Blend> orderedBlends;
            for (const auto& point : convexHull) {
                for (const auto& blend : currentGroup) {
                    if (blend.neededO2() == point.first && blend.neededHe() == point.second) {
                        orderedBlends.push_back(blend);
                    }
                }
            }
            if (orderedBlends.size() == 1) {
                BlendTriangle triangle;
                triangle.blend1 = orderedBlends[0];
                triangle.itemCount = 1;
                allTriangles.push_back(triangle);
                continue;
            }
            if (orderedBlends.size() == 2) {
                BlendTriangle triangle;
                triangle.blend1 = orderedBlends[0];
                triangle.blend2 = orderedBlends[1];
                triangle.itemCount = 2;
                allTriangles.push_back(triangle);
                continue;
            }
            for (int i = 2; i < orderedBlends.size(); i++) {
                BlendTriangle triangle;
                triangle.blend1 = orderedBlends[0];
                triangle.blend2 = orderedBlends[i-1];
                triangle.blend3 = orderedBlends[i];
                triangle.itemCount = 3;
                allTriangles.push_back(triangle);
            }
            */

            if(currentGroup.size() < 4) {
                BlendTriangle baseTriangle;
                if(currentGroup.size() > 0) {
                    baseTriangle.blend1 = currentGroup[0];
                    baseTriangle.itemCount = 1;
                    allTriangles.push_back(baseTriangle);

                }
                if(currentGroup.size() > 1) {
                    baseTriangle.blend2 = currentGroup[1];
                    baseTriangle.itemCount = 2;
                    allTriangles.push_back(baseTriangle);

                }
                if(currentGroup.size() > 2) {
                    baseTriangle.blend3 = currentGroup[2];
                    baseTriangle.itemCount = 3;
                    allTriangles.push_back(baseTriangle);

                }
                continue;
            }
            //For each set of three in currentGroup, make a triangle
            for(int i = 0; i < currentGroup.size() - 2; i++) {
                for(int j = i+1; j < currentGroup.size() - 1; j++) {
                    for(int k = j+1; k < currentGroup.size(); k++) {
                        BlendTriangle triangle;
                        triangle.blend1 = currentGroup[i];
                        triangle.blend2 = currentGroup[j];
                        triangle.blend3 = currentGroup[k];
                        triangle.itemCount = 3;
                        allTriangles.push_back(triangle);
                    }
                }
            }
            
        }
        return allTriangles;
    }


    // Helper to calculate the 2D cross product of vectors OA and OB.
    // Returns a positive value, negative value, or zero.
    // > 0 : Counter-Clockwise turn (Left)
    // < 0 : Clockwise turn (Right)
    // = 0 : Collinear
    double cross_product(const Point& O, const Point& A, const Point& B) {
        return (A.first - O.first) * (B.second - O.second) - 
            (A.second - O.second) * (B.first - O.first);
    }

    /**
     * @brief Computes the 2D Convex Hull of a set of points.
     * @param points Input vector of coordinates (x, y).
     * @return std::vector<Point> Ordered points of the convex hull (Counter-Clockwise).
     * The last point is NOT a duplicate of the first.
     */
    std::vector<Point> get_convex_hull(std::vector<Point> points) {
        size_t n = points.size();
        if (n <= 2) return points; // Cannot form a hull with fewer than 3 points

        // 1. Sort points lexicographically (by X, then by Y).
        // std::pair has built-in comparison operators that do exactly this.
        std::sort(points.begin(), points.end());

        std::vector<Point> hull;

        // 2. Build Lower Hull
        // We iterate from left to right.
        for (const auto& p : points) {
            // While the hull has at least 2 points and the last 3 points do NOT make a CCW turn...
            while (hull.size() >= 2 && 
                cross_product(hull[hull.size() - 2], hull.back(), p) <= 0) {
                hull.pop_back(); // Remove the middle point, it's inside the hull or collinear
            }
            hull.push_back(p);
        }

        // 3. Build Upper Hull
        // We iterate from right to left.
        // Note: The last point of the lower hull is the first point of the upper hull,
        // so we set the stack size limit to t + 1 to avoid popping the shared point initially.
        size_t lower_hull_size = hull.size();
        for (int i = n - 2; i >= 0; i--) {
            const auto& p = points[i];
            while (hull.size() > lower_hull_size && 
                cross_product(hull[hull.size() - 2], hull.back(), p) <= 0) {
                hull.pop_back();
            }
            hull.push_back(p);
        }

        // 4. Cleanup
        // The Monotone Chain algorithm adds the start point twice (at the beginning and the end).
        // Remove the last point to keep the list unique.
        hull.pop_back();

        return hull;
    }

    std::vector<SourceTriangle> getSourceTriangles(std::vector<GasSource> sources_in) 
    {
        std::vector<GasSource> sources;
        for(auto source : sources_in) {
            if (source.unlimited){
                sources.push_back(source);
            }
        }
        std::vector<SourceTriangle> triangles;
        int n = sources.size();

        // Safety check
        if (n < 1) {
            return triangles;
        }

        if (n == 1) {
            SourceTriangle tri;
            tri.itemCount = 1;
            tri.source1 = sources[0];
            triangles.push_back(tri);
            return triangles;
        }

        if (n == 2) {
            SourceTriangle tri;
            tri.itemCount = 2;
            tri.source1 = sources[0];
            tri.source2 = sources[1];
            triangles.push_back(tri);
            return triangles;
        }

        // 1. Generate all unique combinations (N choose 3)
        for (int i = 0; i < n - 2; ++i) {
            for (int j = i + 1; j < n - 1; ++j) {
                for (int k = j + 1; k < n; ++k) {
                    // Create the triangle
                    SourceTriangle tri;
                    tri.itemCount = 3;
                    tri.source1 = sources[i];
                    tri.source2 = sources[j];
                    tri.source3 = sources[k];
                    triangles.push_back(tri);
                }
            }
        }

        // 2. Sort the generated triangles by the sum of their costs
        std::sort(triangles.begin(), triangles.end(), 
            [](const SourceTriangle& a, const SourceTriangle& b) {
                double costA = a.source1.cost_per_unit_volume + a.source2.cost_per_unit_volume + a.source3.cost_per_unit_volume;
                double costB = b.source1.cost_per_unit_volume + b.source2.cost_per_unit_volume + b.source3.cost_per_unit_volume;
                return costA < costB;
            }
        );

        return triangles;
    }

    /**
     * @brief Calculate the top off blend needed to fill the target cylinder
     * 
     * This function takes in a target cylinder and a gas source, and returns a blend object
     * that represents the blend needed to fill the target cylinder from its current state to its
     * target state. The returned blend has only one step, which is the gas source passed in, with
     * a volume equal to the difference between the target volume and the current volume.
     * 
     * @param target The target cylinder to fill
     * @param source The gas source to use for filling the target cylinder
     * @return A blend object representing the top off blend needed to fill the target cylinder
     */
    Blend* calculateTopOffBlend(const TargetCylinder& target, const GasSource source) {

        Blender::Blend* topOff = new Blend();
        topOff->removedGasVolume = 0;
        topOff->target = target;

        double volumeToAdd = target.fillVolume.mliter - topOff->currentVolume();
        if ( volumeToAdd <= 0) {
            return nullptr;
        }
        BlendStep step;
        step.source = source;
        step.volume.mliter = volumeToAdd;
        topOff->steps.push_back(step);
        return topOff;

    }

}