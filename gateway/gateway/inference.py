from __future__ import annotations

import numpy as np
from sklearn.ensemble import IsolationForest


class AnomalyDetector:
    """Deterministic lightweight edge-AI model trained on nominal sensor ranges."""
    def __init__(self) -> None:
        rng = np.random.default_rng(7)
        normal = np.column_stack((rng.normal(24.0, 1.2, 500), rng.normal(48.0, 4.0, 500), rng.normal(0.08, 0.025, 500)))
        self.model = IsolationForest(n_estimators=100, contamination=0.04, random_state=7)
        self.model.fit(normal)

    def assess(self, temperature_c: float, humidity_pct: float, vibration_g: float) -> tuple[bool, float]:
        features = [[temperature_c, humidity_pct, vibration_g]]
        # Higher is more suspicious for an API-friendly score.
        score = float(-self.model.decision_function(features)[0])
        anomaly = bool(self.model.predict(features)[0] == -1)
        return anomaly, round(score, 5)
