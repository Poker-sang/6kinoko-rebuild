"""One-shot hash-checked source patch; removed after the checkpoint applies."""
from pathlib import Path
import base64, hashlib, subprocess, zlib
patch = zlib.decompress(base64.b64decode('''eJy9XHt32zaW/9+fAnVnO1L0MN8i4zYzrqt0ferYWVttZzfrowMSoM1GIlWScuym/u57LwBSoERZctvZnJljCQQu7gu/+yBUlsQxGQxuk5LQo9N39CM/T4qyGJYPJQnXBg6SlPEHwqgTUT4aDn3PdozAJKZheI5zMBgMNmgc9Hq9TTr//CcZuF7fNEgP/zgEBkqa3/JymqTRbMn4lCU5j8osT3jR+Zik2cdsOuO3NHqc0jAh73/89vzslKjJ3QNyQChj01kS5jR/rBbkvGQ8mhbLxSLLS3I9OZmcnR4Q/Ffk0dFiRss4y+dHat6cz7P8cRgtFge9jTlq8zincz6NsoWct5VWvkzLRMycL2iJc4HJ3SKucdwUk7yS7iT/++vJydZqPv7Y59HrRfFSXFPE1tttdtF95NH8bIFrTXv6vy1DxcZ/vth/livpb/azSmTrNBmjynNT//jxueCXipD8GnpuKCEEQwbdaogSlPbRbYKCb/N7VLdygbdfAtEztoTd01lJKBlcgMVrsJuXhlkgNLxfPHlvRnPA7cbDhzE/OP$INVALID'''))
assert hashlib.sha256(patch).hexdigest() == 'c4da6430a65da1f37ee8eccbbabdc83e480efca676488373031c709963d48b17'
subprocess.run(['git','apply','--check','--whitespace=nowarn','-'],input=patch,check=True)
subprocess.run(['git','apply','--whitespace=nowarn','-'],input=patch,check=True)
Path(__file__).unlink()
