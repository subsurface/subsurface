// SPDX-License-Identifier: GPL-2.0
#include "gasblendermodel.h"

#include "core/dive.h"
#include "core/gasblending.h"
#include "core/settings/qPrefDivePlanner.h"
#include "core/settings/qPrefUnit.h"
#include "core/gettextfromc.h"
#include "core/format.h"
#include "core/string-format.h"
#include "core/qthelper.h"
#include "core/equipment.h" 

GasBlenderModel::GasBlenderModel(QObject *parent) : QObject(parent)
{
}

GasBlenderModel *GasBlenderModel::instance()
{
	static GasBlenderModel self;
	return &self;
}

QString GasBlenderModel::calculateGasInfo(const QString &cylinderType, int o2_permille, int he_permille)
{
	struct dive temp_dive;
	make_planner_dc(&temp_dive.dcs[0]);
	cylinder_t temp_cyl;

	std::pair<volume_t, pressure_t> type_info = get_tank_info_data(tank_info_table, cylinderType.toStdString());
	temp_cyl.type.size = type_info.first;
	temp_cyl.type.workingpressure = type_info.second;
	temp_cyl.type.description = cylinderType.toStdString();

	// Populate gas mix
	temp_cyl.gasmix.o2.permille = o2_permille;
	temp_cyl.gasmix.he.permille = he_permille;
	sanitize_gasmix(temp_cyl.gasmix);

	bool o2_is_narcotic = qPrefDivePlanner::o2narcotic();
	int fNarcotic_permille = o2_is_narcotic ? (get_n2(temp_cyl.gasmix) + o2_permille) : get_n2(temp_cyl.gasmix);

	QString results = "<table width='100%'>";
	QString ead_header = o2_is_narcotic ? tr("END @ Depth") : tr("EAD @ Depth");
	results += QString("<tr><th align='center' style='padding-bottom: 5px;'><b>pO₂</b></th><th align='center' style='padding-bottom: 5px;'><b>Depth</b></th><th align='center' style='padding-bottom: 5px;'><b>%1</b></th></tr>").arg(ead_header);

	double po2_values[] = { 0.16, 0.2, 0.5, 0.8, 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0 };

	for (double po2 : po2_values) {
		pressure_t po2_limit = { .mbar = static_cast<int>(po2 * 1000.0) };

		// Calculate MOD
		depth_t mod = temp_dive.gas_mod(temp_cyl.gasmix, po2_limit, 1_m);
		double p_amb_at_mod = temp_dive.depth_to_atm(mod);

		double p_narcotic = p_amb_at_mod * fNarcotic_permille / 1000.0;

		depth_t narcotic_depth = { .mm = 0 };
		if (fNarcotic_permille > 0) {
			double divisor = o2_is_narcotic ? 1.0 : 0.79;
			double ead_atm = p_narcotic / divisor;
			narcotic_depth.mm = static_cast<int>((ead_atm - 1.0) * 10000.0);
			if (narcotic_depth.mm < 0)
				narcotic_depth.mm = 0;
		}
		if (mod.mm < 0) {
			continue;
		}
		if (po2 < .2) {
			results += QString("<tr><td align='center'>%1</td><td align='center'>%2</td><td align='center'>%3</td></tr>")
				   .arg(QString::number(po2, 'f', 2))
				   .arg(get_depth_string(mod, true))
				   .arg(get_depth_string(narcotic_depth, true));
		} else {
			results += QString("<tr><td align='center'>%1</td><td align='center'>%2</td><td align='center'>%3</td></tr>")
					.arg(QString::number(po2, 'f', 1))
					.arg(get_depth_string(mod, true))
					.arg(get_depth_string(narcotic_depth, true));
		}
	}
	results += "</table>";
	return results;
}

static double to_mbar_internal(double inputPressure) {
	if (prefs.units.pressure == units::BAR) {
		return inputPressure * 1000.0;
	} else {
		// PSI to mbar
		return psi_to_mbar(inputPressure);
	}
}

