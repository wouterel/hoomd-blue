import hoomd
import pytest
import numpy as np

_harmonic_args = {'k': [30.0, 25.0, 20.0], 'r0': [1.6, 1.7, 1.8]}
_harmonic_arg_list = [(hoomd.md.mesh.bond.Harmonic,
                       dict(zip(_harmonic_args, val)))
                      for val in zip(*_harmonic_args.values())]

_FENE_args = {
    'k': [30.0, 25.0, 20.0],
    'r0': [1.6, 1.7, 1.8],
    'epsilon': [0.9, 1.0, 1.1],
    'sigma': [1.1, 1.0, 0.9]
}
_FENE_arg_list = [(hoomd.md.mesh.bond.FENE, dict(zip(_FENE_args, val)))
                  for val in zip(*_FENE_args.values())]

_Tether_args = {
    'k_b': [5.0, 6.0, 7.0],
    'l_min': [0.7, 0.8, 0.9],
    'l_c1': [0.9, 1.05, 1.1],
    'l_c0': [1.1, 1.1, 1.3],
    'l_max': [1.3, 1.3, 1.5]
}
_Tether_arg_list = [(hoomd.md.mesh.bond.Tether, dict(zip(_Tether_args, val)))
                    for val in zip(*_Tether_args.values())]


def get_mesh_bond_and_args():
    return _harmonic_arg_list + _FENE_arg_list + _Tether_arg_list


def get_mesh_bond_args_forces_and_energies():
    harmonic_forces1 = [[-28.395, 16.393861, 0], [-27.4125, 15.826614, 0],
                        [-24.93, 14.393342, 0]]
    harmonic_forces2 = [[0, -32.787722, 0], [0, -31.653229, 0],
                        [0, -28.786684, 0]]
    harmonic_forces3 = [[28.395, 16.393861, 0], [27.4125, 15.826614, 0],
                        [24.93, 14.393342, 0]]
    harmonic_energies = [17.9172, 20.0385, 20.7168]
    FENE_forces1 = [[-165.834803, 95.744768, 0], [-9.719869, 5.611769, 0],
                    [33.483261, -19.331569, 0]]
    FENE_forces2 = [[0, -191.489537, 0], [0., -11.223537, 0], [0, 38.663139, 0]]
    FENE_forces3 = [[165.834803, 95.744768, 0], [9.719869, 5.611769, 0],
                    [-33.483261, -19.331569, 0]]
    FENE_energies = [82.0225, 48.6153, 33.4625]
    Tether_forces1 = [[0, 0, 0], [-0.036666, 0.021169, 0],
                      [-5.358389, 3.093667, 0]]
    Tether_forces2 = [[0, 0, 0], [0, -0.042339, 0], [0, -6.187334, 0]]
    Tether_forces3 = [[0, 0, 0], [0.036666, 0.021169, 0],
                      [5.358389, 3.093667, 0]]
    Tether_energies = [0, 0.000463152, 0.1472802]

    harmonic_args_and_vals = []
    FENE_args_and_vals = []
    Tether_args_and_vals = []
    for i in range(3):
        harmonic_args_and_vals.append(
            (*_harmonic_arg_list[i], harmonic_forces1[i], harmonic_forces2[i],
             harmonic_forces3[i], harmonic_energies[i]))
        FENE_args_and_vals.append(
            (*_FENE_arg_list[i], FENE_forces1[i], FENE_forces2[i],
             FENE_forces3[i], FENE_energies[i]))
        Tether_args_and_vals.append(
            (*_Tether_arg_list[i], Tether_forces1[i], Tether_forces2[i],
             Tether_forces3[i], Tether_energies[i]))
    return harmonic_args_and_vals + FENE_args_and_vals + Tether_args_and_vals


