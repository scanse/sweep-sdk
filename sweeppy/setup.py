#!/usr/bin/env python

import os.path

from setuptools import setup, find_packages

with open(os.path.join(os.path.dirname(__file__), 'README.md')) as f:
    readme_content = f.read()

setup(name='sweeppy',
      version='1.1',
      description='Python bindings for libsweep',
      long_description=readme_content,
      author='Scanse',
      url='http://scanse.io',
      packages=find_packages(exclude=['tests']),
      license='MIT',
      classifiers=[
          'Development Status :: 3 - Alpha',

          'Intended Audience :: Developers',

          'License :: OSI Approved :: MIT License',

          'Programming Language :: Python :: 2',
          'Programming Language :: Python :: 2.6',
          'Programming Language :: Python :: 2.7',
          'Programming Language :: Python :: 3',
          'Programming Language :: Python :: 3.2',
          'Programming Language :: Python :: 3.3',
          'Programming Language :: Python :: 3.4',
          'Programming Language :: Python :: 3.5',
          'Programming Language :: Python :: 3.6',

          'Operating System :: Unix',
      ],
      keywords='libsweep sweep sweeppy scanse')
