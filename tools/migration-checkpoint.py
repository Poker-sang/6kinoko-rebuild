"""One-shot repair of two transfer typos; no source changes beyond reviewed patch."""
import hashlib
import subprocess
from pathlib import Path
root = Path(__file__).resolve().parents[1]
script = subprocess.check_output(['git','show','c0fcd9f15a9fc6f45147302be29ac5843afcebd1:tools/migration-checkpoint.py'], cwd=root).decode()
script = script.replace('I87evX5+ZHV','I87evX5+9+ZHV').replace('Tq4UHhdJJSD','Tq4UHhdJSD')
assert hashlib.sha256(script.encode()).hexdigest() == '4446ed0756b7c4f104233eee6ae3f55822b38070b81d7181e623a3386093d18e'
exec(compile(script, __file__, 'exec'))
