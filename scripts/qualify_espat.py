#!/usr/bin/env python3
"""Safely qualify one USB-connected ESP-AT development modem on Linux."""

from __future__ import annotations

import argparse
import dataclasses
import getpass
import glob
import json
import os
import pathlib
import re
import select
import stat
import sys
import termios
import time
from typing import Iterable

ESPRESSIF_VID = "303a"
KNOWN_ESP_BRIDGES = {("1a86", "55d3"): "WCH USB serial bridge commonly used on ESP development boards"}
BAUD_RATES = (115200, 230400, 460800, 921600, 57600, 38400, 19200, 9600)
LINE_TERMINATIONS = ("\r\n", "\r")
FINAL_RESULTS = {"OK", "ERROR", "FAIL", "SEND OK", "SEND FAIL", "ALREADY CONNECTED"}
URC_PREFIXES = (
    "+IPD,", "WIFI ", "+CWJAP:", "+STA_", "+DIST_STA_IP:",
    "CONNECT", "CLOSED", "ready", "busy ",
)


class QualificationError(RuntimeError):
    """Expected, actionable qualification failure."""


class ProbeError(QualificationError):
    """A bounded probe failed; retain its harmless attempts as evidence."""

    def __init__(self, message: str, attempts: list[dict[str, object]]):
        super().__init__(message)
        self.attempts = attempts


@dataclasses.dataclass(frozen=True)
class SerialInterface:
    device: str
    sysfs_tty: str
    usb_path: str
    vid: str
    pid: str
    manufacturer: str
    product: str
    serial: str
    interface: str
    identity_basis: str
    by_id: tuple[str, ...]
    readable: bool
    writable: bool
    mode: str

    def public(self) -> dict[str, object]:
        return dataclasses.asdict(self)


@dataclasses.dataclass
class ParsedResponse:
    raw: str
    lines: list[str]
    echo: list[str]
    results: list[str]
    urcs: list[str]
    data: list[str]
    prompts: list[str]
    timed_out: bool

    @property
    def outcome(self) -> str:
        if "OK" in self.results or "SEND OK" in self.results:
            return "pass"
        if self.results:
            return "fail"
        return "inconclusive" if self.timed_out else "fail"

    def public(self, secrets: Iterable[str] = ()) -> dict[str, object]:
        return {
            "outcome": self.outcome,
            "lines": [concise_line(redact(line, secrets)) for line in self.lines[:20]],
            "results": self.results,
            "urcs": [concise_line(redact(line, secrets)) for line in self.urcs[:20]],
            "data": [concise_line(redact(line, secrets)) for line in self.data[:20]],
            "prompts": self.prompts,
            "timed_out": self.timed_out,
        }


def redact(text: str, secrets: Iterable[str]) -> str:
    result = text
    for secret in sorted({value for value in secrets if value}, key=len, reverse=True):
        result = result.replace(secret, "<redacted>")
    result = re.sub(r'AT\+CWJAP(?:_CUR)?="(?:[^"\\]|\\.)*","(?:[^"\\]|\\.)*"',
                    'AT+CWJAP="<redacted>","<redacted>"', result)
    return result


def concise_line(line: str) -> str:
    """Retain useful text while replacing binary/mismatched-baud noise."""
    if not line:
        return line
    bad = sum(char == "\ufffd" or (ord(char) < 0x20 and char != "\t") for char in line)
    if bad / len(line) > 0.10:
        return f"<undecodable serial data: {len(line)} characters>"
    return line[:512] + ("<truncated>" if len(line) > 512 else "")


def parse_response(raw: bytes | bytearray | str, command: str = "", timed_out: bool = False) -> ParsedResponse:
    text = bytes(raw).decode("utf-8", "replace") if isinstance(raw, (bytes, bytearray)) else raw
    prompts = [">"] if re.search(r"(?:^|[\r\n])> ?", text) else []
    normalized = text.replace("\r\n", "\n").replace("\r", "\n")
    lines = [line.strip() for line in normalized.split("\n") if line.strip() and line.strip() != ">"]
    echo, results, urcs, data = [], [], [], []
    for line in lines:
        if command and line == command:
            echo.append(line)
        elif line in FINAL_RESULTS or line.startswith("ERR CODE:"):
            results.append(line)
        elif line.startswith("+IPD,"):
            urcs.append(line)
            if ":" in line:
                data.append(line.split(":", 1)[1])
        elif line.startswith(URC_PREFIXES):
            urcs.append(line)
        else:
            data.append(line)
    return ParsedResponse(text, lines, echo, results, urcs, data, prompts, timed_out)


def _read_attr(path: pathlib.Path, name: str) -> str:
    try:
        return (path / name).read_text(encoding="utf-8", errors="replace").strip()
    except (FileNotFoundError, PermissionError, OSError):
        return ""


