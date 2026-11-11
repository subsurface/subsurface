// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude)
#include "testtankgasguess.h"
#include "core/libdivecomputer.h"
#include "core/tankgasguess.h"

#include <QTest>

namespace {

const int ntanks = 4;
const int nsamples = 60;

struct trace {
	std::vector<tank_gas_sample> samples;
	/**
	 * A trace of nsamples samples of ntanks tanks, without gas mixes or readings.
	 */
	trace() : samples(nsamples, tank_gas_sample { -1, std::vector<int>(ntanks, 0) }) {}

	/**
	 * Breathe gas mix 'gasmix' from sample 'from' up to (not including) sample 'to'.
	 */
	void breathe(int from, int to, int gasmix)
	{
		for (int n = from; n < to; n++)
			samples[n].gasmix = gasmix;
	}

	/**
	 * Tank 'tank' reads 'start' mbar at sample 'from', dropping by 'step' mbar per sample.
	 */
	void reading(int tank, int from, int to, int start, int step)
	{
		for (int n = from; n < to; n++)
			samples[n].pressure[tank] = start - step * (n - from);
	}

	/**
	 * Guess the gas mix of each tank, with 3 gas mixes by default.
	 */
	std::vector<int> guess(std::vector<bool> sidemount = std::vector<bool>(ntanks, false), unsigned int ngases = 3) const
	{
		return guess_tank_gasmixes(samples, sidemount, ngases);
	}
};

const std::vector<bool> sidemount_pair = { true, true, false, false };

} // namespace

/**
 * A sidemount pair on bottom gas (2), and stages on 50% (1) and oxygen (0) with
 * their valves closed until the switch: until then, the stage transmitters read
 * the pressure left in their regulators.
 */
void TestTankGasGuess::sidemountWithClosedStages()
{
	trace t;
	t.breathe(0, 30, 2);
	t.breathe(30, 45, 1);
	t.breathe(45, 60, 0);
	t.reading(0, 0, 30, 200000, 1000);
	t.reading(0, 30, 60, 171000, 0);
	t.reading(1, 0, 30, 205000, 1000);
	t.reading(1, 30, 60, 176000, 0);
	t.reading(2, 0, 30, 120000, 1000);
	t.reading(2, 30, 45, 190000, 2000);
	t.reading(2, 45, 60, 162000, 0);
	t.reading(3, 0, 45, 110000, 0);
	t.reading(3, 45, 60, 150000, 2000);
	const std::vector<int> expected = { 2, 2, 1, 0 };
	QCOMPARE(t.guess(sidemount_pair), expected);
	// Without the sidemount flag, each tank still gets the gas mix it was breathed on.
	QCOMPARE(t.guess(), expected);
}

/**
 * Cave staging: a sidemount pair and two staged tanks, all on air (0).
 */
void TestTankGasGuess::allTanksOnOneGas()
{
	trace t;
	t.breathe(0, 60, 0);
	t.reading(0, 0, 20, 200000, 2000);
	t.reading(0, 20, 60, 162000, 0);
	t.reading(1, 0, 20, 200000, 0);
	t.reading(1, 20, 60, 200000, 1000);
	t.reading(2, 0, 40, 190000, 0);
	t.reading(2, 40, 60, 190000, 2000);
	t.reading(3, 0, 60, 190000, 500);
	QCOMPARE(t.guess(sidemount_pair), std::vector<int>({ 0, 0, 0, 0 }));
}

/**
 * A stage with its valve open that was carried but never breathed, so that it
 * only lost 13 bar as it cooled in the water, and tanks without readings.
 */
void TestTankGasGuess::unusedStage()
{
	trace t;
	t.breathe(0, 45, 2);
	t.breathe(45, 60, 1);
	t.reading(0, 0, 60, 200000, 1000);
	t.reading(1, 0, 10, 200000, 1300);
	t.reading(1, 10, 60, 187000, 0);
	QCOMPARE(t.guess(), std::vector<int>({ 2, -1, -1, -1 }));
}

/**
 * A stage whose regulator was charged at the surface and its valve then closed:
 * until the gas switch, the transmitter reads the pressure in the regulator
 * dropping, while the bottom gas (2) is in use. Then the valve is opened, and
 * the stage is breathed on 50% (1).
 */
void TestTankGasGuess::stageChargedAndClosed()
{
	trace t;
	t.breathe(0, 30, 2);
	t.breathe(30, 60, 1);
	t.reading(0, 0, 30, 200000, 1500);
	t.reading(0, 30, 60, 156500, 0);
	t.reading(2, 0, 30, 200000, 3000);
	t.reading(2, 30, 60, 190000, 1000);
	QCOMPARE(t.guess(), std::vector<int>({ 2, -1, 1, -1 }));
}

/**
 * A valve drill: the diver closes the valve, breathes the regulator down and
 * opens the valve again. Only what the reopening makes up for is taken back,
 * so the 30 bar breathed before the drill still count, with 14.5 bar after it.
 */
