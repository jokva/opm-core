/*===========================================================================
//
// File: CSRMatrixUmfpackSolver.hpp
//
// Created: 2011-10-03 17:27:26+0200
//
// Authors: Ingeborg S. Ligaarden <Ingeborg.Ligaarden@sintef.no>
//          Jostein R. Natvig     <Jostein.R.Natvig@sintef.no>
//          Halvor M. Nilsen      <HalvorMoll.Nilsen@sintef.no>
//          Atgeirr F. Rasmussen  <atgeirr@sintef.no>
//          Bård Skaflestad       <Bard.Skaflestad@sintef.no>
//
//==========================================================================*/


/*
  Copyright 2011 SINTEF ICT, Applied Mathematics.
  Copyright 2011 Statoil ASA.

  This file is part of the Open Porous Media Project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef OPM_CSRMATRIXUMFPACKSOLVER_HPP_HEADER
#define OPM_CSRMATRIXUMFPACKSOLVER_HPP_HEADER

#include <opm/core/linalg/call_umfpack.h>
#include <opm/core/utility/ErrorMacros.hpp>

namespace Opm
{
    namespace ImplicitTransportLinAlgSupport
    {

        class CSRMatrixUmfpackSolver
        {
        public:


            template <class Vector>
            void
            solve(const struct CSRMatrix* A,
                  const Vector            b,
                  Vector                  x)
            {
#if HAVE_SUITESPARSE_UMFPACK_H
                call_UMFPACK(const_cast<CSRMatrix*>(A), b, x);
#else
    OPM_THROW(std::runtime_error, "Cannot use implicit transport solver without UMFPACK. "
          "Reconfigure opm-core with SuiteSparse/UMFPACK support and recompile.");
#endif
            }


            template <class Vector>
            void
            solve(const struct CSRMatrix& A,
                  const Vector&           b,
                  Vector&                 x)
            {
#if HAVE_SUITESPARSE_UMFPACK_H
                call_UMFPACK(const_cast<CSRMatrix*>(&A), &b[0], &x[0]);
#else
    OPM_THROW(std::runtime_error, "Cannot use implicit transport solver without UMFPACK. "
          "Reconfigure opm-core with SuiteSparse/UMFPACK support and recompile.");
#endif
            }


        }; // class CSRMatrixUmfpackSolver

    } // namespace ImplicitTransportLinAlgSupport
} // namespace Opm

#endif  /* OPM_CSRMATRIXUMFPACKSOLVER_HPP_HEADER */
