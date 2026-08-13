#!/usr/bin/env python3
"""Minimal host tests for the ESP-AT parser and secret redaction."""

import importlib.util
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parent.parent
SPEC = importlib.util.spec_from_file_location("qualify_espat", ROOT / "scripts" / "qualify_espat.py")
assert SPEC and SPEC.loader
MODULE = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = MODULE
SPEC.loader.exec_module(MODULE)


class ResponseParserTests(unittest.TestCase):
    def test_echo_payload_urc_prompt_and_final_result(self):
        response = MODULE.parse_response(
            b'AT+CIPSEND=4\r\nOK\r\n> \r\n+IPD,4:pong\r\nSEND OK\r\n',
            "AT+CIPSEND=4",
        )
        self.assertEqual(response.echo, ["AT+CIPSEND=4"])
        self.assertEqual(response.results, ["OK", "SEND OK"])
        self.assertEqual(response.prompts, [">"])
        self.assertEqual(response.data, ["pong"])
        self.assertEqual(response.outcome, "pass")

    def test_timeout_without_final_result_is_inconclusive(self):
        response = MODULE.parse_response("AT\r\n", "AT", timed_out=True)
        self.assertEqual(response.outcome, "inconclusive")
        self.assertTrue(response.timed_out)

    def test_error_is_failure(self):
        self.assertEqual(MODULE.parse_response("ERROR\r\n").outcome, "fail")

    def test_incremental_bytearray_is_supported(self):
        self.assertEqual(MODULE.parse_response(bytearray(b"OK\r\n")).outcome, "pass")

    def test_binary_noise_is_summarized(self):
        response = MODULE.parse_response(b"\x00\xff\x00\xff\r\n", timed_out=True).public()
        self.assertRegex(response["lines"][0], r"^<undecodable serial data:")


class RedactionTests(unittest.TestCase):
    def test_explicit_secrets_and_join_command_are_redacted(self):
        raw = 'AT+CWJAP="office-net","correct horse"\r\noffice-net correct horse'
        redacted = MODULE.redact(raw, ("office-net", "correct horse"))
        self.assertNotIn("office-net", redacted)
        self.assertNotIn("correct horse", redacted)
        self.assertIn("<redacted>", redacted)

    def test_empty_secret_does_not_destroy_text(self):
        self.assertEqual(MODULE.redact("OK", ("",)), "OK")

    def test_join_command_is_redacted_without_explicit_secret_list(self):
        redacted = MODULE.redact('AT+CWJAP="ssid","passphrase"', ())
        self.assertEqual(redacted, 'AT+CWJAP="<redacted>","<redacted>"')

    def test_control_characters_are_rejected(self):
        with self.assertRaises(MODULE.QualificationError):
            MODULE._at_quote("bad\nssid")


if __name__ == "__main__":
    unittest.main()
