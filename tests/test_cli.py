"""Black-box tests for the debugger and standalone hex loader."""
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
BIN = ROOT / 'lc4_debugger'


def run(path, commands='', *args):
    return subprocess.run([str(BIN), str(path), *args], input=commands,
                          text=True, capture_output=True, timeout=5)


class DebuggerTests(unittest.TestCase):
    def test_sum(self):
        result = run(ROOT / 'examples/sum.hex',
                     'step\nregs\nrun 20\nmem 2000 1\nquit\n')
        self.assertEqual(result.returncode, 0)
        self.assertIn('R0: x000F', result.stdout)
        self.assertIn('R3: x000F', result.stdout)
        self.assertIn('x2000: x000F', result.stdout)
        self.assertIn('PC: x0009', result.stdout)

    def test_inspection_does_not_execute(self):
        result = run(ROOT / 'examples/arithmetic.hex', 'regs\nmem 0000 6\nregs\n')
        self.assertEqual(result.stdout.count('PC: x0000'), 2)

    def test_invalid_commands_and_boundary(self):
        result = run(ROOT / 'examples/arithmetic.hex',
                     'run -1\nrun 1000001\nmem FFFF 4\nmem 10000 1\nregs extra\nquit\n')
        self.assertEqual(result.returncode, 0)
        self.assertIn('xFFFF: x0000', result.stdout)
        self.assertIn('Unknown command', result.stdout)
        self.assertEqual(result.stdout.count('Use: run COUNT'), 2)

    def test_loader_format(self):
        with tempfile.TemporaryDirectory() as folder:
            path = pathlib.Path(folder) / 'sample.hex'
            path.write_text('; comment\n@0020\n9005 # word\n@FFFF\nABCD\n')
            result = run(path, 'step\nmem FFFF 1\nquit\n', '0020')
            self.assertEqual(result.returncode, 0)
            self.assertIn('R0: x0005', result.stdout)
            self.assertIn('xFFFF: xABCD', result.stdout)

    def test_malformed_loader_inputs(self):
        with tempfile.TemporaryDirectory() as folder:
            path = pathlib.Path(folder) / 'bad.hex'
            for content in ['', '# empty\n', '10000\n', '-1\n', 'ZZZZ\n',
                            '9005 9203\n', '@FFFF\n0000\n0000\n',
                            '@10000\n9005\n', 'A' * 300 + '\n']:
                with self.subTest(content=content[:30]):
                    path.write_text(content)
                    self.assertNotEqual(run(path).returncode, 0)

    def test_execution_error_returns_to_prompt(self):
        with tempfile.TemporaryDirectory() as folder:
            path = pathlib.Path(folder) / 'invalid.hex'
            path.write_text('3000\n')
            result = run(path, 'step\nregs\nquit\n')
            self.assertIn('Unsupported opcode', result.stderr)
            self.assertIn('PC: x0000', result.stdout)

    def test_step_limit(self):
        with tempfile.TemporaryDirectory() as folder:
            path = pathlib.Path(folder) / 'loop.hex'
            path.write_text('CFFF\n')
            result = run(path, 'run 100\nquit\n')
            self.assertIn('Executed 100 instruction(s); paused', result.stdout)
            self.assertIn('PC: x0000', result.stdout)

    def test_long_command(self):
        result = run(ROOT / 'examples/arithmetic.hex', 's' * 300 + '\nregs\nquit\n')
        self.assertIn('Command is too long', result.stdout)
        self.assertIn('PC: x0000', result.stdout)


if __name__ == '__main__':
    unittest.main()
