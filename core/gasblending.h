#ifndef GASBLENDING_H
#define GASBLENDING_H

#include "units.h"
#include <vector>
#include <cmath>

namespace Blender {

        // 1. Move global constants inside
        constexpr double ATM = 1013.250;
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

        /** @brief Target percentage of Nitrogen for the final mix (permille). */
        int targetN2_permille;

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

        /** @brief Percentage of Nitrogen in the source gas (permille). */
        int n2_permille;

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

        /** @brief Bool indicating if this cylinder is boosted/unlimited */
        bool unlimited;

        /** @brief int indicating a unique number for this cylinder */
        int cylinder_number;
        
        // Operator needed for comparison
        bool operator!=(const GasSource& other) const;
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
     * @brief Represents a blend
     * * This struct contains a vector of Blend Steps, a target cylinder, the amount to remove from the target cylinder,
     * and functions to calculate the current gas mix and the needed gas mix.
     */
    struct Blend {
        std::vector<BlendStep> steps;
        TargetCylinder target;
        double removedGasVolume;

        /**
         * @brief Returns the current percentage of O2 in the blend, in permille
         */
        double currentO2() const;

        /**
         * @brief Returns the current percentage of He in the blend, in permille
         */
        double currentHe() const;

        double neededO2() const;

        double neededO2BlendInterpolation(int o2_permille_blend, int target_volume) const;

        double neededHe() const;

        double neededHeBlendInterpolation(int he_permille_blend, int target_volume) const;

        double currentVolume() const;

        bool correctPressures();

        /**
         * @brief Checks if the blend is valid and fills the target cylinder correctly depending on partial
         */
        bool validate(bool partial) const;

        double cost() const;
    };

    struct BlendTriangle{
        int itemCount;
        Blend blend1;
        Blend blend2;
        Blend blend3;
    };

    struct SourceTriangle{
        int itemCount;
        GasSource source1;
        GasSource source2;
        GasSource source3;
    };

    // ============================================================================
    // Logic Functions
    // Note: Ensure 'static' is removed from the .cpp definitions to expose these.
    // ============================================================================

    /**
     * @brief: 2 dimensional LINEAR interpolation. 
     */
    double linear_interpolate_2d(double x1, double y1, double z1, double x2, double y2, double z2, double target_x, double target_y);

    /**
     * @brief Performs linear interpolation for a point within a 3D triangle.
     */
    double triangular_interpolate(double x1, double y1, double z1,
                                double x2, double y2, double z2,
                                double x3, double y3, double z3,
                                double target_x, double target_y);

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

    /**
     * @brief Calculates the intersection of two lines.
     */
    bool calculateIntersection(double x1, double y1, double x2, double y2,
                            double x3, double y3, double x4, double y4,
                            double& out_x, double& out_y);

    /**
     * @brief handle the blend objects that need to be 2D/3D interpolated to a particular point
     */
    Blend* interpolateBlends(const Blend* b1, const Blend* b2, const Blend* b3, double x, double y);

    /**
     * @brief Calculate the steps needed to blend the sources to a target mix and volume
     */
    std::vector<BlendStep> calculateSourceMix(GasSource source1, GasSource source2, const GasSource* source3, double targetO2, double targetHe, double targetVolume);

    /**
     * @brief Helper function to determine the "sign" of a point relative to a line segment.
     */
    double sign(double x1, double y1, double x2, double y2, double x3, double y3);

    /**
     * @brief Determine if a point (tx, ty) is within a triangle formed by the points (x1, y1), (x2, y2), (x3, y3).
     */
    bool pointInTriangle(double x1, double y1, double x2, double y2, double x3, double y3, double tx, double ty);

    /**
     * @brief Calculate the blend. This is the primary operating function.
     */
    Blend* calculate_blend(const Blend* blend1, const Blend* blend2, const Blend* blend3, const GasSource* source1, const GasSource* source2, const GasSource* source3);

    std::vector<BlendTriangle> generateExhaustiveBlends(const TargetCylinder& target, const std::vector<GasSource>& limitedSources);

    std::vector<Point> get_convex_hull(std::vector<Point> points);

    std::vector<SourceTriangle> getSourceTriangles(std::vector<GasSource> sources);
}

#endif // GASBLENDING_H