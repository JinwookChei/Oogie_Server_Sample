"""
주석을 제거한 코드가 원본과 동일한지 검증한다.

사용법:
    python -I compare_code.py <원본> <원본 인코딩> <수정본> <수정본 인코딩>
    예) python -I compare_code.py backup/Lock.h cp949 ServerCore/Lock.h cp949

비교 방식:
    - //, /* */ 주석을 제거한다. (문자열 리터럴 안의 // 는 고려하지 않음)
    - 주석이 있던 줄은 주석 제거 후 줄 끝 공백을 지우고, 비게 되면 줄 자체를 뺀다.
    - 주석이 없던 줄은 줄 끝 공백·탭까지 그대로 비교한다.
    - 1차: 빈 줄 포함 완전 일치 / 2차: 빈 줄을 무시하고 일치 (주석 주변 빈 줄 정리만 다른 경우)
"""
import difflib
import sys


def strip_comments(path, encoding):
    text = open(path, encoding=encoding, newline='').read().replace('\r\n', '\n')
    out = []
    in_block = False
    for line in text.split('\n'):
        code = ''
        had_comment = in_block
        i = 0
        while i < len(line):
            if in_block:
                end = line.find('*/', i)
                if end < 0:
                    i = len(line)
                else:
                    in_block = False
                    i = end + 2
            elif line.startswith('/*', i):
                in_block = True
                had_comment = True
                i += 2
            elif line.startswith('//', i):
                had_comment = True
                break
            else:
                code += line[i]
                i += 1
        if had_comment:
            code = code.rstrip()
            if code.strip() == '':
                continue
        out.append(code)
    return out


def main():
    sys.stdout.reconfigure(encoding='utf-8')
    if len(sys.argv) != 5:
        print(__doc__)
        sys.exit(2)
    orig = strip_comments(sys.argv[1], sys.argv[2])
    new = strip_comments(sys.argv[3], sys.argv[4])

    if orig == new:
        print('OK: 코드 완전 일치')
        return
    if [l for l in orig if l.strip()] == [l for l in new if l.strip()]:
        print('OK: 코드 일치 (빈 줄 배치만 다름 - 주석 주변 빈 줄인지 확인할 것)')
        return

    print('FAIL: 코드가 달라졌다')
    for d in difflib.unified_diff(orig, new, 'original', 'annotated', lineterm='', n=1):
        print(repr(d) if d[:1] in '+-' and not d.startswith(('---', '+++')) else d)
    sys.exit(1)


if __name__ == '__main__':
    main()
