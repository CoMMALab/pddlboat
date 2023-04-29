#!/usr/bin/env python

from distutils.core import setup

setup(
    name='pddlboat',
    version='0.1.0',
    description='PDDL parsing, manipulation, and planning',
    author='Zachary Kingston',
    author_email='zak@rice.edu',
    packages=['pddlboat'],
    package_data={'pddlboat': ['*.so', '*.pyi']}
)
