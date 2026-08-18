import pytest

from gateway.protocol import FrameDecoder, Telemetry, encode_telemetry


def test_decoder_handles_fragmented_frame():
    source = Telemetry(7, 11, 1_700_000_000_000, 24.5, 49.2, 0.09)
    frame = encode_telemetry(source)
    decoder = FrameDecoder()
    assert decoder.feed(frame[:5]) == []
    decoded = decoder.feed(frame[5:])
    assert len(decoded) == 1
    assert decoded[0].node_id == source.node_id
    assert decoded[0].sequence == source.sequence
    assert decoded[0].temperature_c == pytest.approx(source.temperature_c)
    assert decoded[0].humidity_pct == pytest.approx(source.humidity_pct)
    assert decoded[0].vibration_g == pytest.approx(source.vibration_g)


def test_decoder_discards_bad_crc_and_resyncs():
    one = encode_telemetry(Telemetry(1, 1, 1, 20.0, 40.0, 0.1))
    corrupt = bytearray(one)
    corrupt[-1] ^= 0xFF
    two_reading = Telemetry(2, 2, 2, 21.0, 41.0, 0.2)
    assert FrameDecoder().feed(b"noise" + bytes(corrupt) + encode_telemetry(two_reading)) == [two_reading]
