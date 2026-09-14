from pathlib import Path
p = Path('src/game/client/deferred/clight_manager.cpp')
s = p.read_text(encoding='utf-8')
old = 'for( int i = 0; i < s.count; i++ )'
if s.count(old) != 1:
    raise SystemExit(f'expected one light batch loop, found {s.count(old)}')
s = s.replace(old, 'for( unsigned int i = 0; i < s.count; i++ )', 1)
p.write_text(s, encoding='utf-8')
