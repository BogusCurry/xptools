// Copyright (c) 2003,2004  INRIA Sophia-Antipolis (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org); you may redistribute it under
// the terms of the Q Public License version 1.0.
// See the file LICENSE.QPL distributed with CGAL.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL: svn+ssh://scm.gforge.inria.fr/svn/cgal/branches/CGAL-3.3-branch/Apollonius_graph_2/include/CGAL/Apollonius_graph_2/uncertain/Uncertain_is_hidden_C2.h $
// $Id$
// 
//
// Author(s)     : Menelaos Karavelas <mkaravel@cse.nd.edu>



#ifndef CGAL_APOLLONIUS_GRAPH_2_UNCERTAIN_IS_HIDDEN_C2_H
#define CGAL_APOLLONIUS_GRAPH_2_UNCERTAIN_IS_HIDDEN_C2_H

#include <CGAL/enum.h>
#include <CGAL/Uncertain.h>
#include <CGAL/number_type_basic.h>


CGAL_BEGIN_NAMESPACE

//--------------------------------------------------------------------


template<class K, class MTag>
class Ag2_uncertain_is_hidden_C2
{
public:
  typedef K                    Kernel;
  typedef MTag                 Method_tag;
  typedef typename K::Site_2   Site_2;
  typedef typename K::RT       RT;

private:
  Uncertain<bool> is_hidden(const Site_2& p, const Site_2& q,
			    const Integral_domain_without_division_tag&) const
  {
    RT w1 = p.weight();
    RT w2 = q.weight();
    Uncertain<Sign> s = CGAL::sign( CGAL::square(p.x() - q.x())
				    + CGAL::square(p.y() - q.y())
				    - CGAL::square(w1 - w2)
				    );
    if ( is_indeterminate(s) ) {
      return Uncertain<bool>::indeterminate();
    }
    if ( s == POSITIVE ) { return false; }

    Uncertain<Comparison_result> cr = CGAL::compare(w1, w2);
    if ( is_indeterminate(cr) ) {
      return Uncertain<bool>::indeterminate();
    }
    return (cr != SMALLER);
  }

  Uncertain<bool> is_hidden(const Site_2& p, const Site_2& q,
			    const Field_with_sqrt_tag&) const
  {
    RT d = CGAL::sqrt(CGAL::square(p.x() - q.x())
		      + CGAL::square(p.y() - q.y()));
    
    Uncertain<Sign> s = CGAL::sign(d - p.weight() + q.weight());
    if ( is_indeterminate(s) ) {
      return Uncertain<bool>::indeterminate();
    }
    return ( s != POSITIVE );
  }

public:
  typedef Uncertain<bool>      result_type;
  typedef Site_2               argument_type;
  typedef Arity_tag<2>         Arity;

  inline bool operator()(const Site_2 &p, const Site_2 &q) const {
#ifdef AG2_PROFILE_PREDICATES
    ag2_predicate_profiler::is_trivial_counter++;
#endif
    return is_hidden(p, q, Method_tag());
  }
};


//--------------------------------------------------------------------


CGAL_END_NAMESPACE

#endif // CGAL_APOLLONIUS_GRAPH_2_UNCERTAIN_IS_HIDDEN_C2_H