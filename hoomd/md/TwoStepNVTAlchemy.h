
// Copyright (c) 2009-2016 The Regents of the University of Michigan
// This file is part of the HOOMD-blue project, released under the BSD 3-Clause License.

// Maintainer: jproc

#ifndef __TWOSTEP_NVT_ALCHEMY_H__
#define __TWOSTEP_NVT_ALCHEMY_H__


#include "AlchemostatTwoStep.h"
#include "hoomd/Variant.h"

/*! \file TwoStepNVTAlchemy.h
    \brief Declares the TwoStepNVTAlchemy class
*/

#ifdef NVCC
#error This header cannot be compiled by nvcc
#endif

#include <hoomd/extern/nano-signal-slot/nano_signal_slot.hpp>
#include <pybind11/pybind11.h>

//! Integrates part of the system forward in two steps in the NVE ensemble
/*! Implements NVT digital alchemy integration through the IntegrationMethodTwoStep interface

    \ingroup updaters
*/
class TwoStepNVTAlchemy : public AlchemostatTwoStep
    {
    public:
    //! Constructs the integration method and associates it with the system
    TwoStepNVTAlchemy(std::shared_ptr<SystemDefinition> sysdef, std::shared_ptr<Variant> T);
    virtual ~TwoStepNVTAlchemy();

    void setQ(Scalar Q)
        {
        m_Q = Q;
        }
        
    /// get the Q value
    Scalar getQ()
        {
        return m_Q;
        }

    void setT(std::shared_ptr<Variant> T)
        {
        m_T = T;
        }
        
    /// Get the current temperature variant
    std::shared_ptr<Variant> getT()
        {
        return m_T;
        }
        
    // static unsigned int getIntegraorNDOF()
    // {
    // return 1;
    // }

    //! Performs the first step of the integration
    void integrateStepOne(uint64_t timestep) override;

    //! Performs the second step of the integration
    void integrateStepTwo(uint64_t timestep) override;

    private:
    Scalar m_Q;
    Scalar m_alchem_KE;
    std::shared_ptr<Variant> m_T;
    unsigned int m_iteratorDOF = 1;
    
    //! advance the thermostat
    /*!\param timestep The time step
     * \param broadcast True if we should broadcast the integrator variables via MPI
     TODO: implement mpi support
     */
    void advanceThermostat(uint64_t timestep, bool broadcast = true);
    };

//! Exports the TwoStepNVTAlchemy class to python
void export_TwoStepNVTAlchemy(pybind11::module& m);

#endif // __TWOSTEP_NVT_ALCHEMY_H__