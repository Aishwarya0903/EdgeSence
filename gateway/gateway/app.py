from __future__ import annotations

import asyncio
import os
from collections import deque
from contextlib import asynccontextmanager
from typing import Any

from fastapi import FastAPI, HTTPException
from pydantic import BaseModel

from .inference import AnomalyDetector
from .protocol import FrameDecoder, Telemetry


class Reading(BaseModel):
    node_id: int
    sequence: int
    timestamp_ms: int
    temperature_c: float
    humidity_pct: float
    vibration_g: float
    anomaly: bool
    anomaly_score: float


class GatewayService:
    def __init__(self) -> None:
        self.detector = AnomalyDetector()
        self.readings: deque[Reading] = deque(maxlen=500)
        self.server: asyncio.AbstractServer | None = None

    def ingest(self, telemetry: Telemetry) -> Reading:
        anomaly, score = self.detector.assess(telemetry.temperature_c, telemetry.humidity_pct, telemetry.vibration_g)
        reading = Reading(**telemetry.__dict__, anomaly=anomaly, anomaly_score=score)
        self.readings.append(reading)
        return reading

    async def handle_client(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter) -> None:
        decoder = FrameDecoder()
        try:
            while packet := await reader.read(1024):
                for telemetry in decoder.feed(packet):
                    self.ingest(telemetry)
        finally:
            writer.close()
            await writer.wait_closed()

    async def start(self) -> None:
        # Container-friendly by default; bind to 127.0.0.1 with EDGESENSE_TCP_HOST for local-only use.
        self.server = await asyncio.start_server(self.handle_client, os.getenv("EDGESENSE_TCP_HOST", "0.0.0.0"), 9000)

    async def stop(self) -> None:
        if self.server:
            self.server.close()
            await self.server.wait_closed()


service = GatewayService()

@asynccontextmanager
async def lifespan(_: FastAPI):
    await service.start()
    yield
    await service.stop()


app = FastAPI(title="EdgeSense Gateway API", version="1.0.0", lifespan=lifespan)

@app.get("/health")
def health() -> dict[str, str]:
    return {"status": "ok"}

@app.get("/readings", response_model=list[Reading])
def readings(limit: int = 50) -> list[Reading]:
    if not 1 <= limit <= 500:
        raise HTTPException(422, "limit must be from 1 to 500")
    return list(service.readings)[-limit:]

@app.get("/readings/latest", response_model=Reading)
def latest() -> Reading:
    if not service.readings:
        raise HTTPException(404, "No telemetry has been received")
    return service.readings[-1]

@app.get("/metrics")
def metrics() -> dict[str, Any]:
    return {"received": len(service.readings), "anomalies": sum(item.anomaly for item in service.readings)}
