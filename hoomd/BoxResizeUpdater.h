// Copyright (c) 2009-2019 The Regents of the University of Michigan
// This file is part of the HOOMD-blue project, released under the BSD 3-Clause License.


// Maintainer: joaander

/*! \file BoxResizeUpdater.h
    \brief Declares an updater that resizes the simulation box of the system
*/

#ifdef __HIPCC__
#error This header cannot be compiled by nvcc
#endif

#include "Updater.h"
#include "Variant.h"
#include "BoxDim.h"

#include <memory>
#include <string>
#include <stdexcept>
#include <pybind11/pybind11.h>

#ifndef __BOXRESIZEUPDATER_H__
#define __BOXRESIZEUPDATER_H__

/// Updates the simulation box over time
/** This simple updater gets the box lengths from specified variants and sets
 * those box sizes over time. As an option, particles can be rescaled with the
 * box lengths or left where they are.
 * \ingroup updaters
*/
class PYBIND11_EXPORT BoxResizeUpdater : public Updater
    {
    public:
        /// Constructor
        BoxResizeUpdater(std::shared_ptr<SystemDefinition> sysdef,
                         BoxDim box1,
                         BoxDim box2,
                         std::shared_ptr<Variant> variant);

        BoxResizeUpdater(std::shared_ptr<SystemDefinition> sysdef,
                         pybind11::object box1,
                         pybind11::object box2,
                         std::shared_ptr<Variant> variant);

        /// Destructor
        virtual ~BoxResizeUpdater();

        /// Sets particle scaling when true particles scale with box
        void setScaleParticles(bool scale_particles)
            {
            m_scale_particles = scale_particles;
            }

        /// Gets particle scaling setting
        bool getScaleParticles() {return m_scale_particles;}

        /// Set a new initial box from a python object
        void setBox1(BoxDim box);

        /// Set a new initial box from a python object
        void setBox1Py(pybind11::object box1);

        /// Get the C++ final box
        BoxDim getBox1() {return m_box1;}

        /// Get the final box
        pybind11::object getBox1Py() {return m_py_box1;}

        /// Set a new initial box from a python object
        void setBox2(BoxDim box);

        /// Set a new final box from a python object
        void setBox2Py(pybind11::object box2);

        /// Get the C++ final box
        BoxDim getBox2() {return m_box2;};

        /// Get the final box
        pybind11::object getBox2Py() {return m_py_box2;}

        /// Set the variant for interpolation
        void setVariant(std::shared_ptr<Variant> variant) {m_variant = variant;}

        /// Get the variant for interpolation
        std::shared_ptr<Variant> getVariant() {return m_variant;}

        /// Get the current box for the given timestep
        BoxDim getCurrentBox(unsigned int timestep);

        /// Determine if two boxes are essentially identical
        bool boxesAreEquivalent(BoxDim& box1, BoxDim& box2);

        /// Update box interpolation based on provided timestep
        virtual void update(unsigned int timestep);

    private:
        BoxDim m_box1;   //!< Initial box size
        pybind11::object m_py_box1;  ///< The python box1
        BoxDim m_box2;  //!< Final box size
        pybind11::object m_py_box2;  ///< The python box2
        std::shared_ptr<Variant> m_variant; //!< Variant that interpolates between boxes
        bool m_scale_particles; //!< Set to true if particle positions are to be scaled as well
    };

/// Export the BoxResizeUpdater to python
void export_BoxResizeUpdater(pybind11::module& m);

/// Get a BoxDim object from a pybind11::object or raise error
BoxDim getBoxDimFromPyObject(pybind11::object box);

#endif
