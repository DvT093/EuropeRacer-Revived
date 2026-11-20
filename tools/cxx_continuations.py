# Lists MSVC (x86) C++ EH catch funclets and their continuation addresses.
import struct, sys
d = open(sys.argv[1], 'rb').read()
pe = struct.unpack_from('<I', d, 0x3c)[0]; n = struct.unpack_from('<H', d, pe + 6)[0]
opt = struct.unpack_from('<H', d, pe + 20)[0]; base = struct.unpack_from('<I', d, pe + 24 + 28)[0]
secs = []
for i in range(n):
    s = pe + 24 + opt + 40 * i
    name = d[s:s + 8].rstrip(b'\0'); vs, va, rs, ra = struct.unpack_from('<IIII', d, s + 8)
    ch = struct.unpack_from('<I', d, s + 36)[0]
    secs.append((name, base + va, max(vs, rs), ra, rs, ch))
def off(v):
    for nm, a, sz, ra, rs, ch in secs:
        if a <= v < a + rs: return ra + v - a
    return None
def is_text(v):
    for nm, a, sz, ra, rs, ch in secs:
        if ch & 0x20 and a <= v < a + sz: return True
    return False
def u32(v): o = off(v); return None if o is None else struct.unpack_from('<I', d, o)[0]
funcinfos = []
for nm, a, sz, ra, rs, ch in secs:
    if ch & 0x20: continue
    for o in range(ra, ra + rs - 4, 4):
        if struct.unpack_from('<I', d, o)[0] == 0x19930520:
            funcinfos.append(a + o - ra)
handlers, conts, bad = set(), set(), []
for fi in funcinfos:
    ntry, ptry = u32(fi + 12), u32(fi + 16)
    if not ntry or ntry > 64 or ptry is None or off(ptry) is None: continue
    for t in range(ntry):
        e = ptry + 20 * t
        ncatch, parr = u32(e + 12), u32(e + 16)
        if not ncatch or ncatch > 32 or parr is None or off(parr) is None: continue
        for k in range(ncatch):
            h = u32(parr + 16 * k + 12)
            if h is None or not is_text(h): continue
            handlers.add(h)
            o = off(h); code = d[o:o + 0x800]
            found = False
            for j in range(len(code) - 5):
                if code[j] == 0xB8 and code[j + 5] == 0xC3:
                    c = struct.unpack_from('<I', code, j + 1)[0]
                    if is_text(c): conts.add(c); found = True; break
            if not found: bad.append(h)
print('# funcinfo', len(funcinfos), 'catch handlers', len(handlers), 'continuations', len(conts), 'unresolved', len(bad), file=sys.stderr)
for h in sorted(bad): print('# no continuation found for catch handler %08X' % h, file=sys.stderr)
for c in sorted(conts): print('entry %08X' % c)
