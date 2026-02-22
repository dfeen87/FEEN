"""
FEEN-backed AILEE trust primitives.

This module provides a minimal, Pythonic façade over FEEN's
hardware-accelerated AILEE primitives. It exposes signals only.
All trust semantics and policy interpretation remain in AILEE.
"""

from pyfeen.ailee import (
    # Shared types
    GateState,
    ConfidenceResult,
    SafetyGateResult,
    ConsensusResult,
    FallbackResult,

    # Confidence
    ConfidenceConfig,
    PhononicConfidenceScorer,

    # Safety gate
    SafetyGateConfig,
    PhononicSafetyGate,

    # Consensus
    ConsensusConfig,
    PhononicConsensus,

    # Fallback
    FallbackMode,
    FallbackConfig,
    PhononicFallback,

    # Delta v Metric
    AileeParams,
    AileeSample,
    AileeMetric,
)

__all__ = [
    # Shared types
    "GateState",
    "ConfidenceResult",
    "SafetyGateResult",
    "ConsensusResult",
    "FallbackResult",

    # Confidence
    "ConfidenceConfig",
    "PhononicConfidenceScorer",

    # Safety gate
    "SafetyGateConfig",
    "PhononicSafetyGate",

    # Consensus
    "ConsensusConfig",
    "PhononicConsensus",

    # Fallback
    "FallbackMode",
    "FallbackConfig",
    "PhononicFallback",

    # Delta v Metric
    "AileeParams",
    "AileeSample",
    "AileeMetric",
]
