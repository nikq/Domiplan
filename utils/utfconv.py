# convert utf16 zmx to utf8
import os
import sys
import glob
import ntpath


def convert(input_name, output_name):
    with open(input_name, 'rb') as source_file:
        with open(output_name, 'w+b') as dest_file:
            contents = source_file.read()
            dest_file.write((contents).decode(
                'utf-16', 'ignore').encode('utf-8'))


files = os.listdir(sys.argv[1])
for file in files:
    print("file : " + sys.argv[1]+file)
    convert(sys.argv[1] + file, sys.argv[2] + file)
