# EdgeSense

EdgeSense is a portfolio-ready simulation of an embedded sensor node with edge anomaly inference. A C++ virtual MCU polls synthetic sensors, serializes telemetry into a UART-style binary frame, and streams it over TCP to a Python gateway. The gateway validates and decodes frames, applies a scikit-learn `IsolationForest`, and exposes recent readings through FastAPI.

## Architecture

```
virtual_mcu (C++) -- binary framed TCP --> gateway (Python) --> FastAPI /readings
      |                                              |
  sensor polling + CRC                         decode + ML inference
```

### Protocol (little-endian)

Each frame is `SOF(0xAA55) | version | type | payload_length | payload | CRC16-CCITT`.
Telemetry payload: `node_id:u16, sequence:u32, timestamp_ms:u64, temperature_c:f32, humidity_pct:f32, vibration_g:f32`.

## Quick start

### Python gateway/API

```bash
python -m venv .venv
.venv\\Scripts\\activate       # Windows
pip install -r gateway/requirements.txt
uvicorn gateway.app:app --app-dir gateway --host 127.0.0.1 --port 8000
```

The TCP gateway listens on `127.0.0.1:9000`. Browse `http://127.0.0.1:8000/docs`.

### Virtual firmware

```bash
cmake -S firmware -B build
cmake --build build
./build/edgesense_firmware --host 127.0.0.1 --port 9000 --count 30
```

On Windows, the executable will be `build/Debug/edgesense_firmware.exe` or `build/Release/edgesense_firmware.exe` depending on the generator.

## Tests

```bash
PYTHONPATH=gateway pytest gateway/tests -q
cmake -S firmware -B build -DEDGESENSE_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

The C++ tests use GoogleTest. CMake discovers an installed GTest package first; otherwise it fetches GoogleTest during configuration.

## Layout

- `firmware/` — portable C++ virtual MCU, codec, CRC and GoogleTest protocol tests
- `gateway/` — TCP decoder, IsolationForest inference, FastAPI endpoints and pytest integration tests
- `docker-compose.yml` — optional gateway container startup

## Example API response

```json
{
  "node_id": 7,
  "sequence": 42,
  "temperature_c": 24.1,
  "humidity_pct": 48.2,
  "vibration_g": 0.08,
  "anomaly": false,
  "anomaly_score": 0.12
}
```
