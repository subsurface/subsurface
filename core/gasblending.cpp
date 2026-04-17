	#include "gasblending.h"
	#include "gas.h"
	#include "units.h"
	#include <cmath>      
	#include <algorithm>
	#include <vector>
	#include "errorhelper.h"

	using namespace std;
namespace Blender {


	//=============================================================================
	// GasSource Member Implementations
	//=============================================================================

	/**
	 * @brief Calculates the tank factor for the source cylinder.
	 *
	 * @return The tank factor of the source cylinder.
	 */
	double GasSource::getTankFactor() const {
		double vol = static_cast<double>(wetVolume.mliter);
        double press = static_cast<double>(ATM);

        if (press <= 0.0) return 0.0; // Safety check

        return press / vol;	
	}

	// ============================================================================
	// Blend Member Implementations
	// ============================================================================

	/**
	 * @brief: Get the series of OutputSteps representing this blend object
	 */
	std::vector<OutputStep> Blend::getOutputSteps() {
		std::vector<OutputStep> outputSteps;
		double currentVolume = target.currentVolume.mliter - removedGasVolume.mliter;
		double stepEndPressure = target.currentPressure.mbar;
		double currentO2Vol = (target.currentVolume.mliter - removedGasVolume.mliter) / 1000.0 * target.currentO2_permille;
		double currentHeVol = (target.currentVolume.mliter - removedGasVolume.mliter) / 1000.0 * target.currentHe_permille;
		if (removedGasVolume.mliter > 0) {
			OutputStep os;
			os.add = false;
			os.volume.mliter = removedGasVolume.mliter;
			os.weight =  removedGasVolume.mliter * target.currentO2_permille / 1000.0 * O2_g + removedGasVolume.mliter * target.currentHe_permille / 1000.0 * He_g + removedGasVolume.mliter * target.currentN2_permille / 1000.0 * N2_g;
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
		finalO2 = currentO2Vol * 1000.0 / currentVolume;
		finalHe = currentHeVol * 1000.0 / currentVolume;
		return outputSteps;
	}

	/**
	 * @brief Checks if the blend is valid and fills the target cylinder correctly depending on partial
	 * * If partial is true, we only want to make sure the current blend steps can be accomplished
	 * If partial is false, we want to make sure the current blend steps can be accomplished and the target cylinder is full
	 * */
	double Blend::validate() const {
		// 1. Base Physics Check: Cannot drain below 1 ATM
		if (target.currentVolume.mliter - removedGasVolume.mliter < target.wetVolume.mliter) {
			return -1.0; 
		}

		double currentVol = target.currentVolume.mliter - removedGasVolume.mliter;
		double currentO2Vol = currentVol / 1000.0 * target.currentO2_permille;
		double currentHeVol = currentVol / 1000.0 * target.currentHe_permille;

		// 2. Iterate through the steps and simulate the physical fill
		for (const BlendStep& step : steps) {
			if (step.volume.mliter < 0) {
				return -1.0; // Negative volume pull is physically impossible
			}

			double stepO2Vol = step.volume.mliter / 1000.0 * step.source.o2_permille;
			double stepHeVol = step.volume.mliter / 1000.0 * step.source.he_permille;
			
			currentO2Vol += stepO2Vol;
			currentHeVol += stepHeVol;
			currentVol += step.volume.mliter;

			// Calculate intermediate pressure to check cascade limits
			int currentO2Permille = std::round((currentO2Vol * 1000.0) / currentVol);
			int currentHePermille = std::round((currentHeVol * 1000.0) / currentVol);
			
			double currentPressure = calculatePressure(currentO2Permille, currentHePermille, target.wetVolume.mliter, currentVol) - ATM;

			if (!step.source.unlimited) {
				double stepEndPressure = calculatePressure(step.source.o2_permille, step.source.he_permille, step.source.wetVolume.mliter, step.source.currentVolume.mliter - step.volume.mliter) - ATM;
				
				// Cascade constraint check
				if (currentPressure > stepEndPressure && !step.source.boosted) {
					return -1.0; 
				}
				// Booster constraint check
				if (step.source.boosted) {
					if (stepEndPressure < BOOSTED_LIMIT) {
						return -1.0; 
					}
				}
			}
		}

		// 3. Calculate the Error Metric (Distance from Ideal)
		// If we reach here, the blend is physically safe. Now we score it.
		
		double final_o2_permille = (currentO2Vol * 1000.0) / currentVol;
		double final_he_permille = (currentHeVol * 1000.0) / currentVol;

		double o2_error = std::abs(final_o2_permille - target.targetO2_permille);
		double he_error = std::abs(final_he_permille - target.targetHe_permille);
		
		// Normalize volume error into parts-per-thousand (permille) so it scales with the gas errors
		double vol_error = std::abs(currentVol - target.fillVolume.mliter) / target.fillVolume.mliter * 1000.0;

		double total_error = o2_error + he_error + vol_error;

		// Handle floating point noise. If the total error is less than 1 part per thousand, 
		// it is functionally a perfect fill.
		if (total_error <= 1.0) {
			return 0.0;
		}
		return total_error;
	}

	double Blend::cost() const { 
		double cost = 0.0;
		for (const BlendStep& step : steps) {
			cost += step.volume.mliter / 1000.0 * step.source.cost_per_unit_volume;
		}
		return cost; 
	}


	std::vector<std::vector<double>> Blend::getSimplexMatrix() const {
		int N = gasSources.size();
		
		// Count limited sources (boosted are also limited)
		int L = 0;
		for (const auto& source : gasSources) {
			if (!source.unlimited) { 
				L++;
			}
		}

		// MATRIX SIZING (Shifted to account for the implicit Residual Gas)
		// Columns: 1 (Residual) + N (Sources) + 1 (Res_Slack) + L (Source_Slacks) + 3 (Arts) + 1 (RHS)
		int num_cols = (1 + N) + (1 + L) + 3 + 1; 
		// Rows: 3 (Vol, O2, He) + 1 (Res_Limit) + L (Source_Limits) + 1 (Cost Row)
		int num_rows = 3 + (1 + L) + 1;     

		std::vector<std::vector<double>> matrix(num_rows, std::vector<double>(num_cols, 0.0));

		// Column Offsets
		int offset_real  = 1;                 // Supply gasses start at Col 1
		int offset_slack = 1 + N;             // Slacks start after the real variables
		int offset_art   = (1 + N) + (1 + L); // Artificials start after slacks
		int col_rhs      = num_cols - 1;
		
		double SAFETY_MARGIN = 0.90;

		// ==========================================
		// ROW 0: Total Volume Equality
		// ==========================================
		matrix[0][0] = 1.0; // Implicit Residual Gas (v0)
		for (int j = 0; j < N; ++j) {
			matrix[0][offset_real + j] = 1.0; 
		}
		matrix[0][offset_art + 0] = 1.0; 
		matrix[0][col_rhs] = static_cast<double>(target.fillVolume.mliter - target.wetVolume.mliter); // Full fill volume

		// ==========================================
		// ROW 1: Oxygen Equality
		// ==========================================
		matrix[1][0] = target.currentO2_permille / 1000.0; // Implicit Residual Gas O2
		for (int j = 0; j < N; ++j) {
			matrix[1][offset_real + j] = gasSources[j].o2_permille / 1000.0; 
		}
		matrix[1][offset_art + 1] = 1.0; 
		matrix[1][col_rhs] = ((target.targetO2_permille / 1000.0) * target.fillVolume.mliter) - 
                             ((target.currentO2_permille / 1000.0) * target.wetVolume.mliter);
		if (matrix[1][col_rhs] < 0) matrix[1][col_rhs] = 0.0;
		// ==========================================
		// ROW 2: Helium Equality
		// ==========================================
		matrix[2][0] = target.currentHe_permille / 1000.0; // Implicit Residual Gas He
		for (int j = 0; j < N; ++j) {
			matrix[2][offset_real + j] = gasSources[j].he_permille / 1000.0; 
		}
		matrix[2][offset_art + 2] = 1.0; 
		matrix[2][col_rhs] = ((target.targetHe_permille / 1000.0) * target.fillVolume.mliter) - 
                             ((target.currentHe_permille / 1000.0) * target.wetVolume.mliter);
		if (matrix[2][col_rhs] < 0) matrix[2][col_rhs] = 0.0;
		// ==========================================
		// ROW 3: Residual Gas Limit
		// ==========================================
		// Constraint: v_0 + s_res <= current volume in the tank
		matrix[3][0] = 1.0; 
		matrix[3][offset_slack + 0] = 1.0; // The very first slack variable belongs to the residual gas
		matrix[3][col_rhs] = static_cast<double>(target.currentVolume.mliter - target.wetVolume.mliter);
		if (matrix[3][col_rhs] < 0) matrix[3][col_rhs] = 0.0;

		// ==========================================
		// ROWS 4+: Physical Supply Limits
		// ==========================================
		int limit_row_idx = 4;
		int slack_idx = 1; // Start at 1 because slack 0 is taken by residual
		
		double Ft = ATM / static_cast<double>(target.wetVolume.mliter);

		for (int j = 0; j < N; ++j) {
			if (gasSources[j].unlimited) {
				continue;
			} 

			double Fj = gasSources[j].getTankFactor();

			if (gasSources[j].boosted) {
				// SCENARIO A: Boosted Gas
				double V_sj = static_cast<double>(gasSources[j].currentVolume.mliter);
				double unpumpable_vol = Fj * Blender::BOOSTED_LIMIT; 
				double max_pull = V_sj - unpumpable_vol;
				if (max_pull < 0) max_pull = 0.0;

				matrix[limit_row_idx][col_rhs] = SAFETY_MARGIN * max_pull;
				matrix[limit_row_idx][offset_slack + slack_idx] = 1.0; 
				matrix[limit_row_idx][offset_real + j] = 1.0; // shifted column
				
			} else {
				// SCENARIO B: Cascaded Gas
				double V_sj = static_cast<double>(gasSources[j].currentVolume.mliter);
				
				matrix[limit_row_idx][col_rhs] = SAFETY_MARGIN * ((V_sj * Fj) - ATM);
                matrix[limit_row_idx][offset_slack + slack_idx] = 1.0;

				// 1. Back-pressure from the Residual Gas (v0)
				matrix[limit_row_idx][0] = Ft; 

				// 2. Back-pressure from previous cascaded gasses
				for (int k = 0; k <= j; ++k) {
					if (gasSources[k].unlimited || gasSources[k].boosted) continue;
					
					if (k == j) {
						matrix[limit_row_idx][offset_real + k] = Fj + Ft;
					} else {
						matrix[limit_row_idx][offset_real + k] = Ft;
					}
				}
			}

			limit_row_idx++;
			slack_idx++;
		}

		// ==========================================
        // ROW BOTTOM: The Objective / Cost Row
        // ==========================================
        int row_cost = num_rows - 1;
        
        // Define two tiers of penalties
        double M_VOL = 10000000.0; // Tier 1: Fill the tank NO MATTER WHAT
        double M_MIX = 10000.0;    // Tier 2: Fix the mix if possible
                
        for (int j = 0; j < num_cols - 1; ++j) {
            double current_cost = 0.0;
            
            if (j == 0) {
                current_cost = 0.0; // Residual gas is free!
            } else if (j < 1 + N) {
                current_cost = gasSources[j - 1].cost_per_unit_volume / 1000.0; 
            } else if (j >= offset_art) {
                // Apply the correct penalty tier
                if (j == offset_art + 0) current_cost = M_VOL; // Volume Artificial
                else current_cost = M_MIX;                     // O2 and He Artificials
            }

            // Apply tiered z_penalty
            double z_penalty = (M_VOL * matrix[0][j]) + 
                               (M_MIX * matrix[1][j]) + 
                               (M_MIX * matrix[2][j]);

            matrix[row_cost][j] = current_cost - z_penalty;
        }

        matrix[row_cost][col_rhs] = (matrix[0][col_rhs] * M_VOL) + 
                                    (matrix[1][col_rhs] * M_MIX) + 
                                    (matrix[2][col_rhs] * M_MIX);

        return matrix;
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
			if (wetVolume <= 0.0) return 0.0; // Safety check

			int n2_permille = 1000 - o2_permille - he_permille;
			
			pressure_t atm_pressure;
			atm_pressure.mbar = ATM;
			double z_atm = get_compressibility_factor(o2_permille, he_permille, n2_permille, atm_pressure);

			// 1. Initial Ideal Gas Guess
			double p_guess_mbar = ATM * (gasVolume / wetVolume);

			// 2. Converging Loop
			double p_last = 0.0;
			int iterations = 0;

			// Loop until the pressure stabilizes to within 1 millibar (0.014 PSI), 
			// with a 20-iteration safety cutoff.
			while (std::abs(p_guess_mbar - p_last) > 1.0 && iterations < 20) {
				p_last = p_guess_mbar;
				
				pressure_t current_guess;
				current_guess.mbar = p_guess_mbar;

				// Look up the Z-factor for our current best guess
				double z_current = get_compressibility_factor(o2_permille, he_permille, n2_permille, current_guess);

				// Refine the guess: P = (V_gas / V_wet) * ATM * (Z_current / Z_atm)
				p_guess_mbar = (gasVolume / wetVolume) * ATM * (z_current / z_atm);
				
				iterations++;
			}

			return p_guess_mbar;    
		}


	/**
	 * @brief Calculates the compressibility factor (Z-factor) for a gas mix at a given pressure.
	 * This uses the subsurface function
	 */
	double get_compressibility_factor(int o2_permille, int he_permille, int n2_permille, pressure_t pressure)
	{
        double bar = pressure.mbar / 1000.0;

        struct gasmix gas;
        gas.o2.permille = o2_permille;
        gas.he.permille = he_permille;

        return ::gas_compressibility_factor(gas, bar);

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
			      
			int target_n2 = 1000 - cylinder.targetO2_permille - cylinder.targetHe_permille;
            
            pressure_t target_pres_abs;
            target_pres_abs.mbar = cylinder.targetPressure.mbar + ATM;
            
            pressure_t atm_pressure;
            atm_pressure.mbar = ATM;

            double real_z = get_compressibility_factor(
                cylinder.targetO2_permille, cylinder.targetHe_permille, target_n2, target_pres_abs
            );
            
            double z_atm_target = get_compressibility_factor(
                cylinder.targetO2_permille, cylinder.targetHe_permille, target_n2, atm_pressure
            );

            cylinder.fillVolume.mliter = z_atm_target * (target_pres_abs.mbar / ATM) * (cylinder.wetVolume.mliter / real_z);       
        
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
	std::vector<Blend> generateExhaustiveBlends(const TargetCylinder& target, const std::vector<GasSource>& sources) {
		std::vector<Blend> allBlends;
		Blend baseBlend;
		baseBlend.target = target;
		baseBlend.removedGasVolume.mliter = 0.0;

		//Actually both boosted and unlimited sources. We don't care at this point, since those just get added last no matter what
		std::vector<GasSource> unlimitedSources;
		bool limitedFound = false;

		if (target.currentVolume.mliter > target.wetVolume.mliter + 1.0) {
			baseBlend.removedGasVolume.mliter = target.currentVolume.mliter - target.wetVolume.mliter;
		}


		for (const GasSource& source : sources) {
			if (source.unlimited || source.boosted) {
				unlimitedSources.push_back(source);
				continue;
			}

			if (!limitedFound) {
				//Add the first limited gas source
				limitedFound = true;
				Blend limitedBlend = baseBlend;
				limitedBlend.gasSources.push_back(source);
				allBlends.push_back(limitedBlend);
				continue;
			}
			std::vector<Blend> newBlends;
			for (const Blend& blend : allBlends) {
				for (size_t i = 0; i <= blend.gasSources.size(); i++) {
					Blend limitedBlend = blend;
					limitedBlend.gasSources.insert(limitedBlend.gasSources.begin() + i, source);
					newBlends.push_back(limitedBlend);	
				}
				
			}
			allBlends = newBlends;
		}
		if (allBlends.size() == 0) {
			allBlends.push_back(baseBlend);
		}
		for (Blend& blend : allBlends) {
			blend.gasSources.insert(blend.gasSources.end(), unlimitedSources.begin(), unlimitedSources.end());
		}


		return allBlends;
	}

	// ============================================================================
	// Simplex Solver Implementation
	// ============================================================================

/**
 * @brief Solves a linear optimization problem using the Simplex Algorithm. Gemini helped me with this one
 *
 * The Simplex Algorithm is a linear optimization technique which is used to
 * find the best solution (in terms of a linear cost function) subject to a set of
 * constraints. 
 *
 * The function takes in a matrix which represents the gasses being blended, the target
 * cylinder, and the amount to remove from the target cylinder. It also takes in the
 * constraints of the optimization problem. It returns a SimplexResult object containing
 * the solution vector and a boolean indicating whether the solution was exact or not.
 *
 * The function first checks if the input matrix is empty. If it is, the function
 * immediately returns an empty SimplexResult object.
 *
 * Then the function initializes the basic variables tracker. This tracker is used to
 * keep track of which variables are currently basic and which are not. The tracker
 * is initialized by scanning the matrix to find the initial identity columns (the slack
 * and artificial variables present after the main three equations).
 *
 * The function then enters the main optimization loop. In this loop, the function
 * finds the entering variable (Per Bland's Rule, the FIRST negative) and the leaving variable
 * (Ratio Test). The entering variable is the first negative value in the cost row, and
 * the leaving variable is the variable which is most easily replaced by the entering variable
 * (i.e. the variable with the lowest ratio of its cost row value to its entering variable value).
 *
 * The function then executes the pivot operations. It normalizes the pivot row and zeros out
 * the rest of the column in all other rows (including the cost row).
 *
 * After the pivot operations, the function updates the state tracker. The entering variable
 * kicks out the leaving variable.
 *
 * The function then checks if the final cost is within the threshold (i.e. the "Big-M"
 * Artificial Variables are eliminated). If it is, the function sets the is_exact flag to true.
 *
 * Finally, the function extracts the results. It maps the final basic variables back to a 1D
 * array. Non-basic variables are set to 0.0. The function returns the SimplexResult object.
 */
	SimplexResult runSimplexSolver(std::vector<std::vector<double>> matrix) {
		SimplexResult result;
        const int MAX_ITERATIONS = 500;
		result.is_exact = false;

		if (matrix.empty() || matrix[0].empty()) {
			return result; 
		}

		int num_rows = matrix.size();
		int num_cols = matrix[0].size();
		int row_cost = num_rows - 1;
		int col_rhs = num_cols - 1;
		
		// Floating point threshold to handle C++ math drift
		const double EPSILON = 1e-7;

		// 1. Initialize Basic Variables Tracker
		// To correctly execute Bland's Rule, we must know which variable is currently
		// holding the "basic" status for each row. We scan the matrix to find the 
		// initial identity columns (the slack and artificial variables).
		std::vector<int> basic_vars(num_rows - 1, -1);
		for (int j = 0; j < col_rhs; ++j) {
			int one_row_idx = -1;
			bool is_basic = true;
			
			for (int i = 0; i < num_rows - 1; ++i) {
				if (std::abs(matrix[i][j] - 1.0) < EPSILON) {
					if (one_row_idx != -1) { is_basic = false; break; } // More than one '1'
					one_row_idx = i;
				} else if (std::abs(matrix[i][j]) > EPSILON) {
					is_basic = false; break; // Not a zero
				}
			}
			
			if (is_basic && one_row_idx != -1 && basic_vars[one_row_idx] == -1) {
				basic_vars[one_row_idx] = j;
			}
		}

		int iteration_count = 0;
		// 2. The Main Optimization Loop
		while (true) {
			if (iteration_count > MAX_ITERATIONS) {
				result.is_exact = false;
				return result; //Return an empty result, as the algorithm might not be in a stable state
			}
			iteration_count++;
			// STEP A: Find Entering Variable (Bland's Rule: FIRST negative)
			int enter_col = -1;
			for (int j = 0; j < col_rhs; ++j) {
				if (matrix[row_cost][j] < -EPSILON) {
					enter_col = j;
					break; // Stop immediately to prevent Degeneracy (Cycling)
				}
			}
			
			// If no negative values exist in the cost row, we are at the optimal solution.
			if (enter_col == -1) break;

			// STEP B: Find Leaving Variable (Ratio Test)
			int leave_row = -1;
			double min_ratio = std::numeric_limits<double>::max();
			int min_basic_var_index = std::numeric_limits<int>::max();

			for (int i = 0; i < num_rows - 1; ++i) {
				double col_val = matrix[i][enter_col];
				
				if (col_val > EPSILON) {
					double ratio = matrix[i][col_rhs] / col_val;
					
					// If it is strictly smaller, it wins
					if (ratio < min_ratio - EPSILON) {
						min_ratio = ratio;
						leave_row = i;
						min_basic_var_index = basic_vars[i];
					} 
					// If it is a TIE, apply Bland's Rule for the leaving variable
					else if (std::abs(ratio - min_ratio) <= EPSILON) {
						if (basic_vars[i] < min_basic_var_index) {
							min_ratio = ratio;
							leave_row = i;
							min_basic_var_index = basic_vars[i];
						}
					}
				}
			}

			// If no valid positive ratio is found, the physics model is unbounded/broken.
			if (leave_row == -1) return result; 

			// STEP C: Execute the Pivot Operations
			double pivot_val = matrix[leave_row][enter_col];
			
			// Normalize the pivot row
			for (int j = 0; j < num_cols; ++j) {
				matrix[leave_row][j] /= pivot_val;
				if (std::abs(matrix[leave_row][j]) < EPSILON) {
                    matrix[leave_row][j] = 0.0;
                }
			}
			
			// Zero out the rest of the column in all other rows (including the cost row)
			for (int i = 0; i < num_rows; ++i) {
				if (i != leave_row) {
					double factor = matrix[i][enter_col];
					for (int j = 0; j < num_cols; ++j) {
						matrix[i][j] -= factor * matrix[leave_row][j];
						if (std::abs(matrix[i][j]) < EPSILON) {
                            matrix[i][j] = 0.0;
                        }
					}
				}
			}

			// Update the state tracker: the entering variable kicks out the leaving variable
			basic_vars[leave_row] = enter_col;
		}

		// 3. Extract Results
        // We map the final basic variables back to a 1D array FIRST.
        // All non-basic variables are set to 0.0.
        result.solvedVolumes.assign(col_rhs, 0.0);
        
        for (int i = 0; i < num_rows - 1; ++i) {
            if (basic_vars[i] != -1) {
                result.solvedVolumes[basic_vars[i]] = matrix[i][col_rhs];
            }
        }

        // 4. Feasibility Check
        // In the Big-M method, if an exact solution exists, all Artificial Variables must be zero.
        // Your 3 artificial variables (Volume, O2, He) are always the last 3 columns before the RHS.
        result.is_exact = true;
        for (int j = col_rhs - 3; j < col_rhs; ++j) {
            // If the solver had to use more than a microscopic fraction of "magic" gas, it's not exact.
            if (result.solvedVolumes[j] > EPSILON) { 
                result.is_exact = false;
                break;
            }
        }

		return result;
	}

	/** @brief Evaluates the sequence and populates the blend's state.
     * @param blend The blend to evaluate (passed by reference so it can be modified).
     * @return true if the blend is feasible (exact or best effort).
     */
	bool evaluateSequence(Blend& blend) {
        auto tableau = blend.getSimplexMatrix();
        SimplexResult result = runSimplexSolver(tableau);

        if (result.solvedVolumes.empty()) {
            blend.isFeasible = false;
            blend.isBestEffort = false;
            return false;
        }

        double kept_volume = result.solvedVolumes[0] + blend.target.wetVolume.mliter;
        
        blend.removedGasVolume.mliter = std::max(0.0, blend.target.currentVolume.mliter - kept_volume);
		
		blend.steps.clear();
		for (size_t i = 0; i < blend.gasSources.size(); ++i) {
			if (result.solvedVolumes[i + 1] > 10.0) { // Filter out < 10ml noise
				BlendStep step;
				step.source = blend.gasSources[i];
				step.volume.mliter = result.solvedVolumes[i + 1];
				blend.steps.push_back(step);
			}
		}
		double blend_score = blend.validate();

		if (blend_score < 0.0) {
			blend.isFeasible = false;
			blend.isBestEffort = false;
			return false;
		} 
		else if (blend_score == 0.0) {
			blend.isFeasible = true;
			blend.isBestEffort = false;
			return true;
		} 
		else {
			// Best Effort threshold (e.g., within 300 permille total error)
			if (blend_score <= 3000.0) {
				blend.isFeasible = true;
				blend.isBestEffort = true;
				return true;
			} else {
				blend.isFeasible = false;
				return false;
			}
		}
	}

    Blend findCheapestBlend(std::vector<Blend> blends) {
		const double EPSILON = 1e-5;
		std::vector<Blend> exactBlends;
		std::vector<Blend> bestEffortBlends;
		for (Blend& blend : blends) {
			evaluateSequence(blend);
			if (blend.isFeasible && !blend.isBestEffort) {
				exactBlends.push_back(blend);
			} else if (blend.isBestEffort) {
				bestEffortBlends.push_back(blend);
			}
		}

		if (exactBlends.empty()) {
			if (bestEffortBlends.empty()) {
				Blend failedBlend;
            	failedBlend.isFeasible = false;
            	return failedBlend;
			}
			Blend bestEffortBlend = bestEffortBlends[0];
			for (const Blend& blend : bestEffortBlends) {
				if (blend.validate() < bestEffortBlend.validate() - EPSILON) {
					bestEffortBlend = blend;
					continue;
				} else if(std::abs(blend.validate() - bestEffortBlend.validate()) < EPSILON) {
					if (blend.cost() < bestEffortBlend.cost()) {
						bestEffortBlend = blend;
					}
				}
			}
			return bestEffortBlend;
		}

		Blend& exactBlend = exactBlends[0];
		for (const Blend& blend : exactBlends) {
			if (blend.cost() < exactBlend.cost()) {
				exactBlend = blend;
				continue;
			}
		}
		return exactBlend;
	}

}