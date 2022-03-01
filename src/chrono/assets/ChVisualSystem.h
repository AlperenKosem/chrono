// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2022 projectchrono.org
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Radu Serban
// =============================================================================

#ifndef CH_VISUAL_SYSTEM_H
#define CH_VISUAL_SYSTEM_H

#include <vector>

#include "chrono/core/ChApiCE.h"
#include "chrono/physics/ChPhysicsItem.h"

namespace chrono {

// Forward references
class ChSystem;

class ChApi ChVisualSystem {
  public:
    virtual ~ChVisualSystem() {}

    /// Process all visual assets in the associated ChSystem.
    /// This function is called by default when the visualization system is attached to a Chrono system (using
    /// ChSystem::SetVisualSystem()), but can also be called later if further modifications to visualization assets
    /// occur.
    virtual void BindAll() {}

    /// Process the visual assets for the spcified physics item.
    /// This function must be called if a new physics item is added to the system or if changes to its visual model
    /// occur after the visualization system was attached to the Chrono system.
    virtual void BindItem(std::shared_ptr<ChPhysicsItem> item) {}

    /// Create a snapshot of the last rendered frame and save it to the provided file.
    /// The file extension determines the image format.
    virtual void WriteImageToFile(const std::string& filename) {}

  protected:
    ChVisualSystem();

    /// Perform any additional setup operations when the visualization system is attached to a ChSystem.
    virtual void OnAttach(ChSystem* sys);

    /// Update the visualization system at the current time step.
    /// Called by the associated ChSystem.
    virtual void Update() {}

    ChSystem* m_system;  ///< associated Chrono system

    friend class ChSystem;
};

}  // namespace chrono

#endif