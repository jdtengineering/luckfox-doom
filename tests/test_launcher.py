"""Keep background audio processes from writing into a Sixel frame."""
import importlib.util
from pathlib import Path
import subprocess
import unittest
from unittest.mock import MagicMock, patch

spec = importlib.util.spec_from_file_location("play", Path(__file__).resolve().parents[1] / "scripts/play.py")
play = importlib.util.module_from_spec(spec)
spec.loader.exec_module(play)

class LauncherTests(unittest.TestCase):
    def test_audio_output_is_isolated(self):
        receiver, player = MagicMock(), MagicMock()
        receiver.poll.return_value = player.poll.return_value = None
        with patch("sys.argv", ["play.py", "--audio", "--pixels"]), \
             patch.object(play.shutil, "which", return_value="available"), \
             patch.object(play.subprocess, "run"), \
             patch.object(play.subprocess, "call", return_value=0), \
             patch.object(play.subprocess, "Popen", side_effect=[receiver, player]) as spawn:
            self.assertEqual(play.main(), 0)
        receive_call, player_call = spawn.call_args_list
        log = receive_call.kwargs["stderr"]
        self.assertIs(player_call.kwargs["stderr"], log)
        self.assertEqual(player_call.kwargs["stdout"], subprocess.DEVNULL)
        self.assertIn("-nostats", player_call.args[0])
        self.assertTrue(log.closed)
        receiver.terminate.assert_called_once()
        player.terminate.assert_called_once()

if __name__ == "__main__": unittest.main()
