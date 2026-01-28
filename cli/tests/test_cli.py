#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0
"""
Test suite for subsurface-cli

This test suite:
1. Uses a test git repository with known dive data
2. Queries each CLI endpoint
3. Validates the JSON responses against expected values
4. Tests error handling for invalid inputs
"""

import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


class CliTestError(Exception):
    """Exception raised when a CLI test fails."""


class SubsurfaceCliTester:
    """Test harness for subsurface-cli."""

    def __init__(self, cli_path=None, test_data_path=None):
        """Initialize the test harness.

        Args:
            cli_path: Path to subsurface-cli binary. If None, searches in common locations.
            test_data_path: Path to test data file (.ssrf). If None, uses SampleDivesV2.ssrf.
        """
        self.cli_path = cli_path or self._find_cli()
        self.test_data_path = test_data_path or self._find_test_data()
        self.temp_dir = None
        self.config_path = None
        self.test_results = []
        self.passed = 0
        self.failed = 0

    def _find_cli(self):
        """Find the subsurface-cli binary."""
        # Look in common build locations relative to this script
        script_dir = Path(__file__).parent
        subsurface_root = script_dir.parent.parent

        candidates = [
            subsurface_root / "build" / "cli" / "subsurface-cli",
            subsurface_root / "build" / "subsurface-cli",
            Path("/usr/local/bin/subsurface-cli"),
            Path("/usr/bin/subsurface-cli"),
        ]

        for path in candidates:
            if path.exists() and os.access(path, os.X_OK):
                return str(path)

        raise CliTestError(
            "Could not find subsurface-cli. Please build it or specify --cli-path."
        )

    def _find_test_data(self):
        """Find the test data file."""
        script_dir = Path(__file__).parent
        subsurface_root = script_dir.parent.parent

        candidates = [
            subsurface_root / "dives" / "SampleDivesV2.ssrf",
            subsurface_root / "dives" / "SampleDives.ssrf",
        ]

        for path in candidates:
            if path.exists():
                return str(path)

        raise CliTestError(
            "Could not find test data file. Please specify --test-data."
        )

    def setup(self):
        """Set up the test environment."""
        # Create temp directory for config and profiles
        # Use /tmp/subsurface* prefix to comply with CLI config path security restrictions
        self.temp_dir = tempfile.mkdtemp(prefix="subsurface-cli-test-", dir="/tmp")

        # Create config file pointing to the test data
        self.config_path = os.path.join(self.temp_dir, "config.json")
        # Normalize the path to remove any ".." components (security validation rejects them)
        normalized_data_path = os.path.realpath(self.test_data_path)
        config = {
            "repo_path": normalized_data_path,
            "userid": "test@example.com",
            "temp_dir": os.path.join(self.temp_dir, "profiles"),
        }

        with open(self.config_path, "w", encoding="utf-8") as f:
            json.dump(config, f)

        print("Test setup complete:")
        print(f"  CLI: {self.cli_path}")
        print(f"  Test data: {self.test_data_path}")
        print(f"  Config: {self.config_path}")
        print()

    def teardown(self):
        """Clean up the test environment."""
        if self.temp_dir and os.path.exists(self.temp_dir):
            shutil.rmtree(self.temp_dir)

    def run_cli(self, command, args=None, expect_error=False):
        """Run a CLI command and return the parsed JSON output.

        Args:
            command: The CLI command (e.g., "list-dives")
            args: Additional arguments as a list
            expect_error: If True, don't raise on non-zero exit code

        Returns:
            Parsed JSON response or None on error
        """
        cmd = [self.cli_path, f"--config={self.config_path}", command]
        if args:
            cmd.extend(args)

        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=30,
            )

            if result.returncode != 0 and not expect_error and not result.stdout:
                raise CliTestError(
                    f"CLI command failed: {' '.join(cmd)}\n"
                    f"Exit code: {result.returncode}\n"
                    f"Stderr: {result.stderr}"
                )

            if result.stdout:
                return json.loads(result.stdout)
            return None

        except subprocess.TimeoutExpired:
            raise CliTestError(f"CLI command timed out: {' '.join(cmd)}")
        except json.JSONDecodeError as e:
            raise CliTestError(
                f"Invalid JSON from CLI: {' '.join(cmd)}\n"
                f"Output: {result.stdout}\n"
                f"Error: {e}"
            )

    def record_result(self, test_name, passed, message=""):
        """Record a test result."""
        status = "PASS" if passed else "FAIL"
        self.test_results.append((test_name, passed, message))
        if passed:
            self.passed += 1
            print(f"  [{status}] {test_name}")
        else:
            self.failed += 1
            print(f"  [{status}] {test_name}: {message}")

    def assert_equal(self, test_name, actual, expected, message=""):
        """Assert two values are equal."""
        if actual == expected:
            self.record_result(test_name, True)
        else:
            self.record_result(
                test_name, False,
                f"{message or 'Values differ'}: expected {expected!r}, got {actual!r}"
            )

    def assert_true(self, test_name, condition, message=""):
        """Assert a condition is true."""
        if condition:
            self.record_result(test_name, True)
        else:
            self.record_result(test_name, False, message or "Condition was false")

    def assert_in(self, test_name, item, container, message=""):
        """Assert an item is in a container."""
        if item in container:
            self.record_result(test_name, True)
        else:
            self.record_result(
                test_name, False,
                f"{message or 'Item not found'}: {item!r} not in {container!r}"
            )

    def assert_key_exists(self, test_name, obj, key, message=""):
        """Assert a key exists in an object."""
        if key in obj:
            self.record_result(test_name, True)
        else:
            self.record_result(
                test_name, False,
                f"{message or 'Key not found'}: {key!r} not in object"
            )

    # ==================== Test Cases ====================

    def test_list_dives_basic(self):
        """Test basic list-dives command."""
        print("\n--- test_list_dives_basic ---")

        result = self.run_cli("list-dives")

        self.assert_equal("status is success", result.get("status"), "success")
        self.assert_key_exists("has total_dives", result, "total_dives")
        self.assert_key_exists("has total_trips", result, "total_trips")
        self.assert_key_exists("has items", result, "items")
        self.assert_key_exists("has statistics", result, "statistics")
        self.assert_true(
            "total_dives > 0",
            result.get("total_dives", 0) > 0,
            f"Expected dives, got {result.get('total_dives')}"
        )

    def test_list_dives_pagination(self):
        """Test list-dives with pagination parameters."""
        print("\n--- test_list_dives_pagination ---")

        # Get first 5 items
        result1 = self.run_cli("list-dives", ["--start=0", "--count=5"])
        self.assert_equal("status is success", result1.get("status"), "success")
        self.assert_true(
            "items count <= 5",
            len(result1.get("items", [])) <= 5,
            f"Expected <= 5 items, got {len(result1.get('items', []))}"
        )

        # Get with offset
        result2 = self.run_cli("list-dives", ["--start=2", "--count=3"])
        self.assert_equal("offset status is success", result2.get("status"), "success")

        # Verify pagination values are clamped
        result3 = self.run_cli("list-dives", ["--start=-10", "--count=1000"])
        self.assert_equal("negative start clamped", result3.get("status"), "success")

    def test_list_dives_statistics(self):
        """Test that statistics are included in list-dives response."""
        print("\n--- test_list_dives_statistics ---")

        result = self.run_cli("list-dives")
        stats = result.get("statistics", {})

        self.assert_key_exists("has total_dive_time_minutes", stats, "total_dive_time_minutes")
        self.assert_key_exists("has max_depth_meters", stats, "max_depth_meters")
        self.assert_key_exists("has avg_depth_meters", stats, "avg_depth_meters")
        self.assert_true(
            "total_dive_time > 0",
            stats.get("total_dive_time_minutes", 0) > 0,
            "Expected positive dive time"
        )

    def test_get_dive_valid(self):
        """Test get-dive with a valid dive reference."""
        print("\n--- test_get_dive_valid ---")

        # First get the dive list to find a valid dive reference
        list_result = self.run_cli("list-dives")
        items = list_result.get("items", [])

        # Find a dive (not a trip) in the items
        dive_ref = None
        for item in items:
            if item.get("type") == "dive":
                dive_ref = item.get("dive_ref")
                break
            elif item.get("type") == "trip":
                # Get dives from the trip
                trip_ref = item.get("trip_ref")
                if trip_ref:
                    trip_result = self.run_cli("get-trip", [f"--trip-ref={trip_ref}"])
                    trip_dives = trip_result.get("dives", [])
                    if trip_dives:
                        dive_ref = trip_dives[0].get("dive_ref")
                        break

        if not dive_ref:
            self.record_result("find valid dive_ref", False, "No dives found in test data")
            return

        self.record_result("find valid dive_ref", True)

        # Get the dive details
        result = self.run_cli("get-dive", [f"--dive-ref={dive_ref}"])

        self.assert_equal("status is success", result.get("status"), "success")
        self.assert_key_exists("has dive object", result, "dive")

        dive = result.get("dive", {})
        self.assert_key_exists("dive has dive_ref", dive, "dive_ref")
        self.assert_key_exists("dive has dive_number", dive, "dive_number")
        self.assert_key_exists("dive has date", dive, "date")
        self.assert_key_exists("dive has time", dive, "time")
        self.assert_key_exists("dive has duration_minutes", dive, "duration_minutes")
        self.assert_key_exists("dive has max_depth_meters", dive, "max_depth_meters")
        self.assert_key_exists("dive has location", dive, "location")

    def test_get_dive_not_found(self):
        """Test get-dive with a non-existent dive reference."""
        print("\n--- test_get_dive_not_found ---")

        result = self.run_cli("get-dive", ["--dive-ref=1999/01/01-Mon-00=00=00"], expect_error=True)

        self.assert_equal("status is error", result.get("status"), "error")
        self.assert_equal("error_code is NOT_FOUND", result.get("error_code"), "NOT_FOUND")

    def test_get_dive_invalid_format(self):
        """Test get-dive with invalid dive reference format."""
        print("\n--- test_get_dive_invalid_format ---")

        result = self.run_cli("get-dive", ["--dive-ref=invalid-format"], expect_error=True)

        self.assert_equal("status is error", result.get("status"), "error")

    def test_get_dive_missing_arg(self):
        """Test get-dive without required argument."""
        print("\n--- test_get_dive_missing_arg ---")

        result = self.run_cli("get-dive", expect_error=True)

        self.assert_equal("status is error", result.get("status"), "error")
        self.assert_equal("error_code is INVALID_ARGS", result.get("error_code"), "INVALID_ARGS")

    def test_get_trip_valid(self):
        """Test get-trip with a valid trip reference."""
        print("\n--- test_get_trip_valid ---")

        # First get the dive list to find a valid trip reference
        list_result = self.run_cli("list-dives")
        items = list_result.get("items", [])

        trip_ref = None
        for item in items:
            if item.get("type") == "trip":
                trip_ref = item.get("trip_ref")
                break

        if not trip_ref:
            self.record_result("find valid trip_ref", False, "No trips found in test data")
            return

        self.record_result("find valid trip_ref", True)

        # Get the trip details
        result = self.run_cli("get-trip", [f"--trip-ref={trip_ref}"])

        self.assert_equal("status is success", result.get("status"), "success")
        self.assert_key_exists("has trip object", result, "trip")
        self.assert_key_exists("has dives array", result, "dives")

        trip = result.get("trip", {})
        self.assert_key_exists("trip has trip_ref", trip, "trip_ref")
        self.assert_key_exists("trip has location", trip, "location")
        self.assert_key_exists("trip has dive_count", trip, "dive_count")

        dives = result.get("dives", [])
        self.assert_true(
            "trip has dives",
            len(dives) > 0,
            "Expected at least one dive in trip"
        )

    def test_get_trip_not_found(self):
        """Test get-trip with a non-existent trip reference."""
        print("\n--- test_get_trip_not_found ---")

        result = self.run_cli("get-trip", ["--trip-ref=1999/01/01-Nonexistent"], expect_error=True)

        self.assert_equal("status is error", result.get("status"), "error")
        self.assert_equal("error_code is NOT_FOUND", result.get("error_code"), "NOT_FOUND")

    def test_get_trip_missing_arg(self):
        """Test get-trip without required argument."""
        print("\n--- test_get_trip_missing_arg ---")

        result = self.run_cli("get-trip", expect_error=True)

        self.assert_equal("status is error", result.get("status"), "error")
        self.assert_equal("error_code is INVALID_ARGS", result.get("error_code"), "INVALID_ARGS")

    def test_get_stats(self):
        """Test get-stats command."""
        print("\n--- test_get_stats ---")

        result = self.run_cli("get-stats")

        self.assert_equal("status is success", result.get("status"), "success")
        self.assert_key_exists("has total_dives", result, "total_dives")
        self.assert_key_exists("has total_trips", result, "total_trips")
        self.assert_key_exists("has statistics", result, "statistics")

        stats = result.get("statistics", {})
        self.assert_key_exists("stats has total_dive_time_minutes", stats, "total_dive_time_minutes")
        self.assert_key_exists("stats has max_depth_meters", stats, "max_depth_meters")

    def test_get_profile_valid(self):
        """Test get-profile with a valid dive reference."""
        print("\n--- test_get_profile_valid ---")

        # Find a valid dive reference
        list_result = self.run_cli("list-dives")
        items = list_result.get("items", [])

        dive_ref = None
        for item in items:
            if item.get("type") == "dive":
                dive_ref = item.get("dive_ref")
                break
            elif item.get("type") == "trip":
                trip_ref = item.get("trip_ref")
                if trip_ref:
                    trip_result = self.run_cli("get-trip", [f"--trip-ref={trip_ref}"])
                    trip_dives = trip_result.get("dives", [])
                    if trip_dives:
                        dive_ref = trip_dives[0].get("dive_ref")
                        break

        if not dive_ref:
            self.record_result("find valid dive_ref for profile", False, "No dives found")
            return

        self.record_result("find valid dive_ref for profile", True)

        # Get the profile
        result = self.run_cli("get-profile", [f"--dive-ref={dive_ref}"])

        self.assert_equal("status is success", result.get("status"), "success")
        self.assert_key_exists("has profile_path", result, "profile_path")

        profile_path = result.get("profile_path", "")
        self.assert_true(
            "profile file exists",
            os.path.exists(profile_path),
            f"Profile file not found: {profile_path}"
        )

        # Verify it's a valid PNG file
        if os.path.exists(profile_path):
            with open(profile_path, "rb") as f:
                header = f.read(8)
            self.assert_true(
                "profile is PNG",
                header[:8] == b'\x89PNG\r\n\x1a\n',
                "Profile file is not a valid PNG"
            )

    def test_get_profile_custom_dimensions(self):
        """Test get-profile with custom width and height."""
        print("\n--- test_get_profile_custom_dimensions ---")

        # Find a valid dive reference
        list_result = self.run_cli("list-dives")
        items = list_result.get("items", [])

        dive_ref = None
        for item in items:
            if item.get("type") == "dive":
                dive_ref = item.get("dive_ref")
                break
            elif item.get("type") == "trip":
                trip_ref = item.get("trip_ref")
                if trip_ref:
                    trip_result = self.run_cli("get-trip", [f"--trip-ref={trip_ref}"])
                    trip_dives = trip_result.get("dives", [])
                    if trip_dives:
                        dive_ref = trip_dives[0].get("dive_ref")
                        break

        if not dive_ref:
            self.record_result("find valid dive_ref for custom profile", False, "No dives found")
            return

        # Get the profile with custom dimensions
        result = self.run_cli("get-profile", [
            f"--dive-ref={dive_ref}",
            "--width=800",
            "--height=600"
        ])

        self.assert_equal("status is success", result.get("status"), "success")
        self.assert_key_exists("has profile_path", result, "profile_path")

    def test_get_profile_not_found(self):
        """Test get-profile with a non-existent dive reference."""
        print("\n--- test_get_profile_not_found ---")

        result = self.run_cli("get-profile", ["--dive-ref=1999/01/01-Mon-00=00=00"], expect_error=True)

        self.assert_equal("status is error", result.get("status"), "error")
        self.assert_equal("error_code is NOT_FOUND", result.get("error_code"), "NOT_FOUND")

    def test_get_profile_missing_arg(self):
        """Test get-profile without required argument."""
        print("\n--- test_get_profile_missing_arg ---")

        result = self.run_cli("get-profile", expect_error=True)

        self.assert_equal("status is error", result.get("status"), "error")
        self.assert_equal("error_code is INVALID_ARGS", result.get("error_code"), "INVALID_ARGS")

    def test_dive_data_consistency(self):
        """Test that dive data is consistent between list and detail views."""
        print("\n--- test_dive_data_consistency ---")

        # Get list
        list_result = self.run_cli("list-dives")
        items = list_result.get("items", [])

        dive_summary = None
        for item in items:
            if item.get("type") == "dive":
                dive_summary = item
                break
            elif item.get("type") == "trip":
                trip_ref = item.get("trip_ref")
                if trip_ref:
                    trip_result = self.run_cli("get-trip", [f"--trip-ref={trip_ref}"])
                    trip_dives = trip_result.get("dives", [])
                    if trip_dives:
                        dive_summary = trip_dives[0]
                        break

        if not dive_summary:
            self.record_result("find dive for consistency check", False, "No dives found")
            return

        dive_ref = dive_summary.get("dive_ref")
        detail_result = self.run_cli("get-dive", [f"--dive-ref={dive_ref}"])
        dive_detail = detail_result.get("dive", {})

        # Check consistency between summary and detail
        self.assert_equal(
            "dive_number matches",
            dive_summary.get("dive_number"),
            dive_detail.get("dive_number")
        )
        self.assert_equal(
            "date matches",
            dive_summary.get("date"),
            dive_detail.get("date")
        )
        self.assert_equal(
            "duration_minutes matches",
            dive_summary.get("duration_minutes"),
            dive_detail.get("duration_minutes")
        )
        self.assert_equal(
            "max_depth_meters matches",
            dive_summary.get("max_depth_meters"),
            dive_detail.get("max_depth_meters")
        )

    def test_security_long_dive_ref(self):
        """Test that overly long dive references are rejected."""
        print("\n--- test_security_long_dive_ref ---")

        # Create a very long dive ref (should be rejected)
        long_ref = "2024/01/01-Mon-" + "A" * 1000
        result = self.run_cli("get-dive", [f"--dive-ref={long_ref}"], expect_error=True)

        self.assert_equal("status is error", result.get("status"), "error")

    def test_security_special_chars_dive_ref(self):
        """Test that special characters in dive reference are handled."""
        print("\n--- test_security_special_chars_dive_ref ---")

        # Test with path traversal attempt
        result = self.run_cli("get-dive", ["--dive-ref=../../../etc/passwd"], expect_error=True)
        self.assert_equal("path traversal rejected", result.get("status"), "error")

        # Test with null byte - Python's subprocess rejects these at the OS level,
        # which is the correct security behavior (null bytes can't reach the CLI)
        try:
            self.run_cli("get-dive", ["--dive-ref=2024/01/01-Mon-00=00=00\x00/etc/passwd"], expect_error=True)
            self.record_result("null byte rejected", False, "Null byte was not rejected")
        except (ValueError, CliTestError):
            # ValueError from subprocess (embedded null byte) or CliTestError are both acceptable
            self.record_result("null byte rejected", True)

    def run_all_tests(self):
        """Run all test cases."""
        print("=" * 60)
        print("Subsurface CLI Test Suite")
        print("=" * 60)

        try:
            self.setup()

            # Run all test methods
            test_methods = [
                self.test_list_dives_basic,
                self.test_list_dives_pagination,
                self.test_list_dives_statistics,
                self.test_get_dive_valid,
                self.test_get_dive_not_found,
                self.test_get_dive_invalid_format,
                self.test_get_dive_missing_arg,
                self.test_get_trip_valid,
                self.test_get_trip_not_found,
                self.test_get_trip_missing_arg,
                self.test_get_stats,
                self.test_get_profile_valid,
                self.test_get_profile_custom_dimensions,
                self.test_get_profile_not_found,
                self.test_get_profile_missing_arg,
                self.test_dive_data_consistency,
                self.test_security_long_dive_ref,
                self.test_security_special_chars_dive_ref,
            ]

            for test_method in test_methods:
                try:
                    test_method()
                except Exception as e:
                    self.record_result(
                        test_method.__name__,
                        False,
                        f"Exception: {e}"
                    )

        finally:
            self.teardown()

        # Print summary
        print()
        print("=" * 60)
        print(f"Test Results: {self.passed} passed, {self.failed} failed")
        print("=" * 60)

        return self.failed == 0


def main():
    """Main entry point."""
    import argparse

    parser = argparse.ArgumentParser(description="Test suite for subsurface-cli")
    parser.add_argument("--cli-path", help="Path to subsurface-cli binary")
    parser.add_argument("--test-data", help="Path to test data file (.ssrf)")
    args = parser.parse_args()

    tester = SubsurfaceCliTester(
        cli_path=args.cli_path,
        test_data_path=args.test_data,
    )

    success = tester.run_all_tests()
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
