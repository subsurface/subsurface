#ifndef GASBLENDING_H
#define GASBLENDING_H

#include "units.h"
#include <vector>
#include <cmath>
#include <string>

namespace Blender {

    constexpr double ATM = 1013.250; //This is redefined here so we can change it to use a variable if we want to later
    constexpr double N2_g = 0.0011572; // g/ml @ 1ATM, 22C, using our Z value
    constexpr double O2_g = 0.001322; // g/ml @ 1ATM, 22C
    constexpr double He_g = 0.0001645; // g/ml @ 1ATM, 22C
    constexpr int BOOSTED_LIMIT = 7000; //mbar limit for boosted gas to get down to. ~100PSI
    using Point = std::pair<double, double>;

    /**
     * @brief Represents the state and properties of the cylinder being filled.
     */
    struct TargetCylinder {
        /** @brief Current percentage of Helium in the cylinder (permille). */
        int currentHe_permille;

        /** @brief Current percentage of Oxygen in the cylinder (permille). */
        int currentO2_permille;

        /** @brief Current percentage of Nitrogen in the cylinder (permille). */
        int currentN2_permille;

        /** @brief Target percentage of Helium for the final mix (permille). */
        int targetHe_permille;

        /** @brief Target percentage of Oxygen for the final mix (permille). */
        int targetO2_permille;

        /** @brief The current pressure inside the cylinder. */
        pressure_t currentPressure;

        /** @brief The target pressure inside the cylinder. */
        pressure_t targetPressure;

        /** @brief The rated working pressure of the cylinder. */
        pressure_t workingPressure;

        /** @brief The current volume of gas in the cylinder. */
        volume_t currentVolume;

        /** @brief The internal volume of the cylinder (water capacity). */
        volume_t wetVolume;

        /** @brief The target volume of gas to be in the cylinder after filling. */
        volume_t fillVolume;
    };

    /**
     * @brief Represents a source gas cylinder available for blending.
     */
    struct GasSource {
        /** @brief Percentage of Oxygen in the source gas (permille). */
        int o2_permille;

        /** @brief Percentage of Helium in the source gas (permille). */
        int he_permille;

        /** @brief The rated working pressure of the source cylinder. */
        pressure_t workingPressure;

        /** @brief The internal volume of the source cylinder (water capacity). */
        volume_t wetVolume;

        /** @brief The total volume of gas the source cylinder can hold at working pressure. */
        volume_t fillVolume;

        /** @brief The current volume of gas in the source cylinder. */
        volume_t currentVolume;

        /** @brief The current pressure inside the source cylinder. */
        pressure_t currentPressure;

        /** @brief The cost per unit of volume for this gas (e.g., cost per liter or cuft). */
        double cost_per_unit_volume;

        /** @brief Bool indicating if this cylinder is unlimited */
        bool unlimited;

        /** @brief int indicating a unique number for this cylinder */
        int cylinder_number;

        /** @brief bool indicating if this gas is boosted */
        bool boosted;

        /** @brief Tank Factor used to handle limited sources in symplex solver */
        double getTankFactor() const;
        
    };

    /**
     * @brief Output Step represents four values, [add/remove], volume (ml), TO gauge pressure (mbar), TO weight (grams)  
     */
    struct OutputStep {
        bool add;
        volume_t volume;
        pressure_t gaugePressure;
        double weight;
        double cost_per_unit_volume;
        int cylinder_number;
        std::string mix;
        int limited_pressure;
    };

    /**
     * @brief Represents a step in a blend.
     * * This struct contains a single GasSource, the volume of gas
     * */
    struct BlendStep {
        GasSource source;
        volume_t volume;
    };

    /**
     * @brief The result of the Simplex optimization.
     */
    struct SimplexResult {
        /** @brief True if the solver perfectly hit the target mix and volume. 
         * False if it hit a physical limit and had to approximate the mix or underfill. */
        bool is_exact;
        
        /** @brief The calculated volumes for each variable. 
         * Index 0 is the Residual Gas (v0). Indices 1..N correspond to the gasSources. */
        std::vector<double> solvedVolumes;
    };

    /**
     * @brief Represents a blend
     * * This struct contains a vector of Blend Steps, a target cylinder, the amount to remove from the target cylinder,
     * and functions to calculate the current gas mix and the needed gas mix.
     */
    struct Blend {
        std::vector<BlendStep> steps;
        TargetCylinder target;
        volume_t removedGasVolume;
        std::vector<GasSource> gasSources;
        bool isFeasible;
        bool isBestEffort;
        double finalO2 = 0.0;
        double finalHe = 0.0;


        std::vector<std::vector<double>> getSimplexMatrix() const;

        std::vector<OutputStep> getOutputSteps();

        /**
         * @brief Checks if the blend is valid. If partial is false, checks if the blend satisfies the target cylinder's targets
         */
        double validate() const;

        double cost() const;
    };
    

    // ============================================================================
    // Logic Functions
    // ============================================================================


    /**
     * @brief: Calculate the pressure of a mix in a given wet volume
     * 
     * This uses an approximation on the compressibility, then uses that approximation to calculate the pressure
     * (In theory, there is a way to not use this approximation, but this should be good enough)
     */
    double calculatePressure(int o2_permille, int he_permille, double wetVolume, double gasVolume);

    /**
     * @brief Calculates the compressibility factor (Z-factor) for a gas mix at a given pressure.
     */
    double get_compressibility_factor(int o2_permille, int he_permille, int n2_permille, pressure_t pressure);

    /**
     * @brief Calculates the unknown volume properties of a target cylinder.
     */
    void calculate_cylinder_volumes(TargetCylinder &cylinder);

    std::vector<Blend> generateExhaustiveBlends(const TargetCylinder& target, const std::vector<GasSource>& sources);

    SimplexResult runSimplexSolver(std::vector<std::vector<double>> simplexMatrix);

    bool evaluateSequence(Blend& blend);

    Blend findCheapestBlend(std::vector<Blend> blends);

}

#endif // GASBLENDING_H