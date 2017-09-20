#!/usr/bin/python
"""
Track a set of contacts through a trajectory.  Intended for use with a protein
or RNA, to track all residue-residue contacts within the trajectory.
"""

"""

  This file is part of LOOS.

  LOOS (Lightweight Object-Oriented Structure library)
  Copyright (c) 2013 Tod Romo, Grossfield Lab
  Department of Biochemistry and Biophysics
  School of Medicine & Dentistry, University of Rochester

  This package (LOOS) is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation under version 3 of the License.

  This package is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""


import loos
import loos.pyloos
import sys
import numpy
from os.path import basename, splitext
import argparse

def fullhelp():
  print """
  Insert fullhelp here
  """

class FullHelp(argparse.Action):
    def __init__(self, option_strings, dest, nargs=None, **kwargs):
        kwargs['nargs']=0
        super(FullHelp, self).__init__(option_strings, dest, **kwargs)
    def __call__(self, parser, namespace, values, option_string = None):
        fullhelp()
        parser.print_help()
        setattr(namespace, self.dest, True)
        parser.exit()

##############################################################################

cmd_args = " ".join(sys.argv)
parser = argparse.ArgumentParser(description="Track residue-residue contacts")
parser.add_argument('system_file', help="File describing the system")
parser.add_argument('selection', help="Selection string describing which residues to use")
parser.add_argument('out_file', help="File with the average contact occupancies")
parser.add_argument('traj_files', type=argparse.FileType('r'), nargs='+')


parser.add_argument('--cutoff', help="Cutoff distance for contact", default=4.0)
# TODO: add a number of contacts option
parser.add_argument('--no_hydrogens', nargs=0, help="Don't include hydrogens")
parser.add_argument('--no_backbone', nargs=0, help="Don't include the backbone")
parser.add_argument('--fullhelp', help="Print detailed description of all options",
                    action=FullHelp)
args = parser.parse_args()


header =  " ".join(sys.argv) + "\n"


system = loos.createSystem(args.system_file)
all_trajs = []
out_names = []
num_trajs = len(args.traj_files)
for t in args.traj_files:
    traj = loos.pyloos.Trajectory(t, system)
    all_trajs.append(traj)
    if num_trajs > 1:
        t_base = basename(t)
        core,ext = splitext(t_base)
        out_names.append(core + ".dat")

if args.no_hydrogens:
    no_hydrogens = loos.selectAtoms(system, "!hydrogen")
    target = loos.selectAtoms(no_hydrogens, args.selection)
else:
    target = loos.selectAtoms(system, selection)




residues = target.splitByResidue()
# now remove the backbone -- doing before the split loses the glycines
# Woohoo, look at me, I used a lambda!
if args.no_backbone:
    residues = list(map(lambda r:loos.selectAtoms(r, "!backbone"), residues))

frac_contacts = numpy.zeros([len(residues), len(residues), num_trajs],
                            numpy.float)


for traj_id in range(num_trajs):
    traj = all_trajs[traj_id]
    for frame in traj:
        for i in range(len(residues)):
            for j in range(i+1, len(residues)):
                if residues[i].contactWith(args.cutoff, residues[j]):
                    frac_contacts[i,j,traj_id] += 1.0
                    frac_contacts[j,i,traj_id] += 1.0
    frac_contacts[:,:,traj_id] /= len(traj)
    if num_trajs > 1:
        numpy.savetxt(out_names[traj_id], frac_contacts[:,:,traj_id],
                      header=header)



average = numpy.add.reduce(frac_contacts, axis=2)
average /= len(args.traj_files)


numpy.savetxt(args.out_file, average, header = header)
