/*
  This file is part of LOOS.

  LOOS (Lightweight Object-Oriented Structure library)
  Copyright (c) 2008, Tod D. Romo, Alan Grossfield
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
*/

#include <tinker_arc.hpp>
#include <AtomicGroup.hpp>

namespace loos {

  void TinkerArc::init(void) {
    char buf[512];

    // Read the first frame to get the # of atoms...
    frame.read(*(ifs));
    _natoms = frame.size();
    indices.push_back(0l);
    cached_first = true;

    // Now determine the # of frames...
    while (!ifs->eof()) {
      indices.push_back(ifs->tellg());
      ifs->getline(buf, sizeof(buf));
      for (uint i=0; i<_natoms; ++i)
        ifs->getline(buf, sizeof(buf));
    }

    _nframes = indices.size() - 1;

    ifs->clear();
    ifs->seekg(indices[1]);
  }


  void TinkerArc::seekNextFrameImpl(void) {
    if (++current_index >= _nframes)
      at_end = true;
  }


  void TinkerArc::seekFrameImpl(const uint i) {
    if (i >= _nframes)
      throw(FileError(_filename, "Requested trajectory frame is out of range"));

    ifs->clear();
    ifs->seekg(indices[i]);
    if (ifs->fail())
      throw(FileError(_filename, "Cannot seek to the requested frame"));

    current_index = i;
    at_end = false;
  }


  bool TinkerArc::parseFrame(void) {
    if (ifs->eof() || at_end)
      return(false);

    TinkerXYZ newframe;
    newframe.read(*(ifs));
    frame = newframe;
    if (frame.size() == 0) {
      at_end = true;
      return(false);
    }

    return(true);
  }


  std::vector<GCoord> TinkerArc::coords(void) const {
    std::vector<GCoord> result(_natoms);

    for (uint i=0; i<_natoms; i++)
      result[i] = frame[i]->coords();

    return(result);
  }


  void TinkerArc::updateGroupCoordsImpl(AtomicGroup& g) 
  {
    for (AtomicGroup::iterator i = g.begin(); i != g.end(); ++i) {
      uint idx = (*i)->index();
      if (idx >= _natoms)
	throw(LOOSError(**i, "Atom index into the trajectory frame is out of bounds"));
      (*i)->coords(frame[idx]->coords());
    }
    
    // Handle periodic boundary conditions (if present)
    if (hasPeriodicBox()) {
      g.periodicBox(periodicBox());
    }
  }
  
}
