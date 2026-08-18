from fastapi.testclient import TestClient

from gateway.app import app, service
from gateway.protocol import Telemetry


def test_sensor_to_api_pipeline():
    service.readings.clear()
    with TestClient(app) as client:
        service.ingest(Telemetry(7, 9, 1234, 25.0, 49.0, 0.08))
        response = client.get("/readings/latest")
    assert response.status_code == 200
    body = response.json()
    assert body["node_id"] == 7
    assert body["anomaly"] is False
