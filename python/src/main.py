#!/usr/bin/python3

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