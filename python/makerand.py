#!/bin/python
import argparse
import random


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog='makerand',
        description='Generate random natural numbers')

    parser.add_argument('--nums', type=int, default=10,
                        help='Number to generate')
    args = parser.parse_args()
    for _ in range(args.nums):
        print(random.randint(0, 2**32-1))
