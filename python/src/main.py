#!/usr/bin/python3

################################################################################
# triplet_challenge is an application that extracts the top 3 triplet words of a
# file
#
# Copyright (C) 2018 Carlos Fuentes
#
# This file is part of triplet_challenge.
#
# triplet_challenge is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# triplet_challenge is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with triplet_challenge.  If not, see <http://www.gnu.org/licenses/>.
################################################################################

import re
import sys
from collections import Counter

def main(argv):
  if len(argv) != 2:
    print('main.py <file>')
    return -1

  with open(argv[1], 'r') as f:
    data = f.read().lower()
    data = re.sub('[^0-9a-z\s\']', ' ', data, flags=re.M)
    data = re.sub('\s+', ' ', data, flags=re.M)

    words = data.split(' ')

    triplets = []
    for i in range(0, len(words) - 3):
      key = ' '.join(words[i:i+3])
      triplets.append(key)

    c = Counter(triplets)
    for key, count in c.most_common(3):
      print(f'{key} - {count}')

  return 0

if __name__ == '__main__':
  sys.exit(main(sys.argv))