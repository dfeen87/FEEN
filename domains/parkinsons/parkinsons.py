"""
Domain_Parkinsons Module
========================

Expands the Adaptive Coherence Modulator (ACM) framework within the FEEN architecture.
Parkinson's Disease (PD) is modeled as a topological wave-state failure within the 
Neuro-Coherence Function (M) rather than a simple neurotransmitter deficiency.

The ACM is designed as a state-dependent scaffold aiming for intentional 
pharmacological obsolescence.
"""

import math

class DomainParkinsons:
    """
    Models the Parkinsonian motor loop and its restoration via an Adaptive Coherence Modulator (ACM).
    """

    def __init__(self):
        # Initialize Unicode math variables for formal physics notation

        self.Δ_GR: float = 0.0
        """Regional Variance: Represents pathological beta-band oscillations (tremors)."""

        self.Λ: float = 0.0
        """Phase Alignment: Represents the synchronization between the Substantia Nigra and Striatum."""

        self.Θ: float = 0.0
        """Thermodynamic Stability: Represents localized mitochondrial ATP efficiency."""

        self.Γ: float = 0.0
        """Adaptive Gain: Represents neuroplastic responsiveness in the motor circuit."""

        self.Φ: float = 0.0
        """External Influence: Represents the active dosage of the ACM scaffold."""

        self.initialize_parkinsonian_state()

    @property
    def M(self) -> float:
        """
        Neuro-Coherence Function (M): The total system stability.
        Derived as a function of phase alignment (Λ), thermodynamic stability (Θ),
        adaptive gain (Γ), inversely proportional to regional variance (Δ_GR).
        """
        # Ensure denominator is never zero or negative in an undefined way
        variance_penalty = max(0.01, 1.0 + self.Δ_GR)
        return (self.Λ * self.Θ * (1.0 + self.Γ)) / variance_penalty

    def initialize_parkinsonian_state(self) -> None:
        """
        Sets the system to a chaotic attractor state (High Δ_GR, Low Λ, Low Θ).
        """
        self.Δ_GR = 0.85  # High regional variance (tremors active)
        self.Λ = 0.15     # Low phase alignment (desynchronized SN-Striatum)
        self.Θ = 0.20     # Low thermodynamic stability (ATP depletion)
        self.Γ = 0.05     # Minimal adaptive gain (low neuroplasticity)
        self.Φ = 0.0      # No external influence initially

    def apply_phase1_scaffold(self, external_influence: float) -> None:
        """
        Simulates the "Variance Damper." 
        Mathematically reduces Δ_GR (dampening beta-oscillations) without crashing Θ.
        
        Args:
            external_influence (float): The active dosage (Φ) applied to the system.
        """
        self.Φ = external_influence
        
        # Exponential damping of the regional variance based on external influence
        self.Δ_GR = self.Δ_GR * math.exp(-0.8 * self.Φ)
        
        # The scaffold draws minimal thermodynamic load, so Θ is slightly perturbed but not crashed.
        # It remains above a critical baseline.
        self.Θ = max(0.1, self.Θ - 0.05 * self.Φ)

  def apply_phase2_tutoring(self, external_influence: float) -> None:
        """
        Simulates "Resonant Tutoring." 
        Aggressively elevates Γ, but accounts for the biological ATP cost (Θ).
        
        Args:
            external_influence (float): The active dosage (Φ) applied to the system.
        """
        self.Φ = external_influence
        
        # 1. The ACM injects thermodynamic support (ATP efficiency)
        gross_thermo_support = 0.6 * self.Φ 
        
        # 2. The ACM drives neuroplastic growth
        plasticity_growth = 0.5 * self.Φ
        self.Γ += plasticity_growth
        
        # 3. THE METABOLIC PENALTY: Neuroplasticity costs ATP.
        # We subtract a metabolic cost based on how much growth is occurring.
        metabolic_cost = plasticity_growth * 0.4
        
        # 4. Net Thermodynamic Stability
        self.Θ += (gross_thermo_support - metabolic_cost)
        
        # Tutoring aligns the phases over time
        self.Λ += 0.4 * self.Φ
        
        # Bound variables to reasonable physiological limits
        self.Γ = min(self.Γ, 1.0)
        self.Θ = min(self.Θ, 1.0)
        self.Λ = min(self.Λ, 1.0)

    def execute_phase3_taper(self, steps: int = 10) -> None:
        """
        Progressively reduces Φ to 0. 
        Simulates the physiological decay or removal of the ACM scaffold.
        The maintenance of system state depends strictly on the achieved Adaptive Gain (Γ).
        
        Args:
            steps (int): The number of tapering steps to reduce Φ to 0.
        """
        if self.Φ <= 0:
            return
            
        step_size = self.Φ / steps
        
        for _ in range(steps):
            self.Φ -= step_size
            if self.Φ < 0:
                self.Φ = 0.0
                
            # If neuroplasticity (Γ) is high, the degradation rate is low to zero.
            # If Γ is low, the system falls back towards chaos.
            degradation_factor = max(0.0, 1.0 - self.Γ)
            
            # The system naturally degrades if it lacks neuroplasticity
            self.Λ = max(0.15, self.Λ - 0.05 * degradation_factor)
            self.Θ = max(0.20, self.Θ - 0.05 * degradation_factor)
            
            # Tremors return if the system loses coherence
            self.Δ_GR = min(0.85, self.Δ_GR + 0.1 * degradation_factor)

    def verify_systemic_independence(self) -> bool:
        """
        Checks if the total M function and Λ (Phase Alignment) remain stable after Φ reaches 0.
        If it crashes back to the baseline state, it flags the intervention as a 
        "Conventional Suppressor" rather than a true ACM.
        
        Returns:
            bool: True if the ACM successfully restored the motor loop (Stable), 
                  False if it acted merely as a conventional suppressor (Crash).
        """
        # A true ACM maintains a high M and Λ despite Φ being 0.
        # If it crashes, M will be low (approaching the original state).
        
        baseline_M = (0.15 * 0.20 * 1.05) / 1.85 # baseline M is approx 0.017
        
        is_stable = self.M > (baseline_M * 5) and self.Λ >= 0.4
        
        if not is_stable:
            # Flags as conventional suppressor
            return False
        
        return True
