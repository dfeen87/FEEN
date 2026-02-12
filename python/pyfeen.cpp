#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <feen/resonator.h>

namespace py = pybind11;
using namespace feen;

PYBIND11_MODULE(pyfeen, m) {
    m.doc() = "FEEN: Phononic Wave Engine Python Bindings";

    py::class_<ResonatorConfig>(m, "ResonatorConfig")
        .def(py::init<>())
        .def_readwrite("name", &ResonatorConfig::name)
        .def_readwrite("frequency_hz", &ResonatorConfig::frequency_hz)
        .def_readwrite("q_factor", &ResonatorConfig::q_factor)
        .def_readwrite("beta", &ResonatorConfig::beta)
        .def_readwrite("sustain_s", &ResonatorConfig::sustain_s);

    py::class_<Resonator>(m, "Resonator")
        .def(py::init<const ResonatorConfig&>())
        .def("inject", &Resonator::inject)
        .def("tick", &Resonator::tick)
        .def("energy", &Resonator::total_energy)
        .def("snr", &Resonator::snr)
        .def("x", &Resonator::x)
        .def("v", &Resonator::v)
        .def("t", &Resonator::t);
}