void TestTankGasGuess::valveDrill()
{
	trace t;
	t.breathe(0, 60, 0);
	t.reading(0, 0, 20, 200000, 1500);
	t.reading(0, 20, 30, 170000, 3000);
	t.reading(0, 30, 60, 170000, 500);
	QCOMPARE(t.guess(), std::vector<int>({ 0, -1, -1, -1 }));
}

/**
 * A rise takes back the most recent drops first: here the 14 bar lost on gas
 * mix 0, not the 29 bar lost on gas mix 1 before them. Taking back the oldest
 * first would leave 15 bar on each, and no link.
 */
void TestTankGasGuess::mostRecentDropsTakenBack()
{
	trace t;
	t.breathe(0, 30, 1);
	t.breathe(30, 60, 0);
	t.reading(0, 0, 45, 200000, 1000);
	t.reading(0, 45, 60, 170000, 0);
	QCOMPARE(t.guess(), std::vector<int>({ 1, -1, -1, -1 }));

	// A rise of 25 bar takes back the 14 bar on gas mix 0, the drop at the gas
	// change, and 10 of the 29 bar on gas mix 1, which leaves too little for a link
	trace cross;
	cross.breathe(0, 30, 1);
	cross.breathe(30, 60, 0);
	cross.reading(0, 0, 45, 200000, 1000);
	cross.reading(0, 45, 60, 181000, 0);
	QCOMPARE(cross.guess(), std::vector<int>({ -1, -1, -1, -1 }));
}

/**
 * Used about as much on two gas mixes (e.g. a gas switch logged late): no guess.
 */
void TestTankGasGuess::ambiguous()
{
	trace t;
	t.breathe(0, 30, 1);
	t.breathe(30, 60, 0);
	t.reading(0, 0, 60, 200000, 1000);
	QCOMPARE(t.guess(), std::vector<int>({ -1, -1, -1, -1 }));
}

/**
 * One side of a sidemount pair barely breathed: it goes with its pair.
 */
void TestTankGasGuess::oneSidedSidemount()
{
	trace t;
	t.breathe(0, 60, 2);
	t.reading(0, 0, 60, 200000, 1000);
	t.reading(1, 0, 50, 200000, 0);
	t.reading(1, 50, 60, 200000, 100);
	QCOMPARE(t.guess(sidemount_pair), std::vector<int>({ 2, 2, -1, -1 }));
	QCOMPARE(t.guess(), std::vector<int>({ 2, -1, -1, -1 }));
}

/**
 * Both sides of a sidemount pair breathed a little, 12 bar each: together they
 * reach the minimum, on their own neither does.
 */
void TestTankGasGuess::pairedSidemountBelowMinimum()
{
	trace t;
	t.breathe(0, 60, 2);
	t.reading(0, 0, 13, 200000, 1000);
	t.reading(0, 13, 60, 188000, 0);
	t.reading(1, 0, 13, 200000, 1000);
	t.reading(1, 13, 60, 188000, 0);
	QCOMPARE(t.guess(sidemount_pair), std::vector<int>({ 2, 2, -1, -1 }));
	QCOMPARE(t.guess(), std::vector<int>({ -1, -1, -1, -1 }));
}

/**
 * Four tanks flagged as sidemount, two on gas mix 2 that dropped 39 bar each,
 * and two on gas mix 1 that dropped 20.9 bar each: which tanks make up a pair
 * isn't known, so each counts on its own. Counted together, the four would have
 * lost less than twice as much on gas mix 2 as on gas mix 1, and none would be
 * linked.
 */
void TestTankGasGuess::moreThanTwoSidemount()
{
	trace t;
	t.breathe(0, 40, 2);
	t.breathe(40, 60, 1);
	t.reading(0, 0, 40, 200000, 1000);
	t.reading(0, 40, 60, 161000, 0);
	t.reading(1, 0, 40, 200000, 1000);
	t.reading(1, 40, 60, 161000, 0);
	t.reading(2, 0, 40, 200000, 0);
	t.reading(2, 40, 60, 200000, 1100);
	t.reading(3, 0, 40, 200000, 0);
	t.reading(3, 40, 60, 200000, 1100);
	QCOMPARE(t.guess({ true, true, true, true }), std::vector<int>({ 2, 2, 1, 1 }));
}

/**
 * Two tanks flagged as sidemount, but breathed on different gas mixes: 44 bar on
 * gas mix 0 from one and 21 bar on gas mix 1 from the other. Counted together,
 * both would go to gas mix 0, which contradicts the second tank on its own, so
 * each keeps its own. The same goes if the second tank lost too little to be
 * linked on its own.
 */
