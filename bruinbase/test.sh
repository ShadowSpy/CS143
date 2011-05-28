#!/bin/sh

rm -f xsmall.tbl xsmall.idx
rm -f small.tbl small.idx
rm -f medium.tbl medium.idx
rm -f large.tbl large.idx
rm -f xlarge.tbl xlarge.idx

./bruinbase < test.sql

