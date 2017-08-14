# Portions Copyright (c) 2008 Nokia Corporation
import unittest, sys
from test import test_support

class LongExpText(unittest.TestCase):
    def test_longexp(self):
        # Increasing the expression multiplier above 1700 causes a stack
        # overflow on the Symbian platform. Setting a safe value of 1500.
        if sys.platform == 'symbian_s60':
            REPS = 1500
        else:
            REPS = 65580
        l = eval("[" + "2," * REPS + "]")
        self.assertEqual(len(l), REPS)

def test_main():
    test_support.run_unittest(LongExpText)

if __name__=="__main__":
    test_main()