def _usb_parent(tty_path: pathlib.Path) -> pathlib.Path | None:
    current = tty_path.resolve()
    for parent in (current, *current.parents):
        if (parent / "idVendor").is_file() and (parent / "idProduct").is_file():
            return parent
    return None


def _by_id_links(device: str) -> tuple[str, ...]:
    links = []
    for link_name in glob.glob("/dev/serial/by-id/*"):
        try:
            if os.path.realpath(link_name) == device:
                links.append(link_name)
        except OSError:
            continue
    return tuple(sorted(links))


def discover() -> list[SerialInterface]:
    interfaces = []
    for tty_class in sorted(pathlib.Path("/sys/class/tty").glob("ttyACM*")) + sorted(pathlib.Path("/sys/class/tty").glob("ttyUSB*")):
        usb = _usb_parent(tty_class)
        if usb is None:
            continue
        vid = _read_attr(usb, "idVendor").lower()
        pid = _read_attr(usb, "idProduct").lower()
        manufacturer = _read_attr(usb, "manufacturer")
        product = _read_attr(usb, "product")
        identity = f"{manufacturer} {product}".lower()
        if vid == ESPRESSIF_VID or "espressif" in identity or "esp32-c6" in identity:
            identity_basis = "Espressif USB identity"
        elif (vid, pid) in KNOWN_ESP_BRIDGES:
            identity_basis = KNOWN_ESP_BRIDGES[(vid, pid)]
        else:
            continue
        device = f"/dev/{tty_class.name}"
        try:
            device_stat = os.stat(device)
            mode = stat.filemode(device_stat.st_mode)
        except OSError:
            mode = "unavailable"
        interface_parent = tty_class.resolve().parent
        interfaces.append(SerialInterface(
            device=device,
            sysfs_tty=str(tty_class.resolve()),
            usb_path=str(usb),
            vid=vid,
            pid=pid,
            manufacturer=manufacturer,
            product=product,
            serial=_read_attr(usb, "serial"),
            interface=_read_attr(interface_parent, "interface"),
            identity_basis=identity_basis,
            by_id=_by_id_links(device),
            readable=os.access(device, os.R_OK),
            writable=os.access(device, os.W_OK),
            mode=mode,
        ))
    return interfaces


def select_endpoint(interfaces: list[SerialInterface], requested: str | None = None) -> SerialInterface:
    if not interfaces:
        raise QualificationError("no ESP32-C6/Espressif USB serial interface was found")
    physical = {item.usb_path for item in interfaces}
    if len(physical) != 1:
        raise QualificationError(f"multiple physical Espressif USB devices are present: {sorted(physical)}")
    if requested:
        real_requested = os.path.realpath(requested)
        matches = [item for item in interfaces if item.device == real_requested or requested in item.by_id]
        if len(matches) != 1:
            raise QualificationError("the requested endpoint is not exactly one discovered Espressif interface")
        selected = matches[0]
    elif len(interfaces) == 1:
        selected = interfaces[0]
    else:
        raise QualificationError("AT endpoint is ambiguous; do not probe until the operator explicitly selects one of the listed interfaces")
    if not selected.readable or not selected.writable:
        raise QualificationError(f"serial endpoint lacks read/write permission: {selected.device} ({selected.mode})")
    return selected


def _baud_constant(baud: int) -> int:
    name = f"B{baud}"
    if not hasattr(termios, name):
        raise QualificationError(f"host termios does not support {baud} baud")
    return getattr(termios, name)