void TestTankGasGuess::sidemountOnDifferentGases()
{
	trace t;
	t.breathe(0, 45, 0);
	t.breathe(45, 60, 1);
	t.reading(0, 0, 45, 200000, 1000);
	t.reading(0, 45, 60, 156000, 0);
	t.reading(1, 0, 45, 200000, 0);
	t.reading(1, 45, 60, 200000, 1500);
	QCOMPARE(t.guess(sidemount_pair), std::vector<int>({ 0, 1, -1, -1 }));

	// With ≈19 bar on gas mix 1, the second tank isn't linked on its own, but it
	// lost more on gas mix 1 than on gas mix 0, so it doesn't go to gas mix 0
	trace below;
	below.breathe(0, 45, 0);
	below.breathe(45, 60, 1);
	below.reading(0, 0, 45, 200000, 1000);
	below.reading(0, 45, 60, 156000, 0);
	below.reading(1, 0, 46, 200000, 0);
	below.reading(1, 46, 60, 200000, 1450);
	QCOMPARE(below.guess(sidemount_pair), std::vector<int>({ 0, -1, -1, -1 }));
}

/**
 * The limits are inclusive: exactly the minimum use, and exactly twice the use
 * on the next gas mix, still get a link; any less doesn't.
 */
void TestTankGasGuess::inclusiveLimits()
{
	trace t;
	t.breathe(0, 30, 1);
	t.breathe(30, 60, 0);
	// 20 bar on gas mix 0
	t.reading(0, 0, 31, 200000, 0);
	t.reading(0, 31, 60, 180000, 0);
	// 10 bar on gas mix 1, and 20 bar on gas mix 0
	t.reading(1, 0, 1, 200000, 0);
	t.reading(1, 1, 31, 190000, 0);
	t.reading(1, 31, 60, 170000, 0);
	// 1 mbar less than 20 bar on gas mix 0
	t.reading(2, 0, 31, 200000, 0);
	t.reading(2, 31, 60, 180001, 0);
	// 1 mbar more than 10 bar on gas mix 1, and 20 bar on gas mix 0
	t.reading(3, 0, 1, 200000, 0);
	t.reading(3, 1, 31, 189999, 0);
	t.reading(3, 31, 60, 169999, 0);
	QCOMPARE(t.guess(), std::vector<int>({ 0, 0, -1, -1 }));
}

/**
 * A drop across missing readings, during which the gas mix changed and changed
 * back, can't be put down to either gas mix.
 */
void TestTankGasGuess::gasChangeDuringMissingReadings()
{
	trace t;
	t.breathe(0, 60, 0);
	t.breathe(20, 40, 1);
	t.reading(0, 0, 10, 200000, 0);
	t.reading(0, 50, 60, 170000, 0);
	QCOMPARE(t.guess(), std::vector<int>({ -1, -1, -1, -1 }));

	// If the gas mix stays the same across the missing readings, the drop counts
	trace same;
	same.breathe(0, 60, 1);
	same.reading(0, 0, 10, 200000, 0);
	same.reading(0, 50, 60, 170000, 0);
	QCOMPARE(same.guess(), std::vector<int>({ 1, -1, -1, -1 }));
}

/**
 * Without gas mixes, or with the gas mix not known, there is nothing to link to.
 */
void TestTankGasGuess::noGasMixes()
{
	trace t;
	t.reading(0, 0, 60, 200000, 1000);
	QCOMPARE(t.guess(), std::vector<int>({ -1, -1, -1, -1 }));
	t.breathe(0, 60, 0);
	QCOMPARE(t.guess(std::vector<bool>(ntanks, false), 0), std::vector<int>({ -1, -1, -1, -1 }));
}

/**
 * How apply_tank_gas_guesses() adds the guesses to the links from the dive
 * computer: it keeps those links, leaves out excluded tanks, and changes
 * nothing if the guesses only confirm the order of the tanks.
 */
void TestTankGasGuess::applyGuesses()
{
	const std::vector<dc_tank_usage_t> none(3, DC_TANK_USAGE_NONE);

	// A link from the dive computer is kept, even if the guess differs
	std::vector<int> links = { 2, -1, -1 };
	QVERIFY(apply_tank_gas_guesses(links, { 0, 1, 1 }, none, 3));
	QCOMPARE(links, std::vector<int>({ 2, 1, 1 }));

	// An excluded tank, e.g. a buddy's, isn't guessed
	links = { -1, -1 };
	QVERIFY(apply_tank_gas_guesses(links, { 1, 0 }, { DC_TANK_USAGE_NONE, DC_TANK_USAGE_EXCLUDED }, 2));
	QCOMPARE(links, std::vector<int>({ 1, -1 }));

	// Guesses that only confirm the order of the tanks change nothing
	links = { -1, -1 };
	QVERIFY(!apply_tank_gas_guesses(links, { 0, 1 }, none, 2));
	QCOMPARE(links, std::vector<int>({ -1, -1 }));

	// Neither does no guess at all
	QVERIFY(!apply_tank_gas_guesses(links, { -1, -1 }, none, 2));
	QCOMPARE(links, std::vector<int>({ -1, -1 }));
}

QTEST_GUILESS_MAIN(TestTankGasGuess)
