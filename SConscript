#!/usr/bin/env python
#  This file is part of LOOS.
#
#  LOOS (Lightweight Object-Oriented Structure library)
#  Copyright (c) 2008-2009, Tod D. Romo
#  Department of Biochemistry and Biophysics
#  School of Medicine & Dentistry, University of Rochester
#
#  This package (LOOS) is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation under version 3 of the License.
#
#  This package is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.


import sys
import os

Import('env')


reparse = env['reparse']
if int(reparse):
   apps = 'grammar.yy scanner.ll '
else:
   apps = 'scanner.cc grammar.cc '


apps = apps + 'dcd.cpp utils.cpp dcd_utils.cpp pdb_remarks.cpp pdb.cpp psf.cpp KernelValue.cpp ensembles.cpp dcdwriter.cpp Fmt.cpp'
apps = apps + ' AtomicGroup.cpp AG_numerical.cpp AG_linalg.cpp Geometry.cpp amber.cpp amber_traj.cpp tinkerxyz.cpp sfactories.cpp'
apps = apps + ' ccpdb.cpp pdbtraj.cpp tinker_arc.cpp ProgressCounters.cpp Atom.cpp KernelActions.cpp'
apps = apps + ' Kernel.cpp KernelStack.cpp ProgressTriggers.cpp Selectors.cpp XForm.cpp amber_rst.cpp'
apps = apps + ' xtc.cpp gro.cpp trr.cpp MatrixOps.cpp'
apps = apps + ' charmm.cpp AtomicNumberDeducer.cpp OptionsFramework.cpp revision.cpp'
apps = apps + ' utils_random.cpp utils_structural.cpp LineReader.cpp'


loos = env.SharedLibrary('libloos', Split(apps))

# Handle installation...
PREFIX = env['PREFIX']

# Library(ies)
loos_lib_inst = env.Install(os.path.join(PREFIX, 'lib'), loos)

# Setup environment script(s)

script_sh = env.Scripts('setup.sh', 'setup.sh-pre')
script_csh = env.Scripts('setup.csh', 'setup.csh-pre')
scripts = [ script_sh, script_csh ]

script_sh_inst = env.Scripts(os.path.join(PREFIX, 'setup.sh'), 'setup.sh-pre')
script_csh_inst = env.Scripts(os.path.join(PREFIX, 'setup.csh'), 'setup.csh-pre')
scripts_inst = [ script_sh_inst, script_csh_inst ]


# Header files...
hdr = 'loos.hpp'
hdr = 'amber.hpp amber_rst.hpp amber_traj.hpp Atom.hpp AtomicGroup.hpp ccpdb.hpp Coord.hpp'
hdr = hdr + ' cryst.hpp dcd.hpp dcd_utils.hpp dcdwriter.hpp ensembles.hpp Fmt.hpp'
hdr = hdr + ' Geometry.hpp KernelActions.hpp Kernel.hpp KernelStack.hpp'
hdr = hdr + ' KernelValue.hpp loos_defs.hpp loos.hpp LoosLexer.hpp Matrix44.hpp'
hdr = hdr + ' Matrix.hpp MatrixImpl.hpp MatrixIO.hpp MatrixOrder.hpp MatrixRead.hpp'
hdr = hdr + ' MatrixStorage.hpp MatrixUtils.hpp MatrixWrite.hpp ParserDriver.hpp'
hdr = hdr + ' Parser.hpp pdb.hpp pdb_remarks.hpp pdbtraj.hpp PeriodicBox.hpp psf.hpp'
hdr = hdr + ' Selectors.hpp sfactories.hpp StreamWrapper.hpp timer.hpp'
hdr = hdr + ' TimeSeries.hpp tinker_arc.hpp tinkerxyz.hpp Trajectory.hpp'
hdr = hdr + ' UniqueStrings.hpp utils.hpp XForm.hpp ProgressCounters.hpp ProgressTriggers.hpp'
hdr = hdr + ' grammar.hh location.hh position.hh stack.hh FlexLexer.h'
hdr = hdr + ' xdr.hpp xtc.hpp gro.hpp trr.hpp exceptions.hpp MatrixOps.hpp sorting.hpp'
hdr = hdr + ' Simplex.hpp charmm.hpp AtomicNumberDeducer.hpp OptionsFramework.hpp'
hdr = hdr + ' utils_random.hpp utils_structural.hpp LineReader.hpp'

loos_hdr_inst = env.Install(os.path.join(PREFIX,'include'), Split(hdr))

env.Alias('lib_install', [loos_lib_inst, loos_hdr_inst, scripts_inst])


# Python bindings
loos_python = env.SharedLibrary('_loos', ['loos.i', loos])



Return('loos','loos_python', 'scripts')
