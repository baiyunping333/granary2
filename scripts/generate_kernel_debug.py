""""Generate a GDB symbol file for a kernel without debug info.

The symbol file is generated by processing the dump of the kernel's `kallsyms`
and `modules.builtin` files.
"""

import bisect
import collections
import os
import re
import sys

KIND_CODE, KIND_GLOBAL_DATA, KIND_CPU_PRIVATE_DATA, KIND_UNKNOWN = 0, 1, 2, 3

# Adapted from: http://stackoverflow.com/a/2233940/247591
def binary_search(a, x, lo=0, hi=None):
  hi = hi if hi is not None else len(a)
  pos = bisect.bisect_left(a, x, lo, hi)
  return (pos if pos != hi and a[pos] == x else -1)

class SectionList(object):
  def __init__(self, sections, text_addr):
    self.addresses = sorted(sections.keys(), reverse=True)
    self.sections = sections
    self.text = text_addr

  def SectionContaining(self, addr):
    index = binary_search(self.addresses, addr)
    return self.sections[self.addresses[index - 1]]

def parse_symbols(kallsyms_lines):
  modules = collections.defaultdict(dict)

  for line in kallsyms_lines:
    parts = line.strip("\s\t\r\n").replace("\t", " ").split(" ")
    addr = int(parts[0], base=16)
    name = parts[2]
    module = "linux"
    if 3 < len(parts):
      module = parts[3][1:-1]
    modules[module][name] = addr
  return modules

SECTION_LINE = re.compile(r"^  \[.*\] ([a-zA-Z0-9_.]+)\s*[A-Z]+\s*([a-f0-9]+).*$")

def parse_sections(sections_lines):
  sections = {}
  text_addr = 0
  for line in sections_lines:
    m = SECTION_LINE.match(line)
    if not m:
      continue
    name = m.group(1)
    begin_addr = int(m.group(2), base=16)
    if ".text" == name:
      text_addr = begin_addr
    sections[begin_addr] = name
  return SectionList(sections, text_addr)

def vmlinux_file_name():
  return os.path.abspath(os.path.join(os.path.dirname(sys.argv[0]),
                                      "../", "vmlinux"))

def main(kallsyms_lines, sections_lines):
  sections = parse_sections(sections_lines)
  symbols = parse_symbols(kallsyms_lines)
  sym_by_sec = collections.defaultdict(dict)
  for name, addr in symbols["linux"].items():
    sec = sections.SectionContaining(addr)
    sym_by_sec[sec][name] = addr
  print "SECTIONS {"
  for sec, syms in sym_by_sec.items():
    print sec, ":"
    print "{"
    sorted_syms = sorted(syms.items(), key=lambda p: p[1])
    for sym, addr in sorted_syms:
      if (addr >> 32) == 0xffffffff:
        print sym, "= ABSOLUTE(0x%x);" % addr
    print "}"
  print "}"

if "__main__" == __name__:
  with open(sys.argv[1], "r") as kallsyms_lines:
    with open(sys.argv[2], "r") as sections_lines:
      main(kallsyms_lines, sections_lines)