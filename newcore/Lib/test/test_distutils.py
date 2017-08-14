# Portions Copyright (c) 2008 Nokia Corporation
"""Tests for distutils.

The tests for distutils are defined in the distutils.tests package;
the test_suite() function there returns a test suite that's ready to
be run.
"""

import distutils.tests
import test.test_support
import sys


def test_main():
    if sys.platform != 'symbian_s60':
        test.test_support.run_unittest(distutils.tests.test_suite())


if __name__ == "__main__":
    test_main()