QString GasBlenderModel::calculateCheapestBlend(
	const QString &target_amount,        
	const QString &target_start_mix,     
	int target_start_pressure,           
	const QString &target_end_mix,       
	int target_end_pressure,             
	const QVariantList &available_gases_in, 
	bool simple_blend)
{
	QString result = "";
	Blender::TargetCylinder target;

	std::pair<volume_t, pressure_t> type_info = get_tank_info_data(tank_info_table, target_amount.toStdString());
	
	if (type_info.first.mliter == 0) {
		result += QString("Could not find cylinder type [%1], defaulting to AL80<br>").arg(target_amount);
		return result;
	} else {
		result += QString("Found cylinder type [%1] with volume [%2] and w.pressure [%3]<br>").arg(target_amount).arg(type_info.first.mliter).arg(type_info.second.mbar);
		target.wetVolume = type_info.first;
		target.workingPressure = type_info.second;
	}
	
	target.fillVolume.mliter = 0; 
	target.currentPressure.mbar = to_mbar_internal(target_start_pressure);
	target.targetPressure.mbar = to_mbar_internal(target_end_pressure); 

	target.currentO2_permille = parseGasMixO2(target_start_mix);
	target.currentHe_permille = parseGasMixHE(target_start_mix);

	target.targetO2_permille = parseGasMixO2(target_end_mix);
	target.targetHe_permille = parseGasMixHE(target_end_mix);

	result += QString("Target Cylinder Current o2/He permille: %1/%2<br>").arg(target.currentO2_permille).arg(target.currentHe_permille);
	result += QString("Target Cylinder Target o2/He permille: %1/%2<br>").arg(target.targetO2_permille).arg(target.targetHe_permille);
	result += QString("Target Cylinder Working Pressure: %1 mbar<br>").arg(target.targetPressure.mbar);
	result += QString("Target Cylinder Current Pressure: %1 mbar<br>").arg(target.currentPressure.mbar);
	
	Blender::calculate_cylinder_volumes(target);
	result += QString("Target Cylinder Current Pressure2: %1 mbar<br>").arg(target.currentPressure.mbar);

	std::vector<Blender::GasSource> sources;
	int cylinderCounter = 0;

	for (const QVariant &item : available_gases_in) {
		QVariantMap map = item.toMap();
		Blender::GasSource source;

		source.o2_permille = parseGasMixO2(map["mix"].toString());
		source.he_permille = parseGasMixHE(map["mix"].toString());
		source.boosted = map["boost"].toBool();
		source.cost_per_unit_volume = map["cost"].toString().toDouble();

		QString amountStr = map["amount"].toString();
		
		if (simple_blend || amountStr == "∞" || amountStr == "UNLIMITED") {
			source.unlimited = true;
			result += "Added unlimited cylinder (" + amountStr + "): (" + map["mix"].toString() + ") " + QString::number(source.o2_permille) + "/" + QString::number(source.he_permille) + "<br>";
		} else {
			source.unlimited = false;

			std::pair<volume_t, pressure_t> src_type_info = get_tank_info_data(tank_info_table, amountStr.toStdString());
			
			if (src_type_info.first.mliter == 0) {
				source.wetVolume.mliter = 11100; 
				source.workingPressure.mbar = 207000;
			} else {
				source.wetVolume = src_type_info.first;
				source.workingPressure = src_type_info.second;
			}

			result += "Current pressure: " + map["pressure"].toString() + " units<br>";
			source.currentPressure.mbar = to_mbar_internal(map["pressure"].toString().toDouble()); 
			result += "Current Pressure mbar: " + QString::number(source.currentPressure.mbar) + "<br>";

			int n2_permille = 1000 - source.o2_permille - source.he_permille;
			double z = Blender::get_compressibility_factor(source.o2_permille, source.he_permille, n2_permille, source.currentPressure);
			if (z > 0.0) {
				 source.currentVolume.mliter = ((source.currentPressure.mbar) / (z * Blender::ATM) * source.wetVolume.mliter);
			} else {
				 source.currentVolume.mliter = source.wetVolume.mliter;
			}
		}

		source.cylinder_number = cylinderCounter++;
		sources.push_back(source);
	}

	std::vector<Blender::Blend> blends;
	blends = Blender::generateExhaustiveBlends(target, sources);
	Blender::Blend cheapest = Blender::findCheapestBlend(blends);

	if (!cheapest.isFeasible) {
		result += "No Feasible Blend Found.<br>";
		return result;
	}

	int loops = 0;
	double real_cost = 0.0;
	QString output = "Testing Blend...<br>";

	while(loops < 5) {
		loops++;
		std::vector<Blender::OutputStep> output_steps = cheapest.getOutputSteps();
		real_cost = 0.0;
		output = "<b><span style='color: red;'>DISCLAIMER/WARNING: ALWAYS TEST YOUR BLENDS. THIS CALCULATOR IS IN BETA AND INCLUDES NO GUARANTEE OF ACCURACY OR WARRANTY.</span><br> Pressures and mix ratios given are representitave of margins of error. Volume and weight calculations are more accurate than pressure. Be mindful of the accuracy of your pressure guage.</b><br>";
		output += "Found " + QString::number(blends.size()) + " blend(s).<br>";
		output += "<b>" + tr("Best Blend Result") + ":</b><br>";
		for (Blender::OutputStep step : output_steps) {
			QString pressureString;
			if (prefs.units.pressure == units::BAR) {
				pressureString = QString::number(step.gaugePressure.mbar/1000.0, 'f', 1) + " Bar";
			} else {
				pressureString = QString::number(mbar_to_PSI(step.gaugePressure.mbar), 'f', 0) + " PSI";
			}
			QString limitedPressureString = "";
			if (step.limited_pressure != -1) {
				if (prefs.units.pressure == units::BAR) {
					limitedPressureString = " (source to " + QString::number(step.limited_pressure/1000.0, 'f', 1) + " Bar)";
				} else {
					limitedPressureString = " (source to " + QString::number(mbar_to_PSI(step.limited_pressure), 'f', 0) + " PSI) ";
				}
			}
			QString volumeString;
			double stepVol = 0.0;
			if (prefs.units.volume == units::LITER) {
				stepVol = step.volume.mliter/1000.0;
				volumeString = " (" + QString::number(stepVol, 'f', 2) + " Liters)";
			} else {
				stepVol = ml_to_cuft(step.volume.mliter);
				volumeString = " (" +QString::number(stepVol, 'f', 2) + " Cuft)";
			}
			QString weightString = QString::number(step.weight, 'f', 0) + " Grams";
			real_cost += stepVol * step.cost_per_unit_volume;

			if (step.add) {
				output += "Add " + weightString + volumeString + " to " + pressureString + " of " + QString::fromStdString(step.mix) + "(" + QString::number(step.cylinder_number) + ")" + limitedPressureString + "<br>";
			} else {
				output += "Remove " + weightString + volumeString + " to " + pressureString + "<br>";
			}
		}
		if (cheapest.isBestEffort) {
			target.targetO2_permille = cheapest.finalO2;
			target.targetHe_permille = cheapest.finalHe;
			target.fillVolume.mliter = 0.0;
			Blender::calculate_cylinder_volumes(target);
			
			cheapest.target = target;
			cheapest = Blender::findCheapestBlend({cheapest});
		} else {
			break;
		}
	}

	if (cheapest.isBestEffort || loops > 1) {
		output += "<br><b><span style='color: red;'>THIS MIX FAILED TO CONVERGE, IT IS THE CLOSEST MIX FOUND TO THE TARGET</span></b><br>";
	}
	output += "<br>Final Mix: " + QString::number(cheapest.finalO2 / 10.0, 'f', 2) + "/" + QString::number(cheapest.finalHe / 10.0, 'f', 2) + "<br>";
	output += tr("<br>Total Cost: %1").arg(QString::number(real_cost, 'f', 2)) + "<br><br>";

	return output;
}
