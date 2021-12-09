// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Authors: Simone Benatti
// =============================================================================
//
// Structures for links, contacts, body properties in PBD systems and their lists
//
// =============================================================================

#ifndef CH_PBD_SHAFTCOUPLE_H
#define CH_PBD_SHAFTCOUPLE_H

#include "chrono/physics/ChSystem.h"
#include "chrono/physics/ChShaft.h"
#include "chrono/physics/ChShaftsGear.h"

namespace chrono {
	
	// Forward references
	class ChSystemPBD;

	/// Struct collecting a Chrono ChLink together with additional info needed by 
	class ChApi ChPBDShaftsCouple {
	public:

	  /// Create a LinkPBD
      ChPBDShaftsCouple(ChSystemPBD* sys, ChShaft* shafta, ChShaft* shaftb)
          : shaft1(shafta), shaft2(shaftb), PBDsys(sys){};

		/// Copy constructor
		//ChLinkPBD(const ChLinkPBD& other);

		/// Destructor
        //virtual ~ChShaftsCouplePBD() {};
	  
	  // Will evaluate the violation and apply torque on the connected elements. Inherited classes will implement it. 
	  virtual void SolveShaftCoupling() = 0;

    protected:
        ChShaft* shaft1;
        ChShaft* shaft2;
        ChSystemPBD* PBDsys;
		// shaft couplings in PBD are violation-based like links, just 1-dimensional
        double alpha = 0;
        double lambda = 0;
		
	 
	};

//CH_CLASS_VERSION(ChPBDShaftsCouple, 0)

	class ChApi ChPBDShaftsCoupleGear : public ChPBDShaftsCouple {
      public:
        /// Create a LinkPBD
        ChPBDShaftsCoupleGear(ChSystemPBD* sys, ChShaftsGear* gear);
		// Will evaluate the violation and apply torque on the connected elements. Inherited classes will implement it.
        void SolveShaftCoupling() override;

      private:
        ChShaftsGear* shaftGear;
		  
    };

	
}  // end namespace chrono

#endif