"""Inspect Squirrel 2.2.2 closure streams; format follows sqobject.cpp::Load."""

import argparse
import json
from pathlib import Path
import re
import struct


class Reader:
    def __init__(self, data):
        if data[:2] != b"\xfa\xfa":
            data = bytes(value ^ (data[0] ^ 0xFA) for value in data)
        self.data = data
        self.offset = 0

    def read(self, count):
        result = self.data[self.offset:self.offset + count]
        if len(result) != count:
            raise ValueError(f"truncated stream at {self.offset:#x}")
        self.offset += count
        return result

    def unpack(self, fmt):
        return struct.unpack("<" + fmt, self.read(struct.calcsize("<" + fmt)))

    def tag(self, expected):
        found = self.read(len(expected))
        if found != expected:
            raise ValueError(f"tag at {self.offset:#x}: {found!r} != {expected!r}")

    def count(self):
        value, = self.unpack("i")
        if not 0 <= value <= 1_000_000:
            raise ValueError(f"invalid count {value} at {self.offset:#x}")
        return value

    def obj(self):
        kind, = self.unpack("I")
        if kind == 0x01000001:
            return None
        if kind == 0x08000010:
            return self.read(self.count()).decode("cp932", errors="replace")
        if kind == 0x05000002:
            return self.unpack("i")[0]
        if kind == 0x05000004:
            return self.unpack("f")[0]
        raise ValueError(f"unsupported object type {kind:#x}")

    def function(self):
        self.tag(b"TRAP")
        source, name = self.obj(), self.obj()
        self.tag(b"TRAP")
        literals, params, outers, locals_, lines, defaults, ops, children = (
            self.count() for _ in range(8)
        )
        self.tag(b"TRAP")
        literal_values = [self.obj() for _ in range(literals)]
        self.tag(b"TRAP")
        parameters = [self.obj() for _ in range(params)]
        self.tag(b"TRAP")
        outer_values = [(self.unpack("I")[0], self.obj(), self.obj())
                        for _ in range(outers)]
        self.tag(b"TRAP")
        local_values = [(self.obj(), *self.unpack("III")) for _ in range(locals_)]
        self.tag(b"TRAP")
        line_values = [self.unpack("ii") for _ in range(lines)]
        self.tag(b"TRAP")
        default_values = [self.unpack("i")[0] for _ in range(defaults)]
        self.tag(b"TRAP")
        instructions = [self.unpack("iBBBB") for _ in range(ops)]
        self.tag(b"TRAP")
        functions = [self.function() for _ in range(children)]
        stack, generator, varargs = self.unpack("i??")
        return dict(source=source, name=name, literals=literal_values,
                    parameters=parameters, outers=outer_values, locals=local_values,
                    lines=line_values, defaults=default_values, instructions=instructions,
                    functions=functions, stack=stack, generator=generator, varargs=varargs)


def display(function, opcode_names, wanted):
    if wanted.search(function["name"] or ""):
        print(f'FUNCTION {function["source"]}: {function["name"]} '
              f'stack={function["stack"]} generator={function["generator"]}')
        print("PARAMETERS", json.dumps(function["parameters"], ensure_ascii=True))
        print("LOCALS", json.dumps(function["locals"], ensure_ascii=True))
        for index, literal in enumerate(function["literals"]):
            print(f"K{index} = {json.dumps(literal, ensure_ascii=True)}")
        for index, (arg1, op, arg0, arg2, arg3) in enumerate(function["instructions"]):
            print(f"{index:04d} {opcode_names.get(op, str(op)):14} "
                  f"a0={arg0:3} a1={arg1:11} a2={arg2:3} a3={arg3:3}")
    for child in function["functions"]:
        display(child, opcode_names, wanted)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("file", type=Path)
    parser.add_argument("--function", default=".")
    parser.add_argument("--opcodes", type=Path, default=Path(__file__).resolve().parents[1]
                        / "third_party/squirrel-2.2.2/squirrel/sqopcodes.h")
    args = parser.parse_args()
    reader = Reader(args.file.read_bytes())
    reader.tag(b"\xfa\xfaRIQS")
    reader.tag(b"\x01\0\0\0")
    function = reader.function()
    reader.tag(b"LIAT")
    if reader.offset != len(reader.data):
        raise ValueError("trailing bytes after closure stream")
    opcode_names = {int(value, 16): name for name, value in re.findall(
        r"_OP_(\w+)\s*=\s*(0x[0-9A-Fa-f]+)", args.opcodes.read_text())}
    display(function, opcode_names, re.compile(args.function))


if __name__ == "__main__":
    main()
