#
#
# ppm_ticks_header_generator.py
#
# Copyright (C) 2025  Barnard33
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# This code is only intended to be used for educational purposes.
# It is not production stable and must under no circumstance be used
# in any kind of radio controlled machinery, e.g., planes, cars, boats, etc.
#
# Created: 2025-03-23 17:24
#
#


# Converts a servo pulse in milliseconds to timer ticks.
# The timer clock is assumed to be prescaled by factor 8.
# Should be scaled by 1.2 but 1.13 works better. May be this is due to the
# overhead for simulating a 16 bit counter with a 8 bit counter.
def to_ticks(pulse_ms):
    return int (pulse_ms * 1.13) 
    
print("/* This file is genereated - do not change it manually! */")
print("#ifndef __ppm_ticks__")
print("#define __ppm_ticks__")
print("#define IN_MIN_TICKS", to_ticks(1000))
print("#define IN_MAX_TICKS", to_ticks(2000))
print("#define MID_TICKS", to_ticks(1500))
print("#define OUT_MIN_TICKS", to_ticks(550))
print("#define OUT_MAX_TICKS", to_ticks(2650))
print("#endif")

