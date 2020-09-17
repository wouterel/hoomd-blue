from itertools import chain
import hoomd.integrate
from hoomd.syncedlist import SyncedList
from hoomd.operation import _Analyzer, _Updater, _Tuner
from hoomd.typeconverter import OnlyType
from hoomd.tune import ParticleSorter


def _triggered_op_conversion(value):
    return (value._cpp_obj, value.trigger)


class Operations:
    def __init__(self, simulation=None):
        self._simulation = simulation
        self._compute = list()
        self._scheduled = False
        self._updaters = SyncedList(OnlyType(_Updater),
                                    _triggered_op_conversion)
        self._analyzers = SyncedList(OnlyType(_Analyzer),
                                     _triggered_op_conversion)
        self._tuners = SyncedList(OnlyType(_Tuner), lambda x: x._cpp_obj)
        self._integrator = None

        self._tuners.append(ParticleSorter())

    def add(self, op):
        # calling _add is handled by the synced lists and integrator property.
        # we raise this error here to provide a more clear error message.
        if op._added:
            raise RuntimeError(
                "Operation cannot be added to twice to operation lists.")
        if isinstance(op, hoomd.integrate._BaseIntegrator):
            self.integrator = op
            return None
        elif isinstance(op, _Tuner):
            self._tuners.append(op)
        elif isinstance(op, _Updater):
            self._updaters.append(op)
        elif isinstance(op, _Analyzer):
            self._analyzers.append(op)
        else:
            raise ValueError("Operation is not of the correct type to add to"
                             " Operations.")

    @property
    def _sys_init(self):
        if self._simulation is None or self._simulation.state is None:
            return False
        else:
            return True

    def schedule(self):
        if not self._sys_init:
            raise RuntimeError("System not initialized yet")
        sim = self._simulation
        if not (self.integrator is None or self.integrator._attached):
            self.integrator._attach()
        if not self.updaters._synced:
            self.updaters._sync(sim, sim._cpp_sys.updaters)
        if not self.analyzers._synced:
            self.analyzers._sync(sim, sim._cpp_sys.analyzers)
        if not self.tuners._synced:
            self.tuners._sync(sim, sim._cpp_sys.tuners)
        self._scheduled = True

    def unschedule(self):
        self._integrator._detach()
        self._analyzers._unsync()
        self._updaters._unsync()
        self._tuners._unsync()
        self._scheduled = False

    def _store_reader(self, reader):
        # TODO
        pass

    def __contains__(self, obj):
        return any(op is obj for op in self)

    def __iter__(self):
        if self._integrator is not None:
            yield from chain((self._integrator,), self._analyzers,
                             self._updaters, self._tuners)
        else:
            yield from chain((self._analyzers, self._updaters, self._tuners))

    @property
    def scheduled(self):
        return self._scheduled

    @property
    def integrator(self):
        return self._integrator

    @integrator.setter
    def integrator(self, op):
        if op._added:
            raise RuntimeError(
                "Integrator cannot be added to twice to Operations objects.")
        else:
            op._add(self._simulation)

        if (not isinstance(op, hoomd.integrate._BaseIntegrator)
                and op is not None):
            raise TypeError("Cannot set integrator to a type not derived "
                            "from hoomd.integrate._BaseIntegrator")
        old_ref = self.integrator
        self._integrator = op
        if self._scheduled:
            if op is not None:
                op._attach()
        if old_ref is not None:
            old_ref._notify_disconnect(self._simulation)
            old_ref._detach()
            old_ref._remove()

    @property
    def updaters(self):
        return self._updaters

    @property
    def analyzers(self):
        return self._analyzers

    @property
    def tuners(self):
        return self._tuners

    def __iadd__(self, operation):
        self.add(operation)

    def remove(self, operation):
        if isinstance(operation, hoomd.integrate._BaseIntegrator):
            raise ValueError(
                "Cannot remove iterator without setting to a new integator.")
        elif isinstance(operation, _Analyzer):
            self._analyzers.remove(operation)
        elif isinstance(operation, _Updater):
            self._updaters.remove(operation)
        elif isinstance(operation, _Tuner):
            self._tuners.remove(operation)

    def __isub__(self, operation):
        self.remove(operation)
