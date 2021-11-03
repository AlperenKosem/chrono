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
// Author: Milad Rakhsha
// =============================================================================
//
// Class for solving a linear linear system via iterative methods.
// =============================================================================

#ifndef CHFSILINEARSOLVER_H_
#define CHFSILINEARSOLVER_H_

#include <ctype.h>
#include <cuda_runtime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <typeinfo>
#include "cublas_v2.h"
#include "cusparse_v2.h"

namespace chrono {
namespace fsi {

/// @addtogroup fsi_solvers
/// @{

/// Base class for solving linear systems on GPUs. 
/// Specific solution methods are implemented in derived classes.
class ChFsiLinearSolver {
  public:
    /// Solver type.
    enum class SolverType { BICGSTAB, GMRES, CR, CG, SAP };

    ChFsiLinearSolver(SolverType msolver,
                      double mrel_res = 1e-8,
                      double mabs_res = 1e-4,
                      int mmax_iter = 1000,
                      bool mverbose = false) {
        rel_res = mrel_res;
        abs_res = mabs_res;
        max_iter = mmax_iter;
        verbose = mverbose;
        solver_type = msolver;
    };

    virtual ~ChFsiLinearSolver() {}

    /// Return the solver type.
    SolverType GetType() { return solver_type; }

    /// Set verbose output from solver.
    void SetVerbose(bool mv) { verbose = mv; }

    /// Return whether or not verbose output is enabled.
    bool GetVerbose() const { return verbose; }

    /// Return the current residual.
    double GetResidual() { return residual; }

    /// Return the number of current Iterations.
    int GetNumIterations() { return Iterations; }

    /// Set the maximum number of iterations.
    void SetIterationLimit(int numIter) { max_iter = numIter; }

    /// Set the maximum number of iterations.
    int GetIterationLimit() { return max_iter; }

    /// Set the absolute residual.
    void SetAbsRes(double mabs_res) { abs_res = mabs_res; }

    /// Get the absolute residual.
    double GetAbsRes() { return abs_res; }

    /// Get the relative residual.
    void SetRelRes(double mrel_res) { rel_res = mrel_res; }

    /// Get the relative residual.
    double GetRelRes() { return rel_res; }

    /// Return the solver status.
    /// - 0: unsuccessful
    /// - 1: successfully converged
    int GetSolverStatus() { return solver_status; }

    /// Solve linear system for x.
    virtual void
    Solve(int SIZE, int NNZ, double* A, unsigned int* ArowIdx, unsigned int* AcolIdx, double* x, double* b) = 0;

  protected:
    double rel_res = 1e-3;
    double abs_res = 1e-6;
    int max_iter = 500;
    bool verbose = false;
    int Iterations = 0;
    double residual = 1e5;
    int solver_status = 0;

  private:
    SolverType solver_type;
};

/// @} fsi_solvers

}  // end namespace fsi
}  // end namespace chrono

#endif