class SerialPort:
    def __init__(self, device: str, baud: int):
        self.device = device
        self.baud = baud
        self.fd: int | None = None

    def __enter__(self) -> "SerialPort":
        try:
            self.fd = os.open(self.device, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
        except OSError as error:
            raise QualificationError(f"cannot open {self.device}: {error}") from error
        attrs = termios.tcgetattr(self.fd)
        attrs[0] = 0
        attrs[1] = 0
        attrs[2] = termios.CLOCAL | termios.CREAD | termios.CS8
        attrs[3] = 0
        speed = _baud_constant(self.baud)
        attrs[4] = speed
        attrs[5] = speed
        attrs[6][termios.VMIN] = 0
        attrs[6][termios.VTIME] = 0
        termios.tcsetattr(self.fd, termios.TCSANOW, attrs)
        termios.tcflush(self.fd, termios.TCIOFLUSH)
        return self

    def __exit__(self, *_: object) -> None:
        if self.fd is not None:
            os.close(self.fd)
            self.fd = None

    def exchange(self, command: str, ending: str, timeout: float = 2.0,
                 expect_prompt: bool = False, secrets: Iterable[str] = ()) -> ParsedResponse:
        assert self.fd is not None
        termios.tcflush(self.fd, termios.TCIFLUSH)
        os.write(self.fd, command.encode("utf-8") + ending.encode("ascii"))
        raw = bytearray()
        deadline = time.monotonic() + timeout
        complete = False
        while time.monotonic() < deadline:
            ready, _, _ = select.select([self.fd], [], [], min(0.1, deadline - time.monotonic()))
            if not ready:
                continue
            chunk = os.read(self.fd, 4096)
            if not chunk:
                continue
            raw.extend(chunk)
            parsed = parse_response(raw, command)
            if (expect_prompt and parsed.prompts) or parsed.results:
                complete = True
                break
        return parse_response(redact(raw.decode("utf-8", "replace"), secrets), command, not complete)

    def send_payload(self, payload: bytes, timeout: float = 8.0) -> ParsedResponse:
        assert self.fd is not None
        os.write(self.fd, payload)
        raw = bytearray()
        deadline = time.monotonic() + timeout
        complete = False
        while time.monotonic() < deadline:
            ready, _, _ = select.select([self.fd], [], [], min(0.1, deadline - time.monotonic()))
            if not ready:
                continue
            chunk = os.read(self.fd, 4096)
            if chunk:
                raw.extend(chunk)
                parsed = parse_response(raw)
                if "SEND FAIL" in parsed.results or ("SEND OK" in parsed.results and bool(parsed.data)):
                    complete = True
                    break
        return parse_response(raw, timed_out=not complete)


def probe(endpoint: SerialInterface) -> tuple[dict[str, object], list[dict[str, object]]]:
    attempts = []
    for baud in BAUD_RATES:
        for ending in LINE_TERMINATIONS:
            try:
                with SerialPort(endpoint.device, baud) as port:
                    response = port.exchange("AT", ending)
            except QualificationError as error:
                attempts.append({"baud": baud, "line_termination": repr(ending), "error": str(error)})
                continue
            attempt = {"baud": baud, "line_termination": repr(ending), "outcome": response.outcome,
                       "response": response.public()}
            attempts.append(attempt)
            if "OK" in response.results:
                return ({"baud": baud, "data_bits": 8, "parity": "none", "stop_bits": 1,
                         "flow_control": "none", "line_termination": "CRLF" if ending == "\r\n" else "CR",
                         "ending": ending}, attempts)
    raise ProbeError("no valid ESP-AT response from the bounded harmless serial configurations", attempts)


def _baseline(endpoint: SerialInterface, uart: dict[str, object]) -> dict[str, object]:
    commands = {}
    with SerialPort(endpoint.device, int(uart["baud"])) as port:
        for command in ("AT", "AT+GMR", "AT+CWMODE?", "AT+CIPSTATUS"):
            commands[command] = port.exchange(command, str(uart["ending"]), timeout=4.0).public()
    gmr_lines = commands["AT+GMR"]["lines"]
    return {
        "commands": commands,
        "identity": {
            "esp_at": next((line for line in gmr_lines if "AT version" in line), "unavailable"),
            "esp_idf": next((line for line in gmr_lines if "IDF version" in line), "unavailable"),
            "build_or_chip": [line for line in gmr_lines if re.search(r"compile|build|chip|Bin version", line, re.I)] or ["unavailable"],
        },
    }


def _at_quote(value: str) -> str:
    if any(ord(char) < 0x20 for char in value):
        raise QualificationError("credentials contain unsupported control characters")
    return value.replace("\\", "\\\\").replace('"', '\\"').replace(",", "\\,")


def wifi_tcp(endpoint: SerialInterface, uart: dict[str, object]) -> dict[str, object]:
    ssid = getpass.getpass("Wi-Fi SSID (hidden input): ")
    password = getpass.getpass("Wi-Fi passphrase (hidden input): ")
    if not ssid:
        raise QualificationError("an SSID is required; credentials were not retained")
    secrets = (ssid, password, _at_quote(ssid), _at_quote(password))
    result: dict[str, object] = {"outcome": "fail", "credentials": "ephemeral and redacted"}
    try:
        with SerialPort(endpoint.device, int(uart["baud"])) as port:
            ending = str(uart["ending"])
            mode = port.exchange("AT+CWMODE=1", ending, 4.0)
            result["station_mode"] = mode.public(secrets)
            if mode.outcome != "pass":
                return result
            join_command = f'AT+CWJAP="{_at_quote(ssid)}","{_at_quote(password)}"'
            joined = port.exchange(join_command, ending, 30.0, secrets=secrets)
            result["association"] = joined.public(secrets)
            if joined.outcome != "pass":
                return result
            ip = port.exchange("AT+CIFSR", ending, 5.0)
            ip_public = ip.public(secrets)
            ip_public["outcome"] = "pass" if any("STAIP" in line and '"0.0.0.0"' not in line for line in ip.lines) else "fail"
            result["ip_acquisition"] = ip_public
            if ip_public["outcome"] != "pass":
                return result
            result["outcome"] = "pass"
            host = input("Plain TCP echo host (non-secret): ").strip()
            port_text = input("Plain TCP echo port (non-secret): ").strip()
            payload_text = input("Plain TCP payload (non-secret): ")
            if not host or not port_text.isdigit() or not payload_text:
                result["tcp"] = {"outcome": "not-run", "reason": "host, numeric port, and payload are required"}
                return result
            tcp_port = int(port_text)
            if not 1 <= tcp_port <= 65535:
                raise QualificationError("TCP port must be between 1 and 65535")
            opened = port.exchange(f'AT+CIPSTART="TCP","{_at_quote(host)}",{tcp_port}', ending, 15.0)
            tcp: dict[str, object] = {"endpoint": {"host": host, "port": tcp_port}, "connect": opened.public()}
            result["tcp"] = tcp
            connected = any(line == "CONNECT" or line.endswith(",CONNECT") for line in opened.urcs)
            already_connected = "ALREADY CONNECTED" in opened.results
            tcp["connect"]["outcome"] = "pass" if connected or already_connected else "fail"
            if tcp["connect"]["outcome"] != "pass":
                return result
            payload = payload_text.encode("utf-8")
            prompt = port.exchange(f"AT+CIPSEND={len(payload)}", ending, 5.0, expect_prompt=True)
            tcp["send_prompt"] = prompt.public()
            if not prompt.prompts:
                return result
            sent = port.send_payload(payload)
            tcp["send"] = sent.public()
            tcp["receive"] = {"outcome": "pass" if bool(sent.data) else "fail", "data": sent.public()["data"]}
            closed = port.exchange("AT+CIPCLOSE", ending, 5.0)
            tcp["close"] = closed.public()
            tcp["outcome"] = "pass" if sent.outcome == "pass" and bool(sent.data) and closed.outcome == "pass" else "fail"
            return result
    finally:
        ssid = password = ""
        secrets = ()


def _base_record(interfaces: list[SerialInterface]) -> dict[str, object]:
    return {
        "schema": "espat-qualification-v1",
        "generated_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "discovery": {"outcome": "pass" if interfaces else "inconclusive", "interfaces": [item.public() for item in interfaces]},
        "serial_probe": {"outcome": "not-run"},
        "baseline": {"outcome": "not-run"},
        "wifi": {"outcome": "not-run"},
        "tcp": {"outcome": "not-run"},
    }


def _write_record(record: dict[str, object], output: str | None) -> None:
    rendered = json.dumps(record, indent=2, sort_keys=True) + "\n"
    if output:
        destination = pathlib.Path(output).resolve()
        repo = pathlib.Path(__file__).resolve().parent.parent
        if repo not in destination.parents:
            raise QualificationError("evidence output must remain inside the repository")
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_text(rendered, encoding="utf-8")
    print(rendered, end="")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("discover", "baseline", "full"))
    parser.add_argument("--device", help="explicit discovered endpoint; required only after resolving ambiguity")
    parser.add_argument("--output", help="repository-local sanitized JSON evidence path")
    args = parser.parse_args(argv)
    interfaces = discover()
    record = _base_record(interfaces)
    try:
        if args.mode == "discover":
            _write_record(record, args.output)
            return 0 if interfaces else 2
        endpoint = select_endpoint(interfaces, args.device)
        record["selected_endpoint"] = endpoint.public()
        try:
            uart, attempts = probe(endpoint)
        except ProbeError as error:
            record["serial_probe"] = {"outcome": "inconclusive", "attempts": error.attempts}
            raise
        record["serial_probe"] = {"outcome": "pass", "uart": {key: value for key, value in uart.items() if key != "ending"}, "attempts": attempts}
        baseline = _baseline(endpoint, uart)
        record["baseline"] = {"outcome": "pass" if all(item["outcome"] == "pass" for item in baseline["commands"].values()) else "fail", **baseline}
        if args.mode == "full":
            network = wifi_tcp(endpoint, uart)
            record["wifi"] = {key: value for key, value in network.items() if key != "tcp"}
            record["tcp"] = network.get("tcp", {"outcome": "not-run"})
        _write_record(record, args.output)
        return 0
    except (QualificationError, KeyboardInterrupt) as error:
        record["error"] = str(error) if str(error) else "operation interrupted; credentials were not retained"
        _write_record(record, args.output)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
