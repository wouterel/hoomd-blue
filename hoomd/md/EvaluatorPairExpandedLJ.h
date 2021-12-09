// Copyright (c) 2009-2021 The Regents of the University of Michigan
// This file is part of the HOOMD-blue project, released under the BSD 3-Clause License.

// Maintainer: joaander

#ifndef __PAIR_EVALUATOR_ExpandedLJ_H__
#define __PAIR_EVALUATOR_ExpandedLJ_H__

#ifndef __HIPCC__
#include <string>
#endif

#include "EvaluatorPairExpandedLJ.h"
#include "hoomd/HOOMDMath.h"

/*! \file EvaluatorPairExpandedLJ.h
    \brief Defines the pair evaluator class for Expanded LJ potentials
*/

// need to declare these class methods with __device__ qualifiers when building in nvcc
// DEVICE is __host__ __device__ when included in nvcc and blank when included into the host
// compiler
#ifdef __HIPCC__
#define DEVICE __device__
#define HOSTDEVICE __host__ __device__
#else
#define DEVICE
#define HOSTDEVICE
#endif

namespace hoomd
    {
namespace md
    {
//! Class for evaluating the Expanded LJ pair potential
/*! <b>General Overview</b>

    See EvaluatorPairExpandedLJ

    <b>ExpandedLJ specifics</b>

    EvaluatorPairExpandedLJ evaluates the function:
    \f{eqnarray*}
    V_{\mathrm{SLJ}}(r)  = & 4 \varepsilon \left[ \left( \frac{\sigma}{r - \Delta} \right)^{12} -
                           \left( \frac{\sigma}{r - \Delta} \right)^{6} \right] & r <
   (r_{\mathrm{cut}} + \Delta) \\
                         = & 0 & r \ge (r_{\mathrm{cut}} + \Delta) \\
    \f}

    The ExpandedLJ potential does not need charge or diameter. Three parameters are specified and
   stored in a Scalar3. \a lj1 is placed in \a params.x, \a lj2 is in \a params.y, and \a delta
   is in params.z.


    These are related to the standard lj parameters sigma and epsilon by:
    - \a lj1 = 4.0 * epsilon * pow(sigma,12.0);
    - \a lj2 = 4.0 * epsilon * pow(sigma,6.0);
    - \a delta = delta

    Due to the way that ExpandedLJ modifies the cutoff condition, it will not function properly with
   the xplor shifting mode.
*/
class EvaluatorPairExpandedLJ
    {
    public:
    //! Define the parameter type used by this pair potential evaluator
    struct param_type
        {
        Scalar sigma_6;
        Scalar epsilon_x_4;
        Scalar delta;

        DEVICE void load_shared(char*& ptr, unsigned int& available_bytes) { }

        HOSTDEVICE void allocate_shared(char*& ptr, unsigned int& available_bytes) const { }

#ifdef ENABLE_HIP
        //! Set CUDA memory hints
        void set_memory_hint() const
            {
            // default implementation does nothing
            }
#endif

#ifndef __HIPCC__
        param_type() : sigma_6(0), epsilon_x_4(0), delta(0) { }

        param_type(pybind11::dict v, bool managed = false)
            {
            auto sigma(v["sigma"].cast<Scalar>());
            auto epsilon(v["epsilon"].cast<Scalar>());
            auto delta(v["delta"].cast<Scalar>());

            sigma_6 = sigma * sigma * sigma * sigma * sigma * sigma;
            epsilon_x_4 = Scalar(4.0) * epsilon;

            // previous parameters
            // lj1 = 4.0 * epsilon * pow(sigma, 12.0);
            // - > lj1 = epsilon_x_4 * sigma_6 * sigma_6

            // lj2 = 4.0 * epsilon * pow(sigma, 6.0);
            // - > lj2 = epsilon_x_4 * sigma_6
            }

        // this constructor facilitates unit testing
        param_type(Scalar sigma, Scalar epsilon, Scalar delta, bool managed = false)
            {
            sigma_6 = sigma * sigma * sigma * sigma * sigma * sigma;
            epsilon_x_4 = Scalar(4.0) * epsilon;
            }

        pybind11::dict asDict()
            {
            pybind11::dict v;
            v["sigma"] = pow(sigma_6, 1. / 6.);
            v["epsilon"] = epsilon_x_4 / 4.0;
            v["delta"] = delta;
            return v;
            }
#endif
        }

        //! Constructs the pair potential evaluator
        /*! \param _rsq Squared distance between the particles
            \param _rcutsq Squared distance at which the potential goes to 0
            \param _params Per type pair parameters of this potential
        */
        DEVICE
        EvaluatorPairExpandedLJ(Scalar _rsq, Scalar _rcutsq, const param_type& _params)
            : rsq(_rsq), rcutsq(_rcutsq),
              lj1(_params.epsilon_x_4 * _params.sigma_6 * _params.sigma_6),
              lj2(_params.epsilon_x_4 * _params.sigma_6), delta(_params.delta)
        {
        }

    //! ExpandedLJ does not use diameter
    DEVICE static bool needsDiameter()
        {
        return false;
        }
    //! Accept the optional diameter values
    /*! \param di Diameter of particle i
        \param dj Diameter of particle j
    */
    DEVICE void setDiameter(Scalar di, Scalar dj) const { }

    //! ExpandedLJ does not use charge
    DEVICE static bool needsCharge()
        {
        return false;
        }
    //! Accept the optional diameter values
    /*! \param qi Charge of particle i
        \param qj Charge of particle j
    */
    DEVICE void setCharge(Scalar qi, Scalar qj) const { }

    //! Evaluate the force and energy
    /*! \param force_divr Output parameter to write the computed force divided by r.
        \param pair_eng Output parameter to write the computed pair energy
        \param energy_shift If true, the potential must be shifted so that V(r) is continuous at the
       cutoff \note There is no need to check if rsq < rcutsq in this method. Cutoff tests are
       performed in PotentialPair.

        \return True if they are evaluated or false if they are not because we are beyond the cutoff
    */
    DEVICE bool evalForceAndEnergy(Scalar& force_divr, Scalar& pair_eng, bool energy_shift)
        {
        // precompute some quantities
        Scalar rinv = fast::rsqrt(rsq);
        Scalar r = Scalar(1.0) / rinv;
        Scalar rcutinv = fast::rsqrt(rcutsq);
        Scalar rcut = Scalar(1.0) / rcutinv;

        // compute the force divided by r in force_divr
        if (r < rcut && lj1 != 0)
            {
            Scalar rmd = r - delta;
            Scalar rmdinv = Scalar(1.0) / rmd;
            Scalar rmd2inv = rmdinv * rmdinv;
            Scalar rmd6inv = rmd2inv * rmd2inv * rmd2inv;
            force_divr
                = rinv * rmdinv * rmd6inv * (Scalar(12.0) * lj1 * rmd6inv - Scalar(6.0) * lj2);

            pair_eng = rmd6inv * (lj1 * rmd6inv - lj2);

            if (energy_shift)
                {
                Scalar rcut2inv = rcutinv * rcutinv;
                Scalar rcut6inv = rcut2inv * rcut2inv * rcut2inv;
                pair_eng -= rcut6inv * (lj1 * rcut6inv - lj2);
                }
            return true;
            }
        else
            return false;
        }

#ifndef __HIPCC__
    //! Get the name of this potential
    /*! \returns The potential name. Must be short and all lowercase, as this is the name energies
       will be logged as via analyze.log.
     */
    static std::string getName()
        {
        return std::string("expanded_lj");
        }

    std::string getShapeSpec() const
        {
        throw std::runtime_error("Shape definition not supported for this pair potential.");
        }
#endif

    protected:
    Scalar rsq;    //!< Stored rsq from the constructor
    Scalar rcutsq; //!< Stored rcutsq from the constructor
    Scalar lj1;    //!< lj1 parameter extracted from the params passed to the constructor
    Scalar lj2;    //!< lj2 parameter extracted from the params passed to the constructor
    Scalar delta;  //!< outward radial shift to apply to LJ potential
    };

    } // end namespace md
    } // end namespace hoomd

#endif // __PAIR_EVALUATOR_EXPANDEDLJ_H__
