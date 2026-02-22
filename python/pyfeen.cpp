#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

// ------------------------------------------------------------------
// Core FEEN physics
// ------------------------------------------------------------------
#include <feen/resonator.h>
#include <feen/network.h>

// ------------------------------------------------------------------
// AILEE trust primitives
// ------------------------------------------------------------------
#include "feen/ailee/ailee_types.h"
#include "feen/ailee/confidence.h"
#include "feen/ailee/safety_gate.h"
#include "feen/ailee/consensus.h"
#include "feen/ailee/fallback.h"
#include "feen/ailee/metric.h"

namespace py = pybind11;
using namespace feen;
using namespace feen::ailee;

PYBIND11_MODULE(pyfeen, m) {
    m.doc() = "FEEN: Phononic Wave Engine with AILEE Trust Acceleration";

    // ================================================================
    // Core FEEN Physics Bindings (UNCHANGED)
    // ================================================================

    py::class_<ResonatorConfig>(m, "ResonatorConfig")
        .def(py::init<>())
        .def_readwrite("name", &ResonatorConfig::name)
        .def_readwrite("frequency_hz", &ResonatorConfig::frequency_hz)
        .def_readwrite("q_factor", &ResonatorConfig::q_factor)
        .def_readwrite("beta", &ResonatorConfig::beta)
        .def_readwrite("sustain_s", &ResonatorConfig::sustain_s);

    py::class_<Resonator>(m, "Resonator")
        .def(py::init<const ResonatorConfig&>())
        .def("inject", &Resonator::inject, py::arg("amplitude"), py::arg("phase") = 0.0)
        .def("tick", &Resonator::tick, py::arg("dt"), py::arg("F") = 0.0, py::arg("omega_d") = -1.0, py::arg("internal_force") = 0.0)
        .def("energy", &Resonator::total_energy)
        .def("snr", &Resonator::snr, py::arg("T") = ROOM_TEMP)
        .def("x", &Resonator::x)
        .def("v", &Resonator::v)
        .def("t", &Resonator::t);

    py::class_<ResonatorNetwork>(m, "ResonatorNetwork")
        .def(py::init<>())
        .def("add_node", py::overload_cast<const Resonator&>(&ResonatorNetwork::add_node))
        .def("add_coupling", &ResonatorNetwork::add_coupling,
             py::arg("i"), py::arg("j"), py::arg("strength"))
        .def("set_coupling", &ResonatorNetwork::set_coupling,
             py::arg("i"), py::arg("j"), py::arg("strength"))
        .def("coupling", &ResonatorNetwork::coupling)
        .def("clear_couplings", &ResonatorNetwork::clear_couplings)
        .def("tick_parallel", &ResonatorNetwork::tick_parallel, py::arg("dt"))
        .def("get_state_vector", &ResonatorNetwork::get_state_vector)
        .def("node", static_cast<Resonator& (ResonatorNetwork::*)(ResonatorNetwork::index_t)>(&ResonatorNetwork::node), py::return_value_policy::reference_internal)
        .def("size", &ResonatorNetwork::size)
        .def("time_s", &ResonatorNetwork::time_s)
        .def("ticks", &ResonatorNetwork::ticks);

    // ================================================================
    // AILEE Trust Primitives (Submodule)
    // ================================================================

    py::module_ ailee = m.def_submodule(
        "ailee",
        "AILEE trust primitives accelerated by FEEN"
    );

    // ----------------------------------------------------------------
    // Shared Types
    // ----------------------------------------------------------------

    py::enum_<GateState>(ailee, "GateState")
        .value("LOW_WELL", GateState::LOW_WELL)
        .value("HIGH_WELL", GateState::HIGH_WELL)
        .value("NEAR_BARRIER", GateState::NEAR_BARRIER)
        .export_values();

    py::class_<ConfidenceResult>(ailee, "ConfidenceResult")
        .def_readonly("score", &ConfidenceResult::score)
        .def_readonly("stability", &ConfidenceResult::stability)
        .def_readonly("agreement", &ConfidenceResult::agreement)
        .def_readonly("likelihood", &ConfidenceResult::likelihood);

    py::class_<SafetyGateResult>(ailee, "SafetyGateResult")
        .def_readonly("state", &SafetyGateResult::state)
        .def_readonly("margin", &SafetyGateResult::margin)
        .def_readonly("barrier_width", &SafetyGateResult::barrier_width);

    py::class_<ConsensusResult>(ailee, "ConsensusResult")
        .def_readonly("coherence", &ConsensusResult::coherence)
        .def_readonly("deviation", &ConsensusResult::deviation)
        .def_readonly("peers", &ConsensusResult::peers);

    py::class_<FallbackResult>(ailee, "FallbackResult")
        .def_readonly("value", &FallbackResult::value)
        .def_readonly("samples", &FallbackResult::samples);

    // ----------------------------------------------------------------
    // Confidence
    // ----------------------------------------------------------------

    py::class_<ConfidenceConfig>(ailee, "ConfidenceConfig")
        .def(py::init<>())
        .def_readwrite("w_stability", &ConfidenceConfig::w_stability)
        .def_readwrite("w_agreement", &ConfidenceConfig::w_agreement)
        .def_readwrite("w_likelihood", &ConfidenceConfig::w_likelihood)
        .def_readwrite("peer_delta", &ConfidenceConfig::peer_delta)
        .def_readwrite("max_abs_z", &ConfidenceConfig::max_abs_z);

    py::class_<PhononicConfidenceScorer>(ailee, "PhononicConfidenceScorer")
        .def(py::init<const ConfidenceConfig&>(),
             py::arg("config") = ConfidenceConfig{})
        .def("evaluate",
             &PhononicConfidenceScorer::evaluate,
             py::arg("raw_value"),
             py::arg("peers"),
             py::arg("history"));

    // ----------------------------------------------------------------
    // Safety Gate
    // ----------------------------------------------------------------

    py::class_<SafetyGateConfig>(ailee, "SafetyGateConfig")
        .def(py::init<>())
        .def_readwrite("barrier_center", &SafetyGateConfig::barrier_center)
        .def_readwrite("barrier_width", &SafetyGateConfig::barrier_width)
        .def_readwrite("hysteresis", &SafetyGateConfig::hysteresis)
        .def_readwrite("min_input", &SafetyGateConfig::min_input)
        .def_readwrite("max_input", &SafetyGateConfig::max_input);

    py::class_<PhononicSafetyGate>(ailee, "PhononicSafetyGate")
        .def(py::init<const SafetyGateConfig&>(),
             py::arg("config") = SafetyGateConfig{})
        .def("evaluate",
             py::overload_cast<double>(&PhononicSafetyGate::evaluate, py::const_),
             py::arg("value"))
        .def("evaluate",
             py::overload_cast<double, GateState>(&PhononicSafetyGate::evaluate, py::const_),
             py::arg("value"),
             py::arg("prior_state"));

    // ----------------------------------------------------------------
    // Consensus
    // ----------------------------------------------------------------

    py::class_<ConsensusConfig>(ailee, "ConsensusConfig")
        .def(py::init<>())
        .def_readwrite("delta", &ConsensusConfig::delta)
        .def_readwrite("coherence_floor", &ConsensusConfig::coherence_floor);

    py::class_<PhononicConsensus>(ailee, "PhononicConsensus")
        .def(py::init<const ConsensusConfig&>(),
             py::arg("config") = ConsensusConfig{})
        .def("evaluate",
             &PhononicConsensus::evaluate,
             py::arg("raw_value"),
             py::arg("peers"));

    // ----------------------------------------------------------------
    // Fallback
    // ----------------------------------------------------------------

    py::enum_<FallbackMode>(ailee, "FallbackMode")
        .value("MEDIAN", FallbackMode::MEDIAN)
        .value("MEAN", FallbackMode::MEAN)
        .value("LAST", FallbackMode::LAST)
        .export_values();

    py::class_<FallbackConfig>(ailee, "FallbackConfig")
        .def(py::init<>())
        .def_readwrite("mode", &FallbackConfig::mode)
        .def_readwrite("clamp_min", &FallbackConfig::clamp_min)
        .def_readwrite("clamp_max", &FallbackConfig::clamp_max);

    py::class_<PhononicFallback>(ailee, "PhononicFallback")
        .def(py::init<const FallbackConfig&>(),
             py::arg("config") = FallbackConfig{})
        .def("evaluate",
             &PhononicFallback::evaluate,
             py::arg("history"),
             py::arg("last_good_value") = 0.0);

    // ----------------------------------------------------------------
    // Delta v Metric
    // ----------------------------------------------------------------

    py::class_<AileeParams>(ailee, "AileeParams")
        .def(py::init<>())
        .def_readwrite("alpha", &AileeParams::alpha)
        .def_readwrite("eta", &AileeParams::eta)
        .def_readwrite("isp", &AileeParams::isp)
        .def_readwrite("v0", &AileeParams::v0);

    py::class_<AileeSample>(ailee, "AileeSample")
        .def(py::init<>())
        .def_readwrite("p_input", &AileeSample::p_input)
        .def_readwrite("workload", &AileeSample::workload)
        .def_readwrite("velocity", &AileeSample::velocity)
        .def_readwrite("mass", &AileeSample::mass)
        .def_readwrite("dt", &AileeSample::dt);

    py::class_<AileeMetric>(ailee, "AileeMetric")
        .def(py::init<const AileeParams&>())
        .def("integrate", &AileeMetric::integrate, py::arg("sample"))
        .def("delta_v", &AileeMetric::delta_v)
        .def("reset", &AileeMetric::reset);
}