@pytest.fixture(scope='session')
def triplet_snapshot_factory(device):

    def make_snapshot(d=1.0,
                      theta_deg=60,
                      particle_types=['A'],
                      dimensions=3,
                      L=20):
        theta_rad = theta_deg * (np.pi / 180)
        s = hoomd.Snapshot(device.communicator)
        N = 3
        if s.communicator.rank == 0:
            box = [L, L, L, 0, 0, 0]
            if dimensions == 2:
                box[2] = 0
            s.configuration.box = box
            s.particles.N = N

            base_positions = np.array(
                [[-d * np.sin(theta_rad / 2), d * np.cos(theta_rad / 2), 0.0],
                 [0.0, 0.0, 0.0],
                 [d * np.sin(theta_rad / 2), d * np.cos(theta_rad / 2), 0.0]])
            # move particles slightly in direction of MPI decomposition which
            # varies by simulation dimension
            nudge_dimension = 2 if dimensions == 3 else 1
            base_positions[:, nudge_dimension] += 0.1
            s.particles.position[:] = base_positions
            s.particles.types = particle_types
        return s

    return make_snapshot


@pytest.mark.parametrize("mesh_bond_cls, potential_kwargs",
                         get_mesh_bond_and_args())
def test_before_attaching(mesh_bond_cls, potential_kwargs):
    mesh = hoomd.mesh.Mesh()
    mesh_bond_potential = mesh_bond_cls(mesh)
    mesh_bond_potential.parameter = potential_kwargs

    assert mesh is mesh_bond_potential.mesh
    for key in potential_kwargs:
        np.testing.assert_allclose(mesh_bond_potential.parameter[key],
                                   potential_kwargs[key],
                                   rtol=1e-6)

    mesh1 = hoomd.mesh.Mesh()
    mesh_bond_potential.mesh = mesh1
    assert mesh1 is mesh_bond_potential.mesh


@pytest.mark.parametrize("mesh_bond_cls, potential_kwargs",
                         get_mesh_bond_and_args())
def test_after_attaching(triplet_snapshot_factory, simulation_factory,
                         mesh_bond_cls, potential_kwargs):
    snap = triplet_snapshot_factory(d=0.969, L=5)
    sim = simulation_factory(snap)

    mesh = hoomd.mesh.Mesh()
    mesh.size = 1
    mesh.triangles = [[0, 1, 2]]

    mesh_bond_potential = mesh_bond_cls(mesh)
    mesh_bond_potential.parameter = potential_kwargs

    integrator = hoomd.md.Integrator(dt=0.005)

    integrator.forces.append(mesh_bond_potential)

    langevin = hoomd.md.methods.Langevin(kT=1,
                                         filter=hoomd.filter.All(),
                                         alpha=0.1)
    integrator.methods.append(langevin)
    sim.operations.integrator = integrator

    sim.run(0)
    for key in potential_kwargs:
        np.testing.assert_allclose(mesh_bond_potential.parameter[key],
                                   potential_kwargs[key],
                                   rtol=1e-6)


@pytest.mark.parametrize(
    "mesh_bond_cls, potential_kwargs, force1, force2, force3, energy",
    get_mesh_bond_args_forces_and_energies())
def test_forces_and_energies(triplet_snapshot_factory, simulation_factory,
                             mesh_bond_cls, potential_kwargs, force1, force2,
                             force3, energy):
    snap = triplet_snapshot_factory(d=0.969, L=5)
    sim = simulation_factory(snap)

    mesh = hoomd.mesh.Mesh()
    mesh.size = 1
    mesh.triangles = [[0, 1, 2]]

    mesh_bond_potential = mesh_bond_cls(mesh)
    mesh_bond_potential.parameter = potential_kwargs

    integrator = hoomd.md.Integrator(dt=0.005)

    integrator.forces.append(mesh_bond_potential)

    langevin = hoomd.md.methods.Langevin(kT=1,
                                         filter=hoomd.filter.All(),
                                         alpha=0.1)
    integrator.methods.append(langevin)
    sim.operations.integrator = integrator

    sim.run(0)

    sim_energies = sim.operations.integrator.forces[0].energies
    sim_forces = sim.operations.integrator.forces[0].forces
    if sim.device.communicator.rank == 0:
        np.testing.assert_allclose(sum(sim_energies),
                                   energy,
                                   rtol=1e-2,
                                   atol=1e-5)
        np.testing.assert_allclose(sim_forces[0], force1, rtol=1e-2, atol=1e-5)
        np.testing.assert_allclose(sim_forces[1], force2, rtol=1e-2, atol=1e-5)
        np.testing.assert_allclose(sim_forces[2], force3, rtol=1e-2, atol=1e-5)